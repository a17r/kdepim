/* Copyright 2009 Klarälvdalens Datakonsult AB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "importjob.h"

#include "kmfolder.h"
#include "folderutil.h"

#include <kdebug.h>
#include <kzip.h>
#include <ktar.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>

#include <qwidget.h>
#include <qtimer.h>

using namespace KMail;

ImportJob::ImportJob( QWidget *parentWidget )
  : QObject( parentWidget ),
    mArchive( 0 ),
    mRootFolder( 0 ),
    mParentWidget( parentWidget ),
    mNumberOfImportedMessages( 0 ),
    mCurrentFolder( 0 )
{
}

ImportJob::~ImportJob()
{
  if ( mArchive && mArchive->isOpened() ) {
    mArchive->close();
  }
  delete mArchive;
  mArchive = 0;
}

void ImportJob::setFile( const KURL &archiveFile )
{
  mArchiveFile = archiveFile;
}

void ImportJob::setRootFolder( KMFolder *rootFolder )
{
  mRootFolder = rootFolder;
}

void ImportJob::finish()
{
  kdDebug(5006) << "Finished import job." << endl;
  QString text = i18n( "Importing the archive file '%1' into the folder '%2' succeeded." )
                     .arg( mArchiveFile.path() ).arg( mRootFolder->name() );
  text += "\n" + i18n( "%1 messages were imported." ).arg( mNumberOfImportedMessages );
  KMessageBox::information( mParentWidget, text, i18n( "Import finished." ) );
  deleteLater();
}

void ImportJob::abort( const QString &errorMessage )
{
  QString text = i18n( "Failed to import the archive into folder '%1'." ).arg( mRootFolder->name() );
  text += "\n" + errorMessage;
  KMessageBox::sorry( mParentWidget, text, i18n( "Importing archive failed." ) );
  deleteLater();
}

KMFolder * ImportJob::createSubFolder( KMFolder *parent, const QString &folderName )
{
  if ( !parent->createChildFolder() ) {
    abort( i18n( "Unable to create subfolder for folder '%1'." ).arg( parent->name() ) );
    return 0;
  }

  KMFolder *newFolder = FolderUtil::createSubFolder( parent, parent->child(), folderName, QString(),
                                                     KMFolderTypeMaildir );
  if ( !newFolder ) {
    abort( i18n( "Unable to create subfolder for folder '%1'." ).arg( parent->name() ) );
    return 0;
  }
  else return newFolder;
}

void ImportJob::enqueueMessages( const KArchiveDirectory *dir, KMFolder *folder )
{
  const KArchiveDirectory *messageDir = dynamic_cast<const KArchiveDirectory*>( dir->entry( "cur" ) );
  if ( messageDir ) {
    Messages messagesToQueue;
    messagesToQueue.parent = folder;
    const QStringList entries = messageDir->entries();
    for ( uint i = 0; i < entries.size(); i++ ) {
      const KArchiveEntry *entry = messageDir->entry( entries[i] );
      Q_ASSERT( entry );
      if ( entry->isDirectory() ) {
        kdWarning(5006) << "Unexpected subdirectory in archive folder " << dir->name() << endl;
      }
      else {
        kdDebug(5006) << "Queueing message " << entry->name() << endl;
        const KArchiveFile *file = static_cast<const KArchiveFile*>( entry );
        messagesToQueue.files.append( file );
      }
    }
    mQueuedMessages.append( messagesToQueue );
  }
  else {
    kdWarning(5006) << "No 'cur' subdirectory for archive directory " << dir->name() << endl;
  }
}

void ImportJob::importNextMessage()
{
  if ( mQueuedMessages.isEmpty() ) {
    kdDebug(5006) << "importNextMessage(): Processed all messages in the queue." << endl;
    if ( mCurrentFolder ) {
      mCurrentFolder->close( "ImportJob" );
    }
    mCurrentFolder = 0;
    importNextDirectory();
    return;
  }

  Messages &messages = mQueuedMessages.front();
  if ( messages.files.isEmpty() ) {
    mQueuedMessages.pop_front();
    importNextMessage();
    return;
  }

  KMFolder *folder = messages.parent;
  if ( folder != mCurrentFolder ) {
    kdDebug(5006) << "importNextMessage(): Processed all messages in the current folder of the queue." << endl;
    if ( mCurrentFolder ) {
      mCurrentFolder->close( "ImportJob" );
    }
    mCurrentFolder = folder;
    if ( mCurrentFolder->open( "ImportJob" ) != 0 ) {
      abort( i18n( "Unable to open folder '%1'." ).arg( mCurrentFolder->name() ) );
      return;
    }
    kdDebug(5006) << "importNextMessage(): Current folder of queue is now: " << mCurrentFolder->name() << endl;
  }
  const KArchiveFile *file = messages.files.first();
  Q_ASSERT( file );
  messages.files.removeFirst();

  KMMessage *newMessage = new KMMessage();
  newMessage->fromByteArray( file->data(), true /* setStatus */ );
  int retIndex;
  if ( mCurrentFolder->addMsg( newMessage, &retIndex ) != 0 ) {
    abort( i18n( "Failed to add a message to the folder '%1'." ).arg( mCurrentFolder->name() ) );
    return;
  }
  else {
    mNumberOfImportedMessages++;
    kdDebug(5006) << "Added message with subject " /*<< newMessage->subject()*/ // < this causes a pure virtual method to be called...
                  << " to folder " << mCurrentFolder->name() << " at index " << retIndex << endl;
  }
  QTimer::singleShot( 0, this, SLOT( importNextMessage() ) );
}

void ImportJob::importNextDirectory()
{
  if ( mQueuedDirectories.isEmpty() ) {
    finish();
    return;
  }

  Folder folder = mQueuedDirectories.first();
  KMFolder *currentFolder = folder.parent;
  mQueuedDirectories.pop_front();
  kdDebug(5006) << "importNextDirectory(): Working on directory " << folder.archiveDir->name() << endl;

  QStringList entries = folder.archiveDir->entries();
  for ( uint i = 0; i < entries.size(); i++ ) {
    const KArchiveEntry *entry = folder.archiveDir->entry( entries[i] );
    Q_ASSERT( entry );
    kdDebug(5006) << "Queueing entry " << entry->name() << endl;
    if ( entry->isDirectory() ) {
      const KArchiveDirectory *dir = static_cast<const KArchiveDirectory*>( entry );
      if ( !dir->name().startsWith( "." ) ) {

        kdDebug(5006) << "Queueing messages in folder " << entry->name() << endl;
        KMFolder *subFolder = createSubFolder( currentFolder, entry->name() );
        if ( !subFolder )
          return;

        enqueueMessages( dir, subFolder );

        const QString dirName = "." + entries[i] + ".directory";
        QStringList::iterator it = entries.find( dirName );
        if ( it != entries.end() ) {
          Q_ASSERT( folder.archiveDir->entry( *it )->isDirectory() );
          Q_ASSERT( folder.archiveDir->entry( *it )->name() == dirName );
          Folder newFolder;
          newFolder.archiveDir = static_cast<const KArchiveDirectory*>( folder.archiveDir->entry( *it ) );
          newFolder.parent = subFolder;
          kdDebug(5006) << "Enqueueing directory " << newFolder.archiveDir->name() << endl;
          mQueuedDirectories.push_back( newFolder );
        }
      }
    }
  }

  importNextMessage();
}

// TODO:
// BUGS:
//    Online IMAP can fail spectacular, for example when cancelling upload
//    Online IMAP: Inform that messages are still being uploaded on finish()!
void ImportJob::start()
{
  Q_ASSERT( mRootFolder );
  Q_ASSERT( mArchiveFile.isValid() );

  KMimeType::Ptr mimeType = KMimeType::findByURL( mArchiveFile, 0, true /* local file */ );
  if ( !mimeType->patterns().grep( "tar", false /* no case-sensitive */ ).isEmpty() )
    mArchive = new KTar( mArchiveFile.path() );
  else if ( !mimeType->patterns().grep( "zip", false ).isEmpty() )
    mArchive = new KZip( mArchiveFile.path() );
  else {
    abort( i18n( "The file '%1' does not appear to be a valid archive." ).arg( mArchiveFile.path() ) );
    return;
  }

  if ( !mArchive->open( IO_ReadOnly ) ) {
    abort( i18n( "Unable to open archive file '%1'" ).arg( mArchiveFile.path() ) );
    return;
  }

  Folder nextFolder;
  nextFolder.archiveDir = mArchive->directory();
  nextFolder.parent = mRootFolder;
  mQueuedDirectories.push_back( nextFolder );
  importNextDirectory();
}

#include "importjob.moc"
