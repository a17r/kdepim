/* -*- mode: C++; c-file-style: "gnu" -*-
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "mailviewer_p.h"
#include "kmreaderwin.h"

#include <errno.h>

//KDE includes
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <kascii.h>
#include <KCharsets>
#include <kcursorsaver.h>
#include <KFileDialog>
#include <KGuiItem>
#include <KHTMLPart>
#include <KHTMLView>
#include <KMenu>
#include <KMessageBox>
#include <KMimeType>
#include <KMimeTypeChooser>
#include <KMimeTypeTrader>
#include <KRun>
#include <KSelectAction>
#include <KSharedConfigPtr>
#include <KStandardDirs>
#include <KStandardGuiItem>
#include <KTemporaryFile>
#include <KToggleAction>

#include <KIO/NetAccess>
#include <KABC/Addressee>
#include <KABC/VCardConverter>

#include <Akonadi/ItemModifyJob>

#include <kpimutils/kfileio.h>
#include <kde_file.h>

#include <dom/html_element.h>
#include <dom/html_block.h>
#include <dom/html_document.h>
#include <dom/dom_string.h>

//Qt includes
#include <QClipboard>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QImageReader>
#include <QItemSelectionModel>
#include <QSignalMapper>
#include <QSplitter>
#include <QStyle>
#include <QTextDocument>
#include <QTreeView>
#include <QVBoxLayout>

//libkdepim
#include "libkdepim/broadcaststatus.h"
#include "libkdepim/attachmentpropertiesdialog.h"

//own includes
#include "attachmentdialog.h"
#include "attachmentstrategy.h"
#include "csshelper.h"
#include "editorwatcher.h"
#include "global.h"
#include "globalsettings.h"
#include "headerstyle.h"
#include "headerstrategy.h"
#include "htmlstatusbar.h"
#include "khtmlparthtmlwriter.h"
#include "mailsourceviewer.h"
#include "mimetreemodel.h"
#include "nodehelper.h"
#include "objecttreeparser.h"
#include "stringutil.h"
#include "urlhandlermanager.h"
#include "util.h"
#include "vcardviewer.h"

#include "interfaces/bodypart.h"
#include "interfaces/htmlwriter.h"

const int MailViewerPrivate::delay = 150;

MailViewerPrivate::MailViewerPrivate(MailViewer *aParent,
                         KSharedConfigPtr config,
                         QWidget *mainWindow,
                         KActionCollection* actionCollection)
  : QObject(aParent),
    mMessage( 0 ),
    mAttachmentStrategy( 0 ),
    mHeaderStrategy( 0 ),
    mHeaderStyle( 0 ),
    mUpdateReaderWinTimer( 0 ),
    mResizeTimer( 0 ),
    mDelayedMarkTimer( 0 ),
    mOldGlobalOverrideEncoding( "---" ), // init with dummy value
    mCSSHelper( 0 ),
    mMainWindow( mainWindow ),
    mActionCollection( actionCollection ),
    mCopyAction( 0 ),
    mCopyURLAction( 0 ),
    mUrlOpenAction( 0 ),
    mSelectAllAction( 0 ),
    mScrollUpAction( 0 ),
    mScrollDownAction( 0 ),
    mScrollUpMoreAction( 0 ),
    mScrollDownMoreAction( 0 ),
    mToggleMimePartTreeAction( 0 ),
    mSelectEncodingAction( 0 ),
    mToggleFixFontAction( 0 ),
    mHtmlWriter( 0 ),
    mSavedRelativePosition( 0 ),
    mDecrytMessageOverwrite( false ),
    mShowSignatureDetails( false ),
    mShowAttachmentQuicklist( true ),
    q( aParent )
{
  if ( !mainWindow )
    mainWindow = aParent;

  mDeleteMessage = false;

  mHtmlOverride = false;
  mHtmlLoadExtOverride = false;
  mHtmlLoadExternal = false;

  Global::instance()->setConfig( config );
  GlobalSettings::self()->setSharedConfig( Global::instance()->config() );
  GlobalSettings::self()->readConfig(); //need to re-read the config as the config object might be different than the default mailviewerrc
  mUpdateReaderWinTimer.setObjectName( "mUpdateReaderWinTimer" );
  mDelayedMarkTimer.setObjectName( "mDelayedMarkTimer" );
  mResizeTimer.setObjectName( "mResizeTimer" );

  mExternalWindow  = ( aParent == mainWindow );
  mSplitterSizes << 180 << 100;
  mLastSerNum = 0;
  mWaitingForSerNum = 0;
  mLastStatus.clear();
  mMsgDisplay = true;
  mPrinting = false;
  mIsPlainText = false;

  createWidgets();
  createActions();
  initHtmlWidget();
  readConfig();

  mLevelQuote = GlobalSettings::self()->collapseQuoteLevelSpin() - 1;
  mLevelQuote = 1;

  mResizeTimer.setSingleShot( true );
  connect( &mResizeTimer, SIGNAL(timeout()),
           this, SLOT(slotDelayedResize()) );

  mDelayedMarkTimer.setSingleShot( true );
  connect( &mDelayedMarkTimer, SIGNAL(timeout()),
           this, SLOT(slotTouchMessage()) );

  mUpdateReaderWinTimer.setSingleShot( true );
  connect( &mUpdateReaderWinTimer, SIGNAL(timeout()),
           this, SLOT(updateReaderWin()) );

 connect( this, SIGNAL(urlClicked(const KUrl&,int)),
          this, SLOT(slotUrlClicked()) );
}

MailViewerPrivate::~MailViewerPrivate()
{
  clearBodyPartMementos();
  delete mHtmlWriter; mHtmlWriter = 0;
  delete mCSSHelper;
  if ( mDeleteMessage )
    delete mMessage;
  mMessage = 0;
  removeTempFiles();
}



//-----------------------------------------------------------------------------
KMime::Content * MailViewerPrivate::nodeFromUrl( const KUrl & url )
{
  if ( url.isEmpty() )
    return 0;
  if ( !url.isLocalFile() )
    return 0;

  QString path = url.toLocalFile();
  uint right = path.lastIndexOf( '/' );
  uint left = path.lastIndexOf( '.', right );

  KMime::ContentIndex index(path.mid( left + 1, right - left - 1 ));
  KMime::Content *node = mMessage->content( index );

  return node;
}

KMime::Content* MailViewerPrivate::nodeForContentIndex( const KMime::ContentIndex& index )
{
  return  mMessage->content( index );
}


void MailViewerPrivate::openAttachment( KMime::Content* node, const QString & name )
{
  if( !node ) {
    return;
  }

  QString atmName = name;
  QString str, pname, cmd, fileName;

  if ( name.isEmpty() )
    atmName = tempFileUrlFromNode( node ).toLocalFile();

  if ( node->contentType()->mediaType() == "message" )
  {
    atmViewMsg( node );
    return;
  }

  // determine the MIME type of the attachment
  KMimeType::Ptr mimetype;
  // prefer the value of the Content-Type header
  mimetype = KMimeType::mimeType( QString::fromLatin1( node->contentType()->mimeType().toLower() ), KMimeType::ResolveAliases );
  if ( !mimetype.isNull() && mimetype->is( KABC::Addressee::mimeType() ) ) {
    showVCard( node );
    return;
  }

  // special case treatment on mac
  if ( Util::handleUrlOnMac( atmName ) )
    return;

  if ( mimetype.isNull() || mimetype->name() == "application/octet-stream" ) {
    // consider the filename if mimetype can not be found by content-type
    mimetype = KMimeType::findByPath( name, 0, true /* no disk access */  );

  }
  if ( ( mimetype->name() == "application/octet-stream" )
      /*FIXME(Andras) port it && msgPart.isComplete() */) {
    // consider the attachment's contents if neither the Content-Type header
    // nor the filename give us a clue
    mimetype = KMimeType::findByFileContent( name );
  }

  KService::Ptr offer =
      KMimeTypeTrader::self()->preferredService( mimetype->name(), "Application" );

  QString filenameText = NodeHelper::fileName( node );

  AttachmentDialog dialog( mMainWindow, filenameText, offer ? offer->name() : QString(),
                                  QString::fromLatin1( "askSave_" ) + mimetype->name() );
  const int choice = dialog.exec();

  if ( choice == AttachmentDialog::Save ) {
    saveAttachments( KMime::Content::List() << node );
  }
  else if ( choice == AttachmentDialog::Open ) { // Open
      attachmentOpen( node );
  } else if ( choice == AttachmentDialog::OpenWith ) {
      attachmentOpenWith( node );
  } else { // Cancel
    kDebug() <<"Canceled opening attachment";
  }

}

bool MailViewerPrivate::deleteAttachment(KMime::Content * node, bool showWarning)
{
  if ( !node )
    return true;
  KMime::Content *parent = node->parent();
  if ( !parent )
    return true;

  if ( showWarning && KMessageBox::warningContinueCancel( mMainWindow,
       i18n("Deleting an attachment might invalidate any digital signature on this message."),
       i18n("Delete Attachment"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
       "DeleteAttachmentSignatureWarning" )
     != KMessageBox::Continue ) {
    return false; //cancelled
  }

  mMimePartModel->setRoot( 0 ); //don't confuse the model
  parent->removeContent( node, true );
  mMimePartModel->setRoot( mMessage );

  mMessageItem.setPayloadFromData( mMessage->encodedContent() );
  Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( mMessageItem );
//TODO(Andras) error checking?   connect( job, SIGNAL(result(KJob*)), SLOT(imapItemUpdateResult(KJob*)) );

  return true;
}

bool MailViewerPrivate::editAttachment( KMime::Content * node, bool showWarning )
{
  //FIXME(Andras) just that I don't forget...handle the case when the user starts editing and switches to another message meantime
  if ( showWarning && KMessageBox::warningContinueCancel( mMainWindow,
        i18n("Modifying an attachment might invalidate any digital signature on this message."),
        i18n("Edit Attachment"), KGuiItem( i18n("Edit"), "document-properties" ), KStandardGuiItem::cancel(),
        "EditAttachmentSignatureWarning" )
        != KMessageBox::Continue ) {
    return false;
  }

  KTemporaryFile file;
  file.setAutoRemove( false );
  if ( !file.open() ) {
    kWarning() << "Edit Attachment: Unable to open temp file.";
    return true;
  }
  file.write( node->decodedContent() );
  file.flush();

  EditorWatcher *watcher =
      new EditorWatcher( KUrl( file.fileName() ), node->contentType()->mimeType(),
                                false, this, mMainWindow );
  mEditorWatchers[ watcher ] = node;

  connect( watcher, SIGNAL(editDone(EditorWatcher*)), SLOT(slotAttachmentEditDone(EditorWatcher*)) );
  if ( !watcher->start() ) {
    QFile::remove( file.fileName() );
  }

  return true;

/*FIXME(Andras) port to akonadi  KMEditAttachmentCommand* command = new KMEditAttachmentCommand( node, message(), this );
  command->start();
  */
}
void MailViewerPrivate::showAttachmentPopup( int id, const QString & name, const QPoint &p )
{
  prepareHandleAttachment( id, name );
  KMenu *menu = new KMenu();
  QAction *action;

  QSignalMapper *attachmentMapper = new QSignalMapper( menu );
  connect( attachmentMapper, SIGNAL( mapped( int ) ),
           this, SLOT( slotHandleAttachment( int ) ) );
/*FIXME(Andras) port to akonadi
  action = menu->addAction(SmallIcon("document-open"),i18nc("to open", "Open"));
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Open );

  action = menu->addAction(i18n("Open With..."));
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::OpenWith );

  action = menu->addAction(i18nc("to view something", "View") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::View );

  action = menu->addAction(SmallIcon("document-save-as"),i18n("Save As...") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Save );

  action = menu->addAction(SmallIcon("edit-copy"), i18n("Copy") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Copy );

  const bool canChange = message()->parent() ? !message()->parent()->isReadOnly() : false;

  if ( GlobalSettings::self()->allowAttachmentEditing() ) {
    action = menu->addAction(SmallIcon("document-properties"), i18n("Edit Attachment") );
    connect( action, SIGNAL(triggered()), attachmentMapper, SLOT(map()) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Edit );
    action->setEnabled( canChange );
  }
  if ( GlobalSettings::self()->allowAttachmentDeletion() ) {
    action = menu->addAction(SmallIcon("edit-delete"), i18n("Delete Attachment") );
    connect( action, SIGNAL(triggered()), attachmentMapper, SLOT(map()) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Delete );
    action->setEnabled( canChange );
  }
  if ( name.endsWith( QLatin1String(".xia"), Qt::CaseInsensitive ) &&
       Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" ) ) {
    action = menu->addAction( i18n( "Decrypt With Chiasmus..." ) );
    connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
    attachmentMapper->setMapping( action, KMHandleAttachmentCommand::ChiasmusEncrypt );
  }
  action = menu->addAction(i18n("Properties") );
  connect( action, SIGNAL( triggered(bool) ), attachmentMapper, SLOT( map() ) );
  attachmentMapper->setMapping( action, KMHandleAttachmentCommand::Properties );
  */
  menu->exec( p );
  delete menu;
}

Interface::BodyPartMemento *MailViewerPrivate::bodyPartMemento( const KMime::Content *node,
                                               const QByteArray &which ) const
{
  const QByteArray index = NodeHelper::path(node) + ':' + which.toLower();
  const QMap<QByteArray,Interface::BodyPartMemento*>::const_iterator it =
    mBodyPartMementoMap.find( index );

  if ( it == mBodyPartMementoMap.end() ) {
    return 0;
  } else {
    return it.value();
  }
}

void MailViewerPrivate::setBodyPartMemento( const KMime::Content *node,
                                      const QByteArray &which,
                                      Interface::BodyPartMemento *memento )
{
  const QByteArray index = NodeHelper::path(node) + ':' + which.toLower();

  const QMap<QByteArray,Interface::BodyPartMemento*>::iterator it =
    mBodyPartMementoMap.lowerBound( index );

  if ( it != mBodyPartMementoMap.end() && it.key() == index ) {
    if ( memento && memento == it.value() ) {
      return;
    }

    delete it.value();

    if ( memento ) {
      it.value() = memento;
    } else {
      mBodyPartMementoMap.erase( it );
    }
  } else {
    if ( memento ) {
      mBodyPartMementoMap.insert( index, memento );
    }
  }
/*FIXME(Andras) review, port
  if ( Observable * o = memento ? memento->asObservable() : 0 ) {
    o->attach( this );
  }
  */
}


void MailViewerPrivate::clearBodyPartMementos()
{
  for ( QMap<QByteArray,Interface::BodyPartMemento*>::iterator
          it = mBodyPartMementoMap.begin(), end = mBodyPartMementoMap.end();
        it != end; ++it ) {
    delete it.value();
  }
  mBodyPartMementoMap.clear();
}

void MailViewerPrivate::prepareHandleAttachment( int id, const QString& fileName )
{
  /*TODO(Andras) remove the whole method
  mAtmCurrent = id;
  mAtmCurrentName = fileName;
  */
}

// This function returns the complete data that were in this
// message parts - *after* all encryption has been removed that
// could be removed.
// - This is used to store the message in decrypted form.
void MailViewerPrivate::objectTreeToDecryptedMsg( KMime::Content* node,
                                            QByteArray& resultingData,
                                            KMime::Message& theMessage,
                                            bool weAreReplacingTheRootNode,
                                            int recCount )
{
  kDebug() << "-------------------------------------------------";
  kDebug() << "START" << "(" << recCount << ")";
  if( node ) {
    KMime::Content* curNode = node;
    KMime::Content* dataNode = curNode;
    KMime::Content * child = NodeHelper::firstChild( node );
    bool bIsMultipart = false;

    QString type = curNode->contentType()->mediaType();
    QString subType = curNode->contentType()->subType();
    if ( type == "text") {
      kDebug() <<"* text *";
      kDebug() << subType;
    } else if ( type == "multipart ") {
        kDebug() <<"* multipart *";
        kDebug() << subType;
        bIsMultipart = true;
        if ( subType == "encrypted" ) {
            if ( child ) {
              /*
                  ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
              */
              KMime::Content* data =
                ObjectTreeParser::findType( child, "application/octet-stream", false, true );
              if ( !data )
                data = ObjectTreeParser::findType( child, "application/pkcs7-mime", false, true );
              dataNode = NodeHelper::firstChild( data );
            }
        }
    } else if ( type == "message" ) {
        if ( subType == "rfc822") {
              if ( child )
                dataNode = child;
        }
    } else if ( type == "application" ) {
          kDebug() <<"* application *";
          kDebug() << subType;
          if ( subType == "octet-stream" ) {
              if ( child )
                dataNode = child;
          } else if ( subType == "pkcs7-mime" ) {
              // note: subtype Pkcs7Mime can also be signed
              //       and we do NOT want to remove the signature!
              if ( child && NodeHelper::instance()->encryptionState( curNode ) != KMMsgNotEncrypted ) {
                dataNode = child;
            }
          }
    } else if ( type == "image" ) {
        kDebug() <<"* image *";
        kDebug() << subType;
    } else if ( type == "audio" ) {
        kDebug() <<"* audio *";
        kDebug() << subType;
    } else if ( type == "video" ) {
        kDebug() << "* video *";
        kDebug() << subType;
    } else {
        kDebug() << type;
        kDebug() << subType;
    }

    KMime::Content* headerContent = 0;
    if ( !dataNode->head().isEmpty() ) {
      headerContent = dataNode;
    }
    if ( weAreReplacingTheRootNode || !dataNode->parent() ) {
      headerContent = &theMessage;
    }
    if( dataNode == curNode ) {
      kDebug() <<"dataNode == curNode:  Save curNode without replacing it.";

      // A) Store the headers of this part IF curNode is not the root node
      //    AND we are not replacing a node that already *has* replaced
      //    the root node in previous recursion steps of this function...
      if ( !headerContent->head().isEmpty() ) {
        if( dataNode->parent() && !weAreReplacingTheRootNode ) {
          kDebug() <<"dataNode is NOT replacing the root node:  Store the headers.";
          resultingData += headerContent->head();
        } else if( weAreReplacingTheRootNode && !dataNode->head().isEmpty() ){
          kDebug() <<"dataNode replace the root node:  Do NOT store the headers but change";
          kDebug() <<"                                 the Message's headers accordingly.";
          kDebug() <<"              old Content-Type =" << theMessage.contentType()->asUnicodeString();
          kDebug() <<"              new Content-Type =" << headerContent->contentType()->asUnicodeString();
          theMessage.contentType()->from7BitString( headerContent->contentType()->as7BitString() );
          theMessage.contentTransferEncoding()->from7BitString(
              headerContent->contentTransferEncoding(false)
            ? headerContent->contentTransferEncoding()->as7BitString()
            : "" );
          theMessage.contentDescription()->from7BitString( headerContent->contentDescription()->as7BitString() );
          theMessage.contentDisposition()->from7BitString( headerContent->contentDisposition()->as7BitString() );
          theMessage.assemble();
        }
      }

      // B) Store the body of this part.
      if( headerContent && bIsMultipart && !dataNode->contents().isEmpty() )  {
        kDebug() <<"is valid Multipart, processing children:";
        QByteArray boundary = headerContent->contentType()->boundary();
        curNode = NodeHelper::firstChild( dataNode );
        // store children of multipart
        while( curNode ) {
          kDebug() <<"--boundary";
          if( resultingData.size() &&
              ( '\n' != resultingData.at( resultingData.size()-1 ) ) )
            resultingData += '\n';
          resultingData += '\n';
          resultingData += "--";
          resultingData += boundary;
          resultingData += '\n';
          // note: We are processing a harmless multipart that is *not*
          //       to be replaced by one of it's children, therefor
          //       we set their doStoreHeaders to true.
          objectTreeToDecryptedMsg( curNode,
                                    resultingData,
                                    theMessage,
                                    false,
                                    recCount + 1 );
          curNode = NodeHelper::nextSibling( curNode );
        }
        kDebug() <<"--boundary--";
        resultingData += "\n--";
        resultingData += boundary;
        resultingData += "--\n\n";
        kDebug() <<"Multipart processing children - DONE";
      } else {
        // store simple part
        kDebug() <<"is Simple part or invalid Multipart, storing body data .. DONE";
        resultingData += dataNode->body();
      }
    } else {
      kDebug() <<"dataNode != curNode:  Replace curNode by dataNode.";
      bool rootNodeReplaceFlag = weAreReplacingTheRootNode || !curNode->parent();
      if( rootNodeReplaceFlag ) {
        kDebug() <<"                      Root node will be replaced.";
      } else {
        kDebug() <<"                      Root node will NOT be replaced.";
      }
      // store special data to replace the current part
      // (e.g. decrypted data or embedded RfC 822 data)
      objectTreeToDecryptedMsg( dataNode,
                                resultingData,
                                theMessage,
                                rootNodeReplaceFlag,
                                recCount + 1 );
    }
  }
  kDebug() << "END" << "(" << recCount << ")";
}


QString MailViewerPrivate::createAtmFileLink( const QString& atmFileName ) const
{
  QFileInfo atmFileInfo( atmFileName );

  KTemporaryFile *linkFile = new KTemporaryFile();
  linkFile->setPrefix( atmFileInfo.fileName() +"_[" );
  linkFile->setSuffix( "]." + KMimeType::extractKnownExtension( atmFileInfo.fileName() ) );
  linkFile->open();
  QString linkName = linkFile->fileName();
  delete linkFile;

  if ( ::link(QFile::encodeName( atmFileName ), QFile::encodeName( linkName )) == 0 ) {
    return linkName; // success
  }
  return QString();
}

KService::Ptr MailViewerPrivate::getServiceOffer( KMime::Content *content)
{
  QString fileName = writeMessagePartToTempFile( content );

  const QString contentTypeStr = content->contentType()->mimeType();

  // determine the MIME type of the attachment
  KMimeType::Ptr mimetype;
  // prefer the value of the Content-Type header
  mimetype = KMimeType::mimeType( contentTypeStr, KMimeType::ResolveAliases );

  if ( !mimetype.isNull() && mimetype->is( KABC::Addressee::mimeType() ) ) {
    slotAtmView( content );
    return KService::Ptr( 0 );
  }

  if ( mimetype.isNull() ) {
    // consider the filename if mimetype can not be found by content-type
    mimetype = KMimeType::findByPath( fileName, 0, true /* no disk access */ );
  }
  if ( ( mimetype->name() == "application/octet-stream" )
    /*TODO(Andris) port when on-demand loading is done   && msgPart.isComplete() */) {
    // consider the attachment's contents if neither the Content-Type header
    // nor the filename give us a clue
    mimetype = KMimeType::findByFileContent( fileName );
  }
  return KMimeTypeTrader::self()->preferredService( mimetype->name(), "Application" );
}

bool MailViewerPrivate::saveContent( KMime::Content* content, const KUrl& url, bool encoded )
{
  KMime::Content *topContent  = content->topLevel();
  bool bSaveEncrypted = false;

  bool bEncryptedParts = NodeHelper::instance()->encryptionState( content ) != KMMsgNotEncrypted;
  if( bEncryptedParts )
    if( KMessageBox::questionYesNo( mMainWindow,
          i18n( "The part %1 of the message is encrypted. Do you want to keep the encryption when saving?",
           url.fileName() ),
          i18n( "KMail Question" ), KGuiItem(i18n("Keep Encryption")), KGuiItem(i18n("Do Not Keep")) ) ==
        KMessageBox::Yes )
      bSaveEncrypted = true;

  bool bSaveWithSig = true;
  if( NodeHelper::instance()->signatureState( content ) != KMMsgNotSigned )
    if( KMessageBox::questionYesNo( mMainWindow,
          i18n( "The part %1 of the message is signed. Do you want to keep the signature when saving?",
           url.fileName() ),
          i18n( "KMail Question" ), KGuiItem(i18n("Keep Signature")), KGuiItem(i18n("Do Not Keep")) ) !=
        KMessageBox::Yes )
      bSaveWithSig = false;

  QByteArray data;
  if ( encoded )
  {
    // This does not decode the Message Content-Transfer-Encoding
    // but saves the _original_ content of the message part
    data = content->encodedContent();
  }
  else
  {
    if( bSaveEncrypted || !bEncryptedParts) {
      KMime::Content *dataNode = content;
      QByteArray rawReplyString;
      bool gotRawReplyString = false;
      if ( !bSaveWithSig ) {
        if ( topContent->contentType()->mimeType() == "multipart/signed" )  {
          // carefully look for the part that is *not* the signature part:
          if ( ObjectTreeParser::findType( topContent, "application/pgp-signature", true, false ) ) {
            dataNode = ObjectTreeParser::findTypeNot( topContent, "application", "pgp-signature", true, false );
          } else if ( ObjectTreeParser::findType( topContent, "application/pkcs7-mime" , true, false ) ) {
            dataNode = ObjectTreeParser::findTypeNot( topContent, "application", "pkcs7-mime", true, false );
          } else {
            dataNode = ObjectTreeParser::findTypeNot( topContent, "multipart", "", true, false );
          }
        } else {
          ObjectTreeParser otp( 0, 0, false, false, false );

          // process this node and all it's siblings and descendants
          NodeHelper::instance()->setNodeUnprocessed( dataNode, true );
          otp.parseObjectTree( dataNode );

          rawReplyString = otp.rawReplyString();
          gotRawReplyString = true;
        }
      }
      QByteArray cstr = gotRawReplyString
                         ? rawReplyString
                         : dataNode->decodedContent();
      data = KMime::CRLFtoLF( cstr );
    }
  }
  QDataStream ds;
  QFile file;
  KTemporaryFile tf;
  if ( url.isLocalFile() )
  {
    // save directly
    file.setFileName( url.toLocalFile() );
    if ( !file.open( QIODevice::WriteOnly ) )
    {
      KMessageBox::error( mMainWindow,
                          i18nc( "1 = file name, 2 = error string",
                                 "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                                 file.fileName(),
                                 QString::fromLocal8Bit( strerror( errno ) ) ),
                          i18n( "Error saving attachment" ) );
      return false;
    }

/*FIXME(Andras) port it
    // #79685 by default use the umask the user defined, but let it be configurable
    if ( GlobalSettings::self()->disregardUmask() )
      fchmod( file.handle(), S_IRUSR | S_IWUSR );
*/
    ds.setDevice( &file );
  } else
  {
    // tmp file for upload
    tf.open();
    ds.setDevice( &tf );
  }

  if ( ds.writeRawData( data.data(), data.size() ) == -1)
  {
    QFile *f = static_cast<QFile *>( ds.device() );
    KMessageBox::error( mMainWindow,
                        i18nc( "1 = file name, 2 = error string",
                               "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                               f->fileName(),
                               f->errorString() ),
                        i18n( "Error saving attachment" ) );
    return false;
  }

  if ( !url.isLocalFile() )
  {
    // QTemporaryFile::fileName() is only defined while the file is open
    QString tfName = tf.fileName();
    tf.close();
    if ( !KIO::NetAccess::upload( tfName, url, mMainWindow ) )
    {
      KMessageBox::error( mMainWindow,
                          i18nc( "1 = file name, 2 = error string",
                                 "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                                 url.prettyUrl(),
                                 KIO::NetAccess::lastErrorString() ),
                          i18n( "Error saving attachment" ) );
      return false;
    }
  } else
    file.close();
  return true;
}


void MailViewerPrivate::saveAttachments( const KMime::Content::List & contents )
{
  KUrl url, dirUrl;
  if ( contents.size() > 1 ) {
    dirUrl = KFileDialog::getExistingDirectoryUrl( KUrl( "kfiledialog:///saveAttachment" ),
                                                   mMainWindow,
                                                   i18n( "Save Attachments To" ) );
    if ( !dirUrl.isValid() ) {
      return;
    }
    // we may not get a slash-terminated url out of KFileDialog
    dirUrl.adjustPath( KUrl::AddTrailingSlash );
  } else {
    // only one item, get the desired filename
    KMime::Content *content = contents[0];
    // replace all ':' with '_' because ':' isn't allowed on FAT volumes
    QString s = content->contentDisposition()->filename().trimmed().replace( ':', '_' );
    if ( s.isEmpty() )
      s = content->contentType()->name().trimmed().replace( ':', '_' );
    if ( s.isEmpty() )
      s = i18nc("filename for an unnamed attachment", "attachment.1");
    url = KFileDialog::getSaveUrl( KUrl( "kfiledialog:///saveAttachment/" + s ),
                                   QString(),
                                   mMainWindow,
                                   i18n( "Save Attachment" ) );
    if ( url.isEmpty() ) {
      return;
    }
  }

  QMap< QString, int > renameNumbering;

  int unnamedAtmCount = 0;
  bool overwriteAll = false;
  for ( KMime::Content::List::const_iterator it = contents.constBegin();
        it != contents.constEnd();
        ++it ) {
    KMime::Content *content = *it;
    KUrl curUrl;
    if ( !dirUrl.isEmpty() ) {
      curUrl = dirUrl;
      QString s = content->contentDisposition()->filename();
      if ( s.isEmpty() )
        s = content->contentType()->name().trimmed().replace( ':', '_' );
      if ( s.isEmpty() ) {
        ++unnamedAtmCount;
        s = i18nc("filename for the %1-th unnamed attachment",
                 "attachment.%1",
              unnamedAtmCount );
      }
      curUrl.setFileName( s );
    } else {
      curUrl = url;
    }

    if ( !curUrl.isEmpty() ) {

     // Rename the file if we have already saved one with the same name:
     // try appending a number before extension (e.g. "pic.jpg" => "pic_2.jpg")
     QString origFile = curUrl.fileName();
     QString file = origFile;

     while ( renameNumbering.contains(file) ) {
       file = origFile;
       int num = renameNumbering[file] + 1;
       int dotIdx = file.lastIndexOf('.');
       file = file.insert( (dotIdx>=0) ? dotIdx : file.length(), QString("_") + QString::number(num) );
     }
     curUrl.setFileName(file);

     // Increment the counter for both the old and the new filename
     if ( !renameNumbering.contains(origFile))
         renameNumbering[origFile] = 1;
     else
         renameNumbering[origFile]++;

     if ( file != origFile ) {
        if ( !renameNumbering.contains(file))
            renameNumbering[file] = 1;
        else
            renameNumbering[file]++;
     }


      if ( !overwriteAll && KIO::NetAccess::exists( curUrl, KIO::NetAccess::DestinationSide, mMainWindow ) ) {
        if ( contents.count() == 1 ) {
          if ( KMessageBox::warningContinueCancel( mMainWindow,
                i18n( "A file named <br><filename>%1</filename><br>already exists.<br><br>Do you want to overwrite it?",
                  curUrl.fileName() ),
                i18n( "File Already Exists" ), KGuiItem(i18n("&Overwrite")) ) == KMessageBox::Cancel) {
            continue;
          }
        }
        else {
          int button = KMessageBox::warningYesNoCancel(
                mMainWindow,
                i18n( "A file named <br><filename>%1</filename><br>already exists.<br><br>Do you want to overwrite it?",
                  curUrl.fileName() ),
                i18n( "File Already Exists" ), KGuiItem(i18n("&Overwrite")),
                KGuiItem(i18n("Overwrite &All")) );
          if ( button == KMessageBox::Cancel )
            continue;
          else if ( button == KMessageBox::No )
            overwriteAll = true;
        }
      }
      // save
      saveContent( content, curUrl, false );
    }
  }
}

KMime::Content::List MailViewerPrivate::allContents( const KMime::Content * content )
{
  KMime::Content::List result;
  KMime::Content *child = NodeHelper::firstChild( content );
  if ( child ) {
    result += child;
    result += allContents( child );
  }
  KMime::Content *next = NodeHelper::nextSibling( content );
  if ( next ) {
    result += next;
    result += allContents( next );
  }

  return result;
}


KMime::Content::List MailViewerPrivate::selectedContents()
{
  KMime::Content::List contents;
  QItemSelectionModel *selectionModel = mMimePartTree->selectionModel();
  QModelIndexList selectedRows = selectionModel->selectedRows();

  Q_FOREACH(QModelIndex index, selectedRows)
  {
     KMime::Content *content = static_cast<KMime::Content*>( index.internalPointer() );
     if ( content )
       contents.append( content );
  }

  return contents;
}


void MailViewerPrivate::attachmentOpenWith( KMime::Content *node )
{
  QString name = writeMessagePartToTempFile( node );
  QString linkName = createAtmFileLink( name );
  KUrl::List lst;
  KUrl url;
  bool autoDelete = true;

  if ( linkName.isEmpty() ) {
  autoDelete = false;
  linkName = name;
  }

  url.setPath( linkName );
  lst.append( url );
  if ( (! KRun::displayOpenWithDialog(lst, mMainWindow, autoDelete)) && autoDelete ) {
    QFile::remove( url.toLocalFile() );
  }
}

void MailViewerPrivate::attachmentOpen( KMime::Content *node )
{
  KService::Ptr offer(0);
  offer = getServiceOffer( node );
  if ( !offer ) {
    kDebug() << "got no offer";
    return;
  }
  QString name = writeMessagePartToTempFile( node );
  KUrl::List lst;
  KUrl url;
  bool autoDelete = true;
  QString fname = createAtmFileLink( name );

  if ( fname.isNull() ) {
    autoDelete = false;
    fname = name;
  }

  url.setPath( fname );
  lst.append( url );
  if ( (!KRun::run( *offer, lst, 0, autoDelete )) && autoDelete ) {
      QFile::remove(url.toLocalFile());
  }
}


CSSHelper* MailViewerPrivate::cssHelper() const
{
  return mCSSHelper;
}

bool MailViewerPrivate::decryptMessage() const
{
  if ( !GlobalSettings::self()->alwaysDecrypt() )
    return mDecrytMessageOverwrite;
}


void MailViewerPrivate::setStyleDependantFrameWidth()
{
  if ( !mBox )
    return;
  // set the width of the frame to a reasonable value for the current GUI style
  int frameWidth;
#if 0 // is this hack still needed with kde4?
  if( !qstrcmp( style()->metaObject()->className(), "KeramikStyle" ) )
    frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth ) - 1;
  else
#endif
    frameWidth = q->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  if ( frameWidth < 0 )
    frameWidth = 0;
  if ( frameWidth != mBox->lineWidth() )
    mBox->setLineWidth( frameWidth );
}


int MailViewerPrivate::pointsToPixel(int pointSize) const
{
  return (pointSize * mViewer->view()->logicalDpiY() + 36) / 72;
}

void MailViewerPrivate::displaySplashPage( const QString &info )
{
  mMsgDisplay = false;
  adjustLayout();

  QString location = KStandardDirs::locate("data", "kmail/about/main.html");//FIXME(Andras) copy to $KDEDIR/share/apps/mailviewer
  QString content = KPIMUtils::kFileToByteArray( location );
  content = content.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
  if ( QApplication::isRightToLeft() )
    content = content.arg( "@import \"" + KStandardDirs::locate( "data",
                           "kdeui/about/kde_infopage_rtl.css" ) +  "\";");
  else
    content = content.arg( "" );

  mViewer->begin(KUrl::fromPath( location ));

  QString fontSize = QString::number( pointsToPixel( mCSSHelper->bodyFont().pointSize() ) );
  QString appTitle = i18n("Mailreader");
  QString catchPhrase = ""; //not enough space for a catch phrase at default window size i18n("Part of the Kontact Suite");
  QString quickDescription = i18n("The email client for the K Desktop Environment.");
  mViewer->write(content.arg(fontSize).arg(appTitle).arg(catchPhrase).arg(quickDescription).arg(info));
  mViewer->end();
}

void MailViewerPrivate::enableMessageDisplay()
{
  mMsgDisplay = true;
  adjustLayout();
}



void MailViewerPrivate::displayMessage()
{

  /*FIXME(Andras) port to Akonadi
  mMimePartTree->clearAndResetSortOrder();
  */
  mMimePartModel->setRoot( mMessage );
  mIsPlainText = !mMessage || mMessage->contentType()->isPlainText();
  showHideMimeTree();

  NodeHelper::instance()->setOverrideCodec( mMessage, overrideCodec() );

  htmlWriter()->begin( mCSSHelper->cssDefinitions( mUseFixedFont ) );
  htmlWriter()->queue( mCSSHelper->htmlHead( mUseFixedFont ) );

  if ( !mMainWindow )
      q->setWindowTitle( mMessage->subject()->asUnicodeString() );

  removeTempFiles();

  mColorBar->setNeutralMode();

  parseMsg();

  if( mColorBar->isNeutral() )
    mColorBar->setNormalMode();

  htmlWriter()->queue("</body></html>");
  htmlWriter()->flush();

  QTimer::singleShot( 1, this, SLOT(injectAttachments()) );
}


void MailViewerPrivate::parseMsg()
{
  assert( mMessage != 0 );

  /* aMsg->setIsBeingParsed( true ); //FIXME(Andras) review and port */
  NodeHelper::instance()->setNodeBeingProcessed( mMessage, true );

  QString cntDesc = i18n("( body part )");

  if ( mMessage->subject( false ) )
      cntDesc = mMessage->subject()->asUnicodeString();

  KIO::filesize_t cntSize = mMessage->size();

  QString cntEnc= "7bit";
  if ( mMessage->contentTransferEncoding( false ) )
      cntEnc = mMessage->contentTransferEncoding()->asUnicodeString();

// Check if any part of this message is a v-card
// v-cards can be either text/x-vcard or text/directory, so we need to check
// both.
  KMime::Content* vCardContent = findContentByType( mMessage, "text/x-vcard" );
  if ( !vCardContent )
      vCardContent = findContentByType( mMessage, "text/directory" );

  bool hasVCard = false;

  if( vCardContent ) {
  // ### FIXME: We should only do this if the vCard belongs to the sender,
  // ### i.e. if the sender's email address is contained in the vCard.
      const QByteArray vCard = vCardContent->decodedContent();
      KABC::VCardConverter t;
      if ( !t.parseVCards( vCard ).isEmpty() ) {
          hasVCard = true;
          kDebug() <<"FOUND A VALID VCARD";
          writeMessagePartToTempFile( vCardContent );
      }
  }
  htmlWriter()->queue( writeMsgHeader( mMessage, hasVCard ? vCardContent : 0, true ) );

  // show message content
  ObjectTreeParser otp( this );
  otp.setAllowAsync( true );
  otp.parseObjectTree( mMessage );

  bool emitReplaceMsgByUnencryptedVersion = false;

  // store encrypted/signed status information in the KMMessage
  //  - this can only be done *after* calling parseObjectTree()
  KMMsgEncryptionState encryptionState = NodeHelper::instance()->overallEncryptionState( mMessage );
  KMMsgSignatureState  signatureState  = NodeHelper::instance()->overallSignatureState( mMessage );
  NodeHelper::instance()->setEncryptionState( mMessage, encryptionState );
  // Don't reset the signature state to "not signed" (e.g. if one canceled the
  // decryption of a signed messages which has already been decrypted before).
  if ( signatureState != KMMsgNotSigned ||
       NodeHelper::instance()->signatureState( mMessage ) == KMMsgSignatureStateUnknown ) {
    NodeHelper::instance()->setSignatureState( mMessage, signatureState );
  }

  const KConfigGroup reader( Global::instance()->config(), "Reader" );
  if ( reader.readEntry( "store-displayed-messages-unencrypted", false ) ) {

  // Hack to make sure the S/MIME CryptPlugs follows the strict requirement
  // of german government:
  // --> All received encrypted messages *must* be stored in unencrypted form
  //     after they have been decrypted once the user has read them.
  //     ( "Aufhebung der Verschluesselung nach dem Lesen" )
  //
  // note: Since there is no configuration option for this, we do that for
  //       all kinds of encryption now - *not* just for S/MIME.
  //       This could be changed in the objectTreeToDecryptedMsg() function
  //       by deciding when (or when not, resp.) to set the 'dataNode' to
  //       something different than 'curNode'.


kDebug() <<"\n\n\nSpecial post-encryption handling:\n1.";
//FIXME(Andras) do we need it? kDebug() <<"(aMsg == msg) ="                      << (aMsg == message());
kDebug() <<"   mLastStatus.isOfUnknownStatus() =" << mLastStatus.isOfUnknownStatus();
kDebug() <<"|| mLastStatus.isNew() ="             << mLastStatus.isNew();
kDebug() <<"|| mLastStatus.isUnread) ="           << mLastStatus.isUnread();
//FIXME(Andras) kDebug() <<"(mIdOfLastViewedMessage != aMsg->msgId()) ="       << (mIdOfLastViewedMessage != aMsg->msgId());
kDebug() <<"   (KMMsgFullyEncrypted == encryptionState) ="     << (KMMsgFullyEncrypted == encryptionState);
kDebug() <<"|| (KMMsgPartiallyEncrypted == encryptionState) =" << (KMMsgPartiallyEncrypted == encryptionState);
         // only proceed if we were called the normal way - not by
         // double click on the message (==not running in a separate window)
  if(    (/*aMsg == message()*/ true) //TODO(Andras) review if still needed
         // only proceed if this message was not saved encryptedly before
         // to make sure only *new* messages are saved in decrypted form
      && (    mLastStatus.isOfUnknownStatus()
           || mLastStatus.isNew()
           || mLastStatus.isUnread() )
         // avoid endless recursions
//FIXME(Andras)      && (mIdOfLastViewedMessage != aMsg->msgId())
         // only proceed if this message is (at least partially) encrypted
      && (    (KMMsgFullyEncrypted == encryptionState)
           || (KMMsgPartiallyEncrypted == encryptionState) ) ) {

    kDebug() <<"Calling objectTreeToDecryptedMsg()";

    KMime::Message *unencryptedMessage = new KMime::Message;
    QByteArray decryptedData;
    // note: The following call may change the message's headers.
    objectTreeToDecryptedMsg( mMessage, decryptedData, *unencryptedMessage );
    kDebug() << "Resulting data:" << decryptedData;

    if( !decryptedData.isEmpty() ) {
      kDebug() <<"Composing unencrypted message";
      unencryptedMessage->setBody( decryptedData );
     //FIXME(Andras) fix it? kDebug() << "Resulting message:" << unencryptedMessage->asString();
      kDebug() << "Attach unencrypted message to aMsg";

      NodeHelper::instance()->attachUnencryptedMessage( mMessage, unencryptedMessage );

      emitReplaceMsgByUnencryptedVersion = true;
    }
    }
  }

  // store message id to avoid endless recursions
/*FIXME(Andras) port it
  setIdOfLastViewedMessage( aMsg->index().toString() );
*/
  if( emitReplaceMsgByUnencryptedVersion ) {
    kDebug() << "Invoce saving in decrypted form:";
    emit replaceMsgByUnencryptedVersion(); //FIXME(Andras) actually connect and do the replacement on the server (see KMMainWidget::slotReplaceByUnencryptedVersion)
  } else {
    mIsPlainText = mMessage->contentType()->isPlainText();
    showHideMimeTree();
  }
/* FIXME(Andras) port it!
  aMsg->setIsBeingParsed( false );
  */
  NodeHelper::instance()->setNodeBeingProcessed( mMessage, false );
}


QString MailViewerPrivate::writeMsgHeader(KMime::Message* aMsg, KMime::Content* vCardNode, bool topLevel)
{
  kFatal( !headerStyle(), 5006 )
    << "trying to writeMsgHeader() without a header style set!";
  kFatal( !headerStrategy(), 5006 )
    << "trying to writeMsgHeader() without a header strategy set!";
  QString href;
  if ( vCardNode )
    href = NodeHelper::asHREF( vCardNode, "body" );

  return headerStyle()->format( aMsg, headerStrategy(), href, mPrinting, topLevel );
}


QString MailViewerPrivate::writeMessagePartToTempFile(KMime::Content* aMsgPart)
{
  // If the message part is already written to a file, no point in doing it again.
  // This function is called twice actually, once from the rendering of the attachment
  // in the body and once for the header.
  KUrl existingFileName = tempFileUrlFromNode( aMsgPart );
  if ( !existingFileName.isEmpty() ) {
    return existingFileName.toLocalFile();
  }

  QString fileName = NodeHelper::fileName( aMsgPart );

  QString fname = createTempDir( aMsgPart->index().toString() );
  if ( fname.isEmpty() )
    return QString();

  // strip off a leading path
  int slashPos = fileName.lastIndexOf( '/' );
  if( -1 != slashPos )
    fileName = fileName.mid( slashPos + 1 );
  if( fileName.isEmpty() )
    fileName = "unnamed";
  fname += '/' + fileName;

  kDebug() << "Create temp file: " << fname;

  QByteArray data = aMsgPart->decodedContent();
  if ( aMsgPart->contentType()->isText() && data.size() > 0 ) {
    // convert CRLF to LF before writing text attachments to disk
    data = KMime::CRLFtoLF( data );
  }
  if( !KPIMUtils::kByteArrayToFile( data, fname, false, false, false ) )
    return QString();

  mTempFiles.append( fname );
  // make file read-only so that nobody gets the impression that he might
  // edit attached files (cf. bug #52813)
  ::chmod( QFile::encodeName( fname ), S_IRUSR );

  return fname;
}


QString MailViewerPrivate::createTempDir( const QString &param )
{
  KTemporaryFile *tempFile = new KTemporaryFile();
  tempFile->setSuffix( ".index." + param );
  tempFile->open();
  QString fname = tempFile->fileName();
  delete tempFile;

  if ( ::access( QFile::encodeName( fname ), W_OK ) != 0 ) {
    // Not there or not writable
    if( KDE_mkdir( QFile::encodeName( fname ), 0 ) != 0 ||
        ::chmod( QFile::encodeName( fname ), S_IRWXU ) != 0 ) {
      return QString(); //failed create
    }
  }

  assert( !fname.isNull() );

  mTempDirs.append( fname );
  return fname;
}


void MailViewerPrivate::showVCard( KMime::Content* msgPart ) {
  const QByteArray vCard = msgPart->decodedContent();

  VCardViewer *vcv = new VCardViewer( mMainWindow, vCard );
  vcv->setObjectName( "vCardDialog" );
  vcv->show();
}


void MailViewerPrivate::initHtmlWidget(void)
{
  mViewer->widget()->setFocusPolicy(Qt::WheelFocus);
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->setPluginsEnabled(false);
  mViewer->setJScriptEnabled(false); // just make this explicit
  mViewer->setJavaEnabled(false);    // just make this explicit
  mViewer->setMetaRefreshEnabled(false);
  mViewer->setURLCursor( QCursor( Qt::PointingHandCursor ) );
  // Espen 2000-05-14: Getting rid of thick ugly frames
  mViewer->view()->setLineWidth(0);
  // register our own event filter for shift-click
  mViewer->view()->viewport()->installEventFilter( this );

  if ( !htmlWriter() ) {
    mPartHtmlWriter = new KHtmlPartHtmlWriter( mViewer, 0 );
#ifdef KMAIL_READER_HTML_DEBUG
    mHtmlWriter = new TeeHtmlWriter( new FileHtmlWriter( QString() ),
                                     mPartHtmlWriter );
#else
    mHtmlWriter = mPartHtmlWriter;
#endif
  }

  // We do a queued connection below, and for that we need to register the meta types of the
  // parameters.
  //
  // Why do we do a queued connection instead of a direct one? slotUrlOpen() handles those clicks,
  // and can end up in the click handler for accepting invitations. That handler can pop up a dialog
  // asking the user for a comment on the invitation reply. This dialog is started with exec(), i.e.
  // executes a sub-eventloop. This sub-eventloop then eventually re-enters the KHTML event handler,
  // which then thinks we started a drag, and therefore adds a silly drag object to the cursor, with
  // urls like x-kmail-whatever/43/8/accept, and we don't want that drag object.
  //
  // Therefore, use queued connections to avoid the reentry of the KHTML event loop, so we don't
  // get the drag object.
  static bool metaTypesRegistered = false;
  if ( !metaTypesRegistered ) {
    qRegisterMetaType<KParts::OpenUrlArguments>( "KParts::OpenUrlArguments" );
    qRegisterMetaType<KParts::BrowserArguments>( "KParts::BrowserArguments" );
    metaTypesRegistered = true;
  }

  connect(mViewer->browserExtension(),
          SIGNAL(openUrlRequest(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),this,
          SLOT(slotUrlOpen(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),
          Qt::QueuedConnection);
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),this,
          SLOT(slotUrlOpen(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),
          Qt::QueuedConnection);
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotUrlPopup(const QString &, const QPoint &)));
}


void MailViewerPrivate::removeTempFiles()
{
  for (QStringList::Iterator it = mTempFiles.begin(); it != mTempFiles.end();
    ++it)
  {
    QFile::remove(*it);
  }
  mTempFiles.clear();
  for (QStringList::Iterator it = mTempDirs.begin(); it != mTempDirs.end();
    it++)
  {
    QDir(*it).rmdir(*it);
  }
  mTempDirs.clear();
}



bool MailViewerPrivate::eventFilter( QObject *, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress ) {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if ( me->button() == Qt::LeftButton && ( me->modifiers() & Qt::ShiftModifier ) ) {
      // special processing for shift+click
      KMime::Content *node = nodeFromUrl( mUrlClicked );
      if ( node ) {
        saveAttachments( KMime::Content::List() << node);
      }
      return true; // eat event
    }
  }
  // standard event processing
  return false;
}


void MailViewerPrivate::readConfig()
{
  const KConfigGroup mdnGroup(Global::instance()->config(), "MDN");
  KConfigGroup reader(Global::instance()->config(), "Reader");

  delete mCSSHelper;
  mCSSHelper = new CSSHelper( mViewer->view() );

  mNoMDNsWhenEncrypted = mdnGroup.readEntry( "not-send-when-encrypted", true );

  mUseFixedFont = reader.readEntry( "useFixedFont", false );
  if ( mToggleFixFontAction )
    mToggleFixFontAction->setChecked( mUseFixedFont );

  mHtmlMail = reader.readEntry( "htmlMail", false );
  mHtmlLoadExternal = reader.readEntry( "htmlLoadExternal", false );

  KToggleAction *raction = actionForHeaderStyle( headerStyle(), headerStrategy() );
  if ( raction )
    raction->setChecked( true );

  setAttachmentStrategy( AttachmentStrategy::create( reader.readEntry( "attachment-strategy", "smart" ) ) );
  raction = actionForAttachmentStrategy( attachmentStrategy() );
  if ( raction )
    raction->setChecked( true );

  const int mimeH = reader.readEntry( "MimePaneHeight", 100 );
  const int messageH = reader.readEntry( "MessagePaneHeight", 180 );
  mSplitterSizes.clear();
  if ( GlobalSettings::self()->mimeTreeLocation() == GlobalSettings::EnumMimeTreeLocation::bottom )
    mSplitterSizes << messageH << mimeH;
  else
    mSplitterSizes << mimeH << messageH;

  adjustLayout();

  readGlobalOverrideCodec();

  // Note that this call triggers an update, see this call has to be at the
  // bottom when all settings are already est.
  setHeaderStyleAndStrategy( HeaderStyle::create( reader.readEntry( "header-style", "fancy" ) ),
                             HeaderStrategy::create( reader.readEntry( "header-set-displayed", "rich" ) ) );

  if ( mMessage )
    update();
  mColorBar->update();
 /*FIXME(Andras)
  KMMessage::readConfig();
  */
}


void MailViewerPrivate::writeConfig( bool sync ) const
{
  KConfigGroup reader( Global::instance()->config() , "Reader" );

  reader.writeEntry( "useFixedFont", mUseFixedFont );
  if ( headerStyle() )
    reader.writeEntry( "header-style", headerStyle()->name() );
  if ( headerStrategy() )
    reader.writeEntry( "header-set-displayed", headerStrategy()->name() );
  if ( attachmentStrategy() )
    reader.writeEntry( "attachment-strategy", attachmentStrategy()->name() );

  saveSplitterSizes( reader );
/*FIXME(Andras) port to akonadi
  if ( sync )
    kmkernel->slotRequestConfigSync();
  */
}


void MailViewerPrivate::setHeaderStyleAndStrategy( const HeaderStyle * style,
                                             const HeaderStrategy * strategy ) {
  mHeaderStyle = style ? style : HeaderStyle::fancy();
  mHeaderStrategy = strategy ? strategy : HeaderStrategy::rich();
  update( MailViewer::Force );
}


void MailViewerPrivate::setAttachmentStrategy( const AttachmentStrategy * strategy ) {
  mAttachmentStrategy = strategy ? strategy : AttachmentStrategy::smart();
  update( MailViewer::Force );
}


void MailViewerPrivate::setOverrideEncoding( const QString & encoding )
{
  if ( encoding == mOverrideEncoding )
    return;

  mOverrideEncoding = encoding;
  if ( mSelectEncodingAction ) {
    if ( encoding.isEmpty() ) {
      mSelectEncodingAction->setCurrentItem( 0 );
    }
    else {
      QStringList encodings = mSelectEncodingAction->items();
      int i = 0;
      for ( QStringList::const_iterator it = encodings.constBegin(), end = encodings.constEnd(); it != end; ++it, ++i ) {
        if ( MailViewerPrivate::encodingForName( *it ) == encoding ) {
          mSelectEncodingAction->setCurrentItem( i );
          break;
        }
      }
      if ( i == encodings.size() ) {
        // the value of encoding is unknown => use Auto
        kWarning() <<"Unknown override character encoding \"" << encoding
                       << "\". Using Auto instead.";
        mSelectEncodingAction->setCurrentItem( 0 );
        mOverrideEncoding.clear();
      }
    }
  }
  update( MailViewer::Force );
}


void MailViewerPrivate::setPrintFont( const QFont& font )
{

  mCSSHelper->setPrintFont( font );
}


void MailViewerPrivate::printMessage( KMime::Message* message )
{
  disconnect( mPartHtmlWriter, SIGNAL( finished() ), this, SLOT( slotPrintMsg() ) );
  connect( mPartHtmlWriter, SIGNAL( finished() ), this, SLOT( slotPrintMsg() ) );
  setMessage( message, MailViewer::Force );
}


void MailViewerPrivate::setMessageItem( const Akonadi::Item &item,  MailViewer::UpdateMode updateMode )
{
  if ( mMessage && !NodeHelper::instance()->nodeProcessed( mMessage ) ) {
    kWarning() << "The root node is not yet processed! Danger!";
    return;
  }

  if ( mMessage && NodeHelper::instance()->nodeBeingProcessed( mMessage ) ) {
    kWarning() << "The root node is not yet fully processed! Danger!";
    return;
  }

    if ( mDeleteMessage ) {
      kDebug() << "DELETE " << mMessage;
      delete mMessage;
      mMessage = 0;
    }
    NodeHelper::instance()->clear();
    mMimePartModel->setRoot( 0 );

    mMessage = 0; //forget the old message if it was set
    mMessageItem = item;

    if ( !mMessageItem.hasPayload<KMime::Message::Ptr>() ) {
      kWarning() << "Payload is not a MessagePtr!";
      return;
    }
    //Note: if I use MessagePtr for mMessage all over, I get a crash in the destructor
    mMessage = new KMime::Message;
    kDebug() << "START SHOWING" << mMessage;
  /*
  mMessage->setContent(mMessageItem.payloadData());
  mMessage->parse();*/

    mMessage ->setContent( mMessageItem.payloadData() );
    mMessage ->parse();
    mDeleteMessage = true;


    update( updateMode );
    kDebug() << "SHOWN" << mMessage;

}

void MailViewerPrivate::setMessage(KMime::Message* aMsg, MailViewer::UpdateMode updateMode, MailViewer::Ownership ownerShip)
{
  if ( mDeleteMessage ) {
    delete mMessage;
    mMessage = 0;
  }
  NodeHelper::instance()->clear();
  mMimePartModel->setRoot( 0 );



  if ( aMsg ) {
  /*FIXME(Andras) port to akonadi
      kDebug() <<"(" << aMsg->getMsgSerNum() <<", last" << mLastSerNum <<")" << aMsg->subject()
        << aMsg->fromStrip() << ", readyToShow" << (aMsg->readyToShow());

// Reset message-transient state
  if (aMsg && aMsg->getMsgSerNum() != mLastSerNum ){
    mLevelQuote = GlobalSettings::self()->collapseQuoteLevelSpin()-1;
    clearBodyPartMementos();
  }
  */
  }
  if ( mPrinting )
    mLevelQuote = -1;

  bool complete = true;
  /*FIXME(Andras) port to akonadi
  if ( aMsg &&
       !aMsg->readyToShow() &&
       (aMsg->getMsgSerNum() != mLastSerNum) &&
       !aMsg->isComplete() )
    complete = false;

  // If not forced and there is aMsg and aMsg is same as mMsg then return
  if (!force && aMsg && mLastSerNum != 0 && aMsg->getMsgSerNum() == mLastSerNum)
    return;


  // (de)register as observer
  if (aMsg && message())
    message()->detach( this );
  if (aMsg)
    aMsg->attach( this );
  */

  // connect to the updates if we have hancy headers

  mDelayedMarkTimer.stop();

  if ( !aMsg ) {
    mWaitingForSerNum = 0; // otherwise it has been set
    mLastSerNum = 0;
  } else {
  /*FIXME(Andras) port to akonadi
    mLastSerNum = aMsg->getMsgSerNum();
    // Check if the serial number can be used to find the assoc KMMessage
    // If so, keep only the serial number (and not mMessage), to avoid a dangling mMessage
    // when going to another message in the mainwindow.
    // Otherwise, keep only mMessage, this is fine for standalone KMReaderMainWins since
    // we're working on a copy of the KMMessage, which we own.
    */
    mMessage = aMsg;
    mDeleteMessage = (ownerShip == MailViewer::Transfer);
    mLastSerNum = 0;
  }

  if ( mMessage ) {
    NodeHelper::instance()->setOverrideCodec( mMessage, overrideCodec() );
  /*FIXME(Andras) port to akonadi
    aMsg->setDecodeHTML( htmlMail() );
    mLastStatus = aMsg->status();
    // FIXME: workaround to disable DND for IMAP load-on-demand
    if ( !aMsg->isComplete() )
      mViewer->setDNDEnabled( false );
    else
      mViewer->setDNDEnabled( true );
  */
  } else {
    mLastStatus.clear();
  }

  // only display the msg if it is complete
  // otherwise we'll get flickering with progressively loaded messages
  if ( complete )
  {
    update( updateMode );
  }

  /*FIXME(Andras) port to akonadi
  if ( aMsg && (aMsg->status().isUnread() || aMsg->status().isNew())
       && GlobalSettings::self()->delayedMarkAsRead() ) {
    if ( GlobalSettings::self()->delayedMarkTime() != 0 )
      mDelayedMarkTimer.start( GlobalSettings::self()->delayedMarkTime() * 1000 );
    else
      slotTouchMessage();
  }
  */
}

void MailViewerPrivate::setMessagePart( KMime::Content * node )
{
  htmlWriter()->reset();
  mColorBar->hide();
  htmlWriter()->begin( mCSSHelper->cssDefinitions( mUseFixedFont ) );
  htmlWriter()->write( mCSSHelper->htmlHead( mUseFixedFont ) );
  // end ###
  if ( node ) {
    ObjectTreeParser otp( this, 0, true );
    otp.parseObjectTree( node );
  }
  // ### this, too
  htmlWriter()->queue( "</body></html>" );
  htmlWriter()->flush();
}


void MailViewerPrivate::setMessagePart( KMime::Content* aMsgPart, bool aHTML,
                              const QString& aFileName, const QString& pname )
{
  // Cancel scheduled updates of the reader window, as that would stop the
  // timer of the HTML writer, which would make viewing attachment not work
  // anymore as not all HTML is written to the HTML part.
  // We're updating the reader window here ourselves anyway.
  mUpdateReaderWinTimer.stop();

  KCursorSaver busy(KBusyPtr::busy());
  if ( aMsgPart->contentType()->mediaType() == "message" ) {
      // if called from compose win

      KMime::Message* msg = new KMime::Message;
      assert(aMsgPart!=0);
      msg->setContent(aMsgPart->decodedContent());
      msg->parse();
      mMainWindow->setWindowTitle( msg->subject()->asUnicodeString() );
      setMessage( msg, MailViewer::Force );
      mDeleteMessage = true;
  } else if ( aMsgPart->contentType()->mediaType() == "text" ) {
      if ( aMsgPart->contentType()->subType() == "x-vcard" ||
          aMsgPart->contentType()->subType() == "directory" ) {
        showVCard( aMsgPart );
        return;
      }
      htmlWriter()->begin( mCSSHelper->cssDefinitions( mUseFixedFont ) );
      htmlWriter()->queue( mCSSHelper->htmlHead( mUseFixedFont ) );

      if (aHTML && aMsgPart->contentType()->subType() == "html" ) { // HTML
        // ### this is broken. It doesn't stip off the HTML header and footer!
        htmlWriter()->queue( overrideCodec()? overrideCodec()->toUnicode(aMsgPart->decodedContent() ) : aMsgPart->decodedText() );
        mColorBar->setHtmlMode();
      } else { // plain text
        const QByteArray str = aMsgPart->decodedContent();
        ObjectTreeParser otp( this );
        otp.writeBodyStr( str,
                          overrideCodec() ? overrideCodec() : NodeHelper::instance()->codec( aMsgPart ),
                          mMessage ? mMessage->from()->asUnicodeString() : QString() );
      }
      htmlWriter()->queue("</body></html>");
      htmlWriter()->flush();
      mMainWindow->setWindowTitle(i18n("View Attachment: %1", pname));
  } else if (aMsgPart->contentType()->mediaType() == "image"  ||
             aMsgPart->contentType()->mimeType() ==  "application/postscript" )
  {
      if (aFileName.isEmpty()) return;  // prevent crash
      // Open the window with a size so the image fits in (if possible):
      QImageReader *iio = new QImageReader();
      iio->setFileName(aFileName);
      if( iio->canRead() ) {
          QImage img = iio->read();
          QRect desk = KGlobalSettings::desktopGeometry(mMainWindow);
          // determine a reasonable window size
          int width, height;
          if( img.width() < 50 )
              width = 70;
          else if( img.width()+20 < desk.width() )
              width = img.width()+20;
          else
              width = desk.width();
          if( img.height() < 50 )
              height = 70;
          else if( img.height()+20 < desk.height() )
              height = img.height()+20;
          else
              height = desk.height();
          mMainWindow->resize( width, height );
      }
      // Just write the img tag to HTML:
      htmlWriter()->begin( mCSSHelper->cssDefinitions( mUseFixedFont ) );
      htmlWriter()->write( mCSSHelper->htmlHead( mUseFixedFont ) );
      htmlWriter()->write( "<img src=\"file:" +
                           KUrl::toPercentEncoding( aFileName ) +
                           "\" border=\"0\">\n"
                           "</body></html>\n" );
      htmlWriter()->end();
      q->setWindowTitle( i18n("View Attachment: %1", pname ) );
      q->show();
      delete iio;
  } else {
    htmlWriter()->begin( mCSSHelper->cssDefinitions( mUseFixedFont ) );
    htmlWriter()->queue( mCSSHelper->htmlHead( mUseFixedFont ) );
    htmlWriter()->queue( "<pre>" );

    QString str = aMsgPart->decodedText();
    // A QString cannot handle binary data. So if it's shorter than the
    // attachment, we assume the attachment is binary:
    if( str.length() < aMsgPart->decodedContent().size() ) {
      str.prepend( i18np("[KMail: Attachment contains binary data. Trying to show first character.]",
          "[KMail: Attachment contains binary data. Trying to show first %1 characters.]",
                               str.length()) + QChar::fromLatin1('\n') );
    }
    htmlWriter()->queue( Qt::escape( str ) );
    htmlWriter()->queue( "</pre>" );
    htmlWriter()->queue("</body></html>");
    htmlWriter()->flush();
    mMainWindow->setWindowTitle(i18n("View Attachment: %1", pname));
  }
}


void MailViewerPrivate::showHideMimeTree( )
{
  if ( GlobalSettings::self()->mimeTreeMode() == GlobalSettings::EnumMimeTreeMode::Always )
    mMimePartTree->show();
  else {
    // don't rely on QSplitter maintaining sizes for hidden widgets:
    KConfigGroup reader( Global::instance()->config() , "Reader" );
    saveSplitterSizes( reader );
    mMimePartTree->hide();
  }
  if ( mToggleMimePartTreeAction && ( mToggleMimePartTreeAction->isChecked() != mMimePartTree->isVisible() ) )
    mToggleMimePartTreeAction->setChecked( mMimePartTree->isVisible() );
}


void MailViewerPrivate::atmViewMsg(KMime::Content* aMsgPart)
{
  assert(aMsgPart!=0);
  KMime::Content* msg = new KMime::Content( mMessage->parent() );
  msg->setContent(aMsgPart->decodedContent());
  msg->parse();
  assert(msg != 0);
/*FIXME(Andras)  port it
  msg->setMsgSerNum( 0 ); // because lookups will fail
  // some information that is needed for imap messages with LOD
  msg->setParent( message()->parent() );
  msg->setUID(message()->UID());
  msg->setReadyToShow(true);

  KMReaderMainWin *win = new KMReaderMainWin();
  win->showMsg( overrideEncoding(), msg );
  win->show();
 */
}


KUrl MailViewerPrivate::tempFileUrlFromNode( const KMime::Content *node )
{
  if (!node)
    return KUrl();

  QString index = node->index().toString();

  foreach ( const QString &path, mTempFiles ) {
    int right = path.lastIndexOf( '/' );
    int left = path.lastIndexOf( ".index.", right );
    if (left != -1)
        left += 7;

    QString storedIndex = path.mid( left + 1, right - left - 1 );
    if ( left != -1 && storedIndex == index )
      return KUrl( path );
  }
  return KUrl();
}


void MailViewerPrivate::adjustLayout() {
  if ( mMimeTreeAtBottom )
    mSplitter->addWidget( mMimePartTree );
  else
    mSplitter->insertWidget( 0, mMimePartTree );
  mSplitter->setSizes( mSplitterSizes );

  if ( GlobalSettings::self()->mimeTreeMode() == GlobalSettings::EnumMimeTreeMode::Always &&
    mMsgDisplay )
    mMimePartTree->show();
  else
    mMimePartTree->hide();

  if (  GlobalSettings::self()->showColorBar() && mMsgDisplay )
    mColorBar->show();
  else
    mColorBar->hide();
}


void MailViewerPrivate::saveSplitterSizes( KConfigGroup & c ) const
{
  if ( !mSplitter || !mMimePartTree )
    return;
  if ( mMimePartTree->isHidden() )
    return; // don't rely on QSplitter maintaining sizes for hidden widgets.

  const bool mimeTreeAtBottom = GlobalSettings::self()->mimeTreeLocation() == GlobalSettings::EnumMimeTreeLocation::bottom;
  c.writeEntry( "MimePaneHeight", mSplitter->sizes()[ mimeTreeAtBottom ? 1 : 0 ] );
  c.writeEntry( "MessagePaneHeight", mSplitter->sizes()[ mimeTreeAtBottom ? 0 : 1 ] );
}

void MailViewerPrivate::createWidgets() {
  QVBoxLayout * vlay = new QVBoxLayout( q );
  vlay->setMargin( 0 );
  mSplitter = new QSplitter( Qt::Vertical, q );
  mSplitter->setObjectName( "mSplitter" );
  mSplitter->setChildrenCollapsible( false );
  vlay->addWidget( mSplitter );
  mMimePartTree = new QTreeView( mSplitter );
  mMimePartTree->setObjectName( "mMimePartTree" );
  mMimePartModel = new MimeTreeModel( mMimePartTree );
  mMimePartTree->setModel( mMimePartModel );
  mMimePartTree->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mMimePartTree->setSelectionBehavior( QAbstractItemView::SelectRows );
  connect(mMimePartTree, SIGNAL( activated( const QModelIndex& ) ), this, SLOT( slotMimePartSelected( const QModelIndex& ) ) );
  mMimePartTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mMimePartTree, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( slotMimeTreeContextMenuRequested(const QPoint&)) );
  mBox = new KHBox( mSplitter );
  setStyleDependantFrameWidth();
  mBox->setFrameStyle( mMimePartTree->frameStyle() );
  mColorBar = new HtmlStatusBar( mBox );
  mColorBar->setObjectName( "mColorBar" );
  mViewer = new KHTMLPart( mBox );
  mViewer->setObjectName( "mViewer" );
  // Remove the shortcut for the selectAll action from khtml part. It's redefined to
  // CTRL-SHIFT-A in kmail and clashes with kmails CTRL-A action.
  KAction *selectAll = qobject_cast<KAction*>(
          mViewer->actionCollection()->action( "selectAll" ) );
  if ( selectAll ) {
    selectAll->setShortcut( KShortcut() );
  } else {
    kDebug() << "Failed to find khtml's selectAll action to remove it's shortcut";
  }
  mSplitter->setStretchFactor( mSplitter->indexOf(mMimePartTree), 0 );
  mSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
}


void MailViewerPrivate::createActions()
{
  KActionCollection *ac = mActionCollection;
  if ( !ac ) {
    return;
  }

  KToggleAction *raction = 0;

  // header style
  KActionMenu *headerMenu  = new KActionMenu(i18nc("View->", "&Headers"), this);
  ac->addAction("view_headers", headerMenu );
  headerMenu->setHelpText( i18n("Choose display style of message headers") );

  connect( headerMenu, SIGNAL(triggered(bool)),
           this, SLOT(slotCycleHeaderStyles()) );

  QActionGroup *group = new QActionGroup( this );
  raction = new KToggleAction( i18nc("View->headers->", "&Enterprise Headers"), this);
  ac->addAction( "view_headers_enterprise", raction );
  connect( raction, SIGNAL(triggered(bool)), SLOT(slotEnterpriseHeaders()) );
  raction->setHelpText( i18n("Show the list of headers in Enterprise style") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Fancy Headers"), this);
  ac->addAction("view_headers_fancy", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotFancyHeaders()));
  raction->setHelpText( i18n("Show the list of headers in a fancy format") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Brief Headers"), this);
  ac->addAction("view_headers_brief", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotBriefHeaders()));
  raction->setHelpText( i18n("Show brief list of message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Standard Headers"), this);
  ac->addAction("view_headers_standard", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotStandardHeaders()));
  raction->setHelpText( i18n("Show standard list of message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&Long Headers"), this);
  ac->addAction("view_headers_long", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotLongHeaders()));
  raction->setHelpText( i18n("Show long list of message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->headers->", "&All Headers"), this);
  ac->addAction("view_headers_all", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotAllHeaders()));
  raction->setHelpText( i18n("Show all message headers") );
  group->addAction( raction );
  headerMenu->addAction( raction );

  // attachment style
  KActionMenu *attachmentMenu  = new KActionMenu(i18nc("View->", "&Attachments"), this);
  ac->addAction("view_attachments", attachmentMenu );
  attachmentMenu->setHelpText( i18n("Choose display style of attachments") );
  connect( attachmentMenu, SIGNAL(triggered(bool)),
           this, SLOT(slotCycleAttachmentStrategy()) );

  group = new QActionGroup( this );
  raction  = new KToggleAction(i18nc("View->attachments->", "&As Icons"), this);
  ac->addAction("view_attachments_as_icons", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotIconicAttachments()));
  raction->setHelpText( i18n("Show all attachments as icons. Click to see them.") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->attachments->", "&Smart"), this);
  ac->addAction("view_attachments_smart", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotSmartAttachments()));
  raction->setHelpText( i18n("Show attachments as suggested by sender.") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->attachments->", "&Inline"), this);
  ac->addAction("view_attachments_inline", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotInlineAttachments()));
  raction->setHelpText( i18n("Show all attachments inline (if possible)") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  raction  = new KToggleAction(i18nc("View->attachments->", "&Hide"), this);
  ac->addAction("view_attachments_hide", raction );
  connect(raction, SIGNAL(triggered(bool) ), SLOT(slotHideAttachments()));
  raction->setHelpText( i18n("Do not show attachments in the message viewer") );
  group->addAction( raction );
  attachmentMenu->addAction( raction );

  // Set Encoding submenu
  mSelectEncodingAction  = new KSelectAction(KIcon("character-set"), i18n("&Set Encoding"), this);
  mSelectEncodingAction->setToolBarMode( KSelectAction::MenuMode );
  ac->addAction("encoding", mSelectEncodingAction );
  connect(mSelectEncodingAction,SIGNAL( triggered(int)),
          SLOT( slotSetEncoding() ));
  QStringList encodings = MailViewerPrivate::supportedEncodings( false );
  encodings.prepend( i18n( "Auto" ) );
  mSelectEncodingAction->setItems( encodings );
  mSelectEncodingAction->setCurrentItem( 0 );

  //
  // Message Menu
  //

  // copy selected text to clipboard
  mCopyAction = ac->addAction( KStandardAction::Copy, "kmail_copy", this,
                               SLOT(slotCopySelectedText()) );

  // copy all text to clipboard
  mSelectAllAction  = new KAction(i18n("Select All Text"), this);
  ac->addAction("mark_all_text", mSelectAllAction );
  connect(mSelectAllAction, SIGNAL(triggered(bool) ), SLOT(selectAll()));
  mSelectAllAction->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_A));

  // copy Email address to clipboard
  mCopyURLAction = new KAction( KIcon( "edit-copy" ),
                                i18n( "Copy Link Address" ), this );
  ac->addAction( "copy_url", mCopyURLAction );
  connect( mCopyURLAction, SIGNAL(triggered(bool)), SLOT(slotUrlCopy()) );

  // open URL
  mUrlOpenAction = new KAction( KIcon( "document-open" ), i18n( "Open URL" ), this );
  ac->addAction( "open_url", mUrlOpenAction );
  connect( mUrlOpenAction, SIGNAL(triggered(bool)), this, SLOT(slotUrlOpen()) );

  // use fixed font
  mToggleFixFontAction = new KToggleAction( i18n( "Use Fi&xed Font" ), this );
  ac->addAction( "toggle_fixedfont", mToggleFixFontAction );
  connect( mToggleFixFontAction, SIGNAL(triggered(bool)), SLOT(slotToggleFixedFont()) );
  mToggleFixFontAction->setShortcut( QKeySequence( Qt::Key_X ) );

  // Show message structure viewer
  mToggleMimePartTreeAction = new KToggleAction( i18n( "Show Message Structure" ), this );
  ac->addAction( "toggle_mimeparttree", mToggleMimePartTreeAction );
  connect( mToggleMimePartTreeAction, SIGNAL(toggled(bool)),
           SLOT(slotToggleMimePartTree()));

  mViewSourceAction  = new KAction(i18n("&View Source"), this);
  ac->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool) ), SLOT(slotShowMessageSource()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  mSaveMessageAction = new KAction(i18n("&Save message"), this);
  ac->addAction("save_message", mSaveMessageAction);
  connect(mSaveMessageAction, SIGNAL(triggered(bool) ), SLOT(slotSaveMessage()));
  mSaveMessageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));

  //
  // Scroll actions
  //
  mScrollUpAction = new KAction( i18n("Scroll Message Up"), this );
  mScrollUpAction->setShortcut( QKeySequence( Qt::Key_Up ) );
  ac->addAction( "scroll_up", mScrollUpAction );
  connect( mScrollUpAction, SIGNAL( triggered( bool ) ),
           q, SLOT( slotScrollUp() ) );

  mScrollDownAction = new KAction( i18n("Scroll Message Down"), this );
  mScrollDownAction->setShortcut( QKeySequence( Qt::Key_Down ) );
  ac->addAction( "scroll_down", mScrollDownAction );
  connect( mScrollDownAction, SIGNAL( triggered( bool ) ),
           q, SLOT( slotScrollDown() ) );

  mScrollUpMoreAction = new KAction( i18n("Scroll Message Up (More)"), this );
  mScrollUpMoreAction->setShortcut( QKeySequence( Qt::Key_PageUp ) );
  ac->addAction( "scroll_up_more", mScrollUpMoreAction );
  connect( mScrollUpMoreAction, SIGNAL( triggered( bool ) ),
           q, SLOT( slotScrollPrior() ) );

  mScrollDownMoreAction = new KAction( i18n("Scroll Message Down (More)"), this );
  mScrollDownMoreAction->setShortcut( QKeySequence( Qt::Key_PageDown ) );
  ac->addAction( "scroll_down_more", mScrollDownMoreAction );
  connect( mScrollDownMoreAction, SIGNAL( triggered( bool ) ),
           q, SLOT( slotScrollNext() ) );
}


void MailViewerPrivate::showContextMenu( KMime::Content* content, const QPoint &pos )
{
  if ( !content )
    return;
  const bool isAttachment = !content->contentType()->isMultipart() && !content->isTopLevel();
  const bool isRoot = (content == mMessage);

  KMenu popup;

  if ( !isRoot ) {
    popup.addAction( SmallIcon( "document-save-as" ), i18n( "Save &As..." ),
                     this, SLOT( slotAttachmentSaveAs() ) );

    if ( isAttachment ) {
      popup.addAction( SmallIcon( "document-open" ), i18nc( "to open", "Open" ),
                       this, SLOT( slotAttachmentOpen() ) );
      popup.addAction( i18n( "Open With..." ), this, SLOT( slotAttachmentOpenWith() ) );
      popup.addAction( i18nc( "to view something", "View" ), this, SLOT( slotAttachmentView() ) );
    }
  }

  /*
   * FIXME make optional?
  popup.addAction( i18n( "Save as &Encoded..." ), this,
                   SLOT( slotSaveAsEncoded() ) );
  */

  popup.addAction( i18n( "Save All Attachments..." ), this,
                   SLOT( slotAttachmentSaveAll() ) );

  // edit + delete only for attachments
  if ( !isRoot ) {
    if ( isAttachment ) {
      popup.addAction( SmallIcon( "edit-copy" ), i18n( "Copy" ),
                       this, SLOT( slotAttachmentCopy() ) );
      if ( GlobalSettings::self()->allowAttachmentDeletion() )
        popup.addAction( SmallIcon( "edit-delete" ), i18n( "Delete Attachment" ),
                         this, SLOT( slotAttachmentDelete() ) );
      if ( GlobalSettings::self()->allowAttachmentEditing() )
        popup.addAction( SmallIcon( "document-properties" ), i18n( "Edit Attachment" ),
                         this, SLOT( slotAttachmentEdit() ) );
    }

    if ( !content->isTopLevel() )
      popup.addAction( i18n( "Properties" ), this, SLOT( slotAttachmentProperties() ) );
  }
  popup.exec( mMimePartTree->viewport()->mapToGlobal( pos ) );

}


KToggleAction *MailViewerPrivate::actionForHeaderStyle( const HeaderStyle * style, const HeaderStrategy * strategy ) {
  if ( !mActionCollection )
    return 0;
  const char * actionName = 0;
  if ( style == HeaderStyle::enterprise() )
    actionName = "view_headers_enterprise";
  if ( style == HeaderStyle::fancy() )
    actionName = "view_headers_fancy";
  else if ( style == HeaderStyle::brief() )
    actionName = "view_headers_brief";
  else if ( style == HeaderStyle::plain() ) {
    if ( strategy == HeaderStrategy::standard() )
      actionName = "view_headers_standard";
    else if ( strategy == HeaderStrategy::rich() )
      actionName = "view_headers_long";
    else if ( strategy == HeaderStrategy::all() )
      actionName = "view_headers_all";
  }
  if ( actionName )
    return static_cast<KToggleAction*>(mActionCollection->action(actionName));
  else
    return 0;
}

KToggleAction *MailViewerPrivate::actionForAttachmentStrategy( const AttachmentStrategy * as ) {
  if ( !mActionCollection )
    return 0;
  const char * actionName = 0;
  if ( as == AttachmentStrategy::iconic() )
    actionName = "view_attachments_as_icons";
  else if ( as == AttachmentStrategy::smart() )
    actionName = "view_attachments_smart";
  else if ( as == AttachmentStrategy::inlined() )
    actionName = "view_attachments_inline";
  else if ( as == AttachmentStrategy::hidden() )
    actionName = "view_attachments_hide";

  if ( actionName )
    return static_cast<KToggleAction*>(mActionCollection->action(actionName));
  else
    return 0;
}


void MailViewerPrivate::readGlobalOverrideCodec()
{
  // if the global character encoding wasn't changed then there's nothing to do
 if ( GlobalSettings::self()->overrideCharacterEncoding() == mOldGlobalOverrideEncoding )
    return;

  setOverrideEncoding( GlobalSettings::self()->overrideCharacterEncoding() );
  mOldGlobalOverrideEncoding = GlobalSettings::self()->overrideCharacterEncoding();
}


const QTextCodec * MailViewerPrivate::overrideCodec() const
{
  if ( mOverrideEncoding.isEmpty() || mOverrideEncoding == "Auto" ) // Auto
    return 0;
  else
    return MailViewerPrivate::codecForName( mOverrideEncoding.toLatin1() );
}


static QColor nextColor( const QColor & c )
{
  int h, s, v;
  c.getHsv( &h, &s, &v );
  return QColor::fromHsv( (h + 50) % 360, qMax(s, 64), v );
}

QString MailViewerPrivate::renderAttachments(KMime::Content * node, const QColor &bgColor )
{

  if ( !node )
    return QString();

  QString html;
  KMime::Content * child = NodeHelper::firstChild( node );

  if ( child) {
    QString subHtml = renderAttachments( child, nextColor( bgColor ) );
    if ( !subHtml.isEmpty() ) {

      QString visibility;
      if( !mShowAttachmentQuicklist ) {
        visibility.append( "display:none;" );
      }

      QString margin;
      if ( node != mMessage || headerStyle() != HeaderStyle::enterprise() )
        margin = "padding:2px; margin:2px; ";
      QString align = "left";
      if ( headerStyle() == HeaderStyle::enterprise() )
        align = "right";
      if ( node->contentType()->mediaType() == "message" || node == mMessage )
        html += QString::fromLatin1("<div style=\"background:%1; %2"
                "vertical-align:middle; float:%3; %4\">").arg( bgColor.name() ).arg( margin )
                                                         .arg( align ).arg( visibility );
      html += subHtml;
      if ( node->contentType()->mediaType() == "message" || node == mMessage )
        html += "</div>";
    }
  } else {
    QString label, icon;
    icon = NodeHelper::instance()->iconName( node, KIconLoader::Small );
    label = node->contentDescription()->asUnicodeString();
    if( label.isEmpty() ) {
      label = NodeHelper::fileName( node );
    }

    bool typeBlacklisted = node->contentType()->mediaType() == "multipart";
    if ( !typeBlacklisted ) {
      typeBlacklisted = StringUtil::isCryptoPart( node->contentType()->mediaType(),  node->contentType()->subType(),
                                                  node->contentDisposition()->filename() );
    }
    typeBlacklisted = typeBlacklisted || node == mMessage;
    if ( !label.isEmpty() && !icon.isEmpty() && !typeBlacklisted ) {
      html += "<div style=\"float:left;\">";
      html += QString::fromLatin1( "<span style=\"white-space:nowrap; border-width: 0px; border-left-width: 5px; border-color: %1; 2px; border-left-style: solid;\">" ).arg( bgColor.name() );
      QString fileName = writeMessagePartToTempFile( node );
      QString href = NodeHelper::asHREF( node, "header" );
      html += QString::fromLatin1( "<a href=\"" ) + href +
              QString::fromLatin1( "\">" );
      html += "<img style=\"vertical-align:middle;\" src=\"" + icon + "\"/>&nbsp;";
      if ( headerStyle() == HeaderStyle::enterprise() ) {
        QFont bodyFont = mCSSHelper->bodyFont( mUseFixedFont );
        QFontMetrics fm( bodyFont );
        html += fm.elidedText( label, Qt::ElideRight, 180 );
      } else {
        html += label;
      }
      html += "</a></span></div> ";
    }
  }

  KMime::Content *next  = NodeHelper::nextSibling( node );
  if ( next )
    html += renderAttachments( next, nextColor ( bgColor ) );

  return html;
}

KMime::Content* MailViewerPrivate::findContentByType(KMime::Content *content, const QByteArray &type)
{
    KMime::Content::List list = content->contents();
    Q_FOREACH(KMime::Content *c, list)
    {
        if (c->contentType()->mimeType() ==  type)
            return c;
    }
    return 0L;

}


QString MailViewerPrivate::fixEncoding( const QString &encoding )
{
  QString returnEncoding = encoding;
  // According to http://www.iana.org/assignments/character-sets, uppercase is
  // preferred in MIME headers
  if ( returnEncoding.toUpper().contains( "ISO " ) ) {
    returnEncoding = returnEncoding.toUpper();
    returnEncoding.replace( "ISO ", "ISO-" );
  }
  return returnEncoding;
}

//-----------------------------------------------------------------------------
QString MailViewerPrivate::encodingForName( const QString &descriptiveName )
{
  QString encoding = KGlobal::charsets()->encodingForName( descriptiveName );
  return MailViewerPrivate::fixEncoding( encoding );
}

//-----------------------------------------------------------------------------
const QTextCodec* MailViewerPrivate::codecForName(const QByteArray& _str)
{
  if (_str.isEmpty())
    return 0;
  QByteArray codec = _str;
  kAsciiToLower(codec.data());
  return KGlobal::charsets()->codecForName(codec);
}

QStringList MailViewerPrivate::supportedEncodings(bool usAscii)
{
  QStringList encodingNames = KGlobal::charsets()->availableEncodingNames();
  QStringList encodings;
  QMap<QString,bool> mimeNames;
  for (QStringList::Iterator it = encodingNames.begin();
    it != encodingNames.end(); ++it)
  {
    QTextCodec *codec = KGlobal::charsets()->codecForName(*it);
    QString mimeName = (codec) ? QString(codec->name()).toLower() : (*it);
    if (!mimeNames.contains(mimeName) )
    {
      encodings.append( KGlobal::charsets()->descriptionForEncoding(*it) );
      mimeNames.insert( mimeName, true );
    }
  }
  encodings.sort();
  if (usAscii)
    encodings.prepend(KGlobal::charsets()->descriptionForEncoding("us-ascii") );
  return encodings;
}


void MailViewerPrivate::update( MailViewer::UpdateMode updateMode )
{
  // Avoid flicker, somewhat of a cludge
  if ( updateMode == MailViewer::Force ) {
    // stop the timer to avoid calling updateReaderWin twice
      mUpdateReaderWinTimer.stop();
      updateReaderWin();
  }
  else if (mUpdateReaderWinTimer.isActive()) {
      mUpdateReaderWinTimer.setInterval( delay );
  } else {
      mUpdateReaderWinTimer.start( 0 );
  }
}


void MailViewerPrivate::slotUrlOpen( const KUrl &url )
{
  kDebug() << "slotUrlOpen " << url;
  if ( !url.isEmpty() ) {
    mUrlClicked = url;
    slotUrlOpen( url, KParts::OpenUrlArguments(), KParts::BrowserArguments() );
  }
}


void MailViewerPrivate::slotUrlOpen(const KUrl &aUrl, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)
{
  mUrlClicked = aUrl;

  if ( URLHandlerManager::instance()->handleClick( aUrl, this ) )
    return;

  kWarning() << "Unhandled URL click! " << aUrl;
  emit urlClicked( aUrl, Qt::LeftButton );
}


void MailViewerPrivate::slotUrlOn(const QString &aUrl)
{
  const KUrl url(aUrl);
  if ( url.protocol() == "kmail" || url.protocol() == "x-kmail"
       || (url.protocol().isEmpty() && url.path().isEmpty()) ) {
    mViewer->setDNDEnabled( false );
  } else {
    mViewer->setDNDEnabled( true );
  }

  if ( aUrl.trimmed().isEmpty() ) {
    KPIM::BroadcastStatus::instance()->reset();
    return;
  }

  mUrlClicked = url;

  const QString msg = URLHandlerManager::instance()->statusBarMessage( url, this );

  kWarning( msg.isEmpty(), 5006 ) << "Unhandled URL hover!";
  KPIM::BroadcastStatus::instance()->setTransientStatusMsg( msg );
}

void MailViewerPrivate::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  const KUrl url( aUrl );
  mUrlClicked = url;

  if ( URLHandlerManager::instance()->handleContextMenuRequest( url, aPos, this ) )
    return;

  if ( mMessage ) {
    kWarning() << "Unhandled URL right-click!";
    emit popupMenu( *mMessage, url, aPos );
  }
}


void MailViewerPrivate::slotFind()
{
  mViewer->findText();
}


void MailViewerPrivate::slotToggleFixedFont()
{
  mUseFixedFont = !mUseFixedFont;
  saveRelativePosition();
  update( MailViewer::Force );
}

void MailViewerPrivate::slotToggleMimePartTree()
{
  if ( mToggleMimePartTreeAction->isChecked() )
    GlobalSettings::self()->setMimeTreeMode( GlobalSettings::EnumMimeTreeMode::Always );
  else
    GlobalSettings::self()->setMimeTreeMode( GlobalSettings::EnumMimeTreeMode::Never );
  showHideMimeTree();
}


void MailViewerPrivate::slotShowMessageSource()
{
/* FIXME(Andras)
  if ( msg->isComplete() && !mMsgWasComplete ) {
    msg->notify(); // notify observers as msg was transferred
  }
*/
  QString str = QString::fromAscii( mMessage->encodedContent() );

  MailSourceViewer *viewer = new MailSourceViewer(); // deletes itself upon close
  viewer->setWindowTitle( i18n("Message as Plain Text") );
  viewer->setText( str );
  if( mUseFixedFont ) {
    viewer->setFont( KGlobalSettings::fixedFont() );
  }

  // Well, there is no widget to be seen here, so we have to use QCursor::pos()
  // Update: (GS) I'm not going to make this code behave according to Xinerama
  //         configuration because this is quite the hack.
  if ( QApplication::desktop()->isVirtualDesktop() ) {
    int scnum = QApplication::desktop()->screenNumber( QCursor::pos() );
    viewer->resize( QApplication::desktop()->screenGeometry( scnum ).width()/2,
                    2 * QApplication::desktop()->screenGeometry( scnum ).height()/3);
  } else {
    viewer->resize( QApplication::desktop()->geometry().width()/2,
                    2 * QApplication::desktop()->geometry().height()/3);
  }
  viewer->show();
}

void MailViewerPrivate::updateReaderWin()
{
  if ( !mMsgDisplay ) {
    return;
  }

  mViewer->setOnlyLocalReferences( !htmlLoadExternal() );

  htmlWriter()->reset();
  //TODO: if the item doesn't have the payload fetched, try to fetch it? Maybe not here, but in setMessageItem.
  if ( mMessage )
  {
    if ( GlobalSettings::self()->showColorBar() ) {
      mColorBar->show();
    } else {
      mColorBar->hide();
    }
    displayMessage();
  } else {
    mColorBar->hide();
    mMimePartTree->hide();
  //FIXME(Andras)  mMimePartTree->clearAndResetSortOrder();
    htmlWriter()->begin( mCSSHelper->cssDefinitions( mUseFixedFont) );
    htmlWriter()->write( mCSSHelper->htmlHead( mUseFixedFont ) + "</body></html>" );
    htmlWriter()->end();
  }

  if ( mSavedRelativePosition ) {
    QScrollArea *scrollview = mViewer->view();
    scrollview->widget()->move( 0,
      qRound( scrollview->widget()->size().height() * mSavedRelativePosition ) );
    mSavedRelativePosition = 0;
  }
}


void MailViewerPrivate::slotMimePartSelected( const QModelIndex &index )
{
  KMime::Content *content = static_cast<KMime::Content*>( index.internalPointer() );
  if ( !mMimePartModel->parent(index).isValid() && index.row() == 0 ) {
   update(MailViewer::Force);
  } else
    setMessagePart( content );
}


void MailViewerPrivate::slotCycleHeaderStyles() {
  const HeaderStrategy * strategy = headerStrategy();
  const HeaderStyle * style = headerStyle();

  const char * actionName = 0;
  if ( style == HeaderStyle::enterprise() ) {
    slotFancyHeaders();
    actionName = "view_headers_fancy";
  }
  if ( style == HeaderStyle::fancy() ) {
    slotBriefHeaders();
    actionName = "view_headers_brief";
  } else if ( style == HeaderStyle::brief() ) {
    slotStandardHeaders();
    actionName = "view_headers_standard";
  } else if ( style == HeaderStyle::plain() ) {
    if ( strategy == HeaderStrategy::standard() ) {
      slotLongHeaders();
      actionName = "view_headers_long";
    } else if ( strategy == HeaderStrategy::rich() ) {
      slotAllHeaders();
      actionName = "view_headers_all";
    } else if ( strategy == HeaderStrategy::all() ) {
      slotEnterpriseHeaders();
      actionName = "view_headers_enterprise";
    }
  }

  if ( actionName )
    static_cast<KToggleAction*>( mActionCollection->action( actionName ) )->setChecked( true );
}


void MailViewerPrivate::slotBriefHeaders()
{
  setHeaderStyleAndStrategy( HeaderStyle::brief(),
                             HeaderStrategy::brief() );
  if( !mExternalWindow )
    writeConfig();
}


void MailViewerPrivate::slotFancyHeaders()
{
  setHeaderStyleAndStrategy( HeaderStyle::fancy(),
                             HeaderStrategy::rich() );
  if( !mExternalWindow )
    writeConfig();
}


void MailViewerPrivate::slotEnterpriseHeaders()
{
  setHeaderStyleAndStrategy( HeaderStyle::enterprise(),
                             HeaderStrategy::rich() );
  if( !mExternalWindow )
    writeConfig();
}


void MailViewerPrivate::slotStandardHeaders()
{
  setHeaderStyleAndStrategy( HeaderStyle::plain(),
                             HeaderStrategy::standard());
  writeConfig();
}


void MailViewerPrivate::slotLongHeaders()
{
  setHeaderStyleAndStrategy( HeaderStyle::plain(),
                             HeaderStrategy::rich() );
  if( !mExternalWindow )
    writeConfig();
}



void MailViewerPrivate::slotAllHeaders() {
  setHeaderStyleAndStrategy( HeaderStyle::plain(),
                             HeaderStrategy::all() );
  if( !mExternalWindow )
    writeConfig();
}


void MailViewerPrivate::slotCycleAttachmentStrategy()
{
  setAttachmentStrategy( attachmentStrategy()->next() );
  KToggleAction * action = actionForAttachmentStrategy( attachmentStrategy() );
  assert( action );
  action->setChecked( true );
}


void MailViewerPrivate::slotIconicAttachments()
{
  setAttachmentStrategy( AttachmentStrategy::iconic() );
}


void MailViewerPrivate::slotSmartAttachments()
{
  setAttachmentStrategy( AttachmentStrategy::smart() );
}


void MailViewerPrivate::slotInlineAttachments()
{
  setAttachmentStrategy( AttachmentStrategy::inlined() );
}


void MailViewerPrivate::slotHideAttachments()
{
  setAttachmentStrategy( AttachmentStrategy::hidden() );
}


void MailViewerPrivate::slotAtmView( KMime::Content *atmNode )
{
  if ( atmNode ) {
    QString fileName = tempFileUrlFromNode( atmNode).toLocalFile();

    QString pname = NodeHelper::fileName( atmNode );
    if (pname.isEmpty()) pname = atmNode->contentDescription()->asUnicodeString();
    if (pname.isEmpty()) pname = "unnamed";
    // image Attachment is saved already
    if (kasciistricmp(atmNode->contentType()->mediaType(), "message")==0) {
      atmViewMsg( atmNode );
    } else if ((kasciistricmp(atmNode->contentType()->mediaType(), "text")==0) &&
               ( (kasciistricmp(atmNode->contentType()->subType(), "x-vcard")==0) ||
                 (kasciistricmp(atmNode->contentType()->subType(), "directory")==0) )) {
      setMessagePart( atmNode, htmlMail(), fileName, pname );
    } else {
      /*TODO(Andras) port
      KMReaderMainWin *win = new KMReaderMainWin(&msgPart, htmlMail(),
          name, pname, overrideEncoding() );
      win->show();
      */
    }
  }
}


void MailViewerPrivate::slotDelayedResize()
{
  mSplitter->setGeometry( 0, 0, q->width(), q->height() );
}


void MailViewerPrivate::slotPrintMsg()
{
  disconnect( mPartHtmlWriter, SIGNAL( finished() ), this, SLOT( slotPrintMsg() ) );
  if ( !mMessage ) return;
  mViewer->view()->print();
}


void MailViewerPrivate::slotSetEncoding()
{
  if ( mSelectEncodingAction->currentItem() == 0 ) // Auto
    mOverrideEncoding.clear();
  else
    mOverrideEncoding = MailViewerPrivate::encodingForName( mSelectEncodingAction->currentText() );
  update( MailViewer::Force );
}

void MailViewerPrivate::injectAttachments()
{
  // inject attachments in header view
  // we have to do that after the otp has run so we also see encrypted parts
  DOM::Document doc = mViewer->htmlDocument();
  DOM::Element injectionPoint = doc.getElementById( "attachmentInjectionPoint" );
  if ( injectionPoint.isNull() )
    return;

  QString imgpath( KStandardDirs::locate("data","kmail/pics/") );
  QString visibility;
  QString urlHandle;
  QString imgSrc;
  if( !mShowAttachmentQuicklist ) {
    urlHandle.append( "kmail:showAttachmentQuicklist" );
    imgSrc.append( "attachmentQuicklistClosed.png" );
  } else {
    urlHandle.append( "kmail:hideAttachmentQuicklist" );
    imgSrc.append( "attachmentQuicklistOpened.png" );
  }

  QColor background = KColorScheme( QPalette::Active, KColorScheme::View ).background().color();
  QString html = renderAttachments( mMessage, background );
  if ( html.isEmpty() )
    return;

  QString link("");
  if ( headerStyle() == HeaderStyle::fancy() ) {
    link += "<div style=\"text-align: left;\"><a href=\""+urlHandle+"\"><img src=\""+imgpath+imgSrc+"\"/></a></div>";
    html.prepend( link );
    html.prepend( QString::fromLatin1("<div style=\"float:left;\">%1&nbsp;</div>" ).arg(i18n("Attachments:")) );
  } else {
    link += "<div style=\"text-align: right;\"><a href=\""+urlHandle+"\"><img src=\""+imgpath+imgSrc+"\"/></a></div>";
    html.prepend( link );
  }

  assert( injectionPoint.tagName() == "div" );
  static_cast<DOM::HTMLElement>( injectionPoint ).setInnerHTML( html );
}


void MailViewerPrivate::slotSettingsChanged()
{
  mShowColorbar = GlobalSettings::self()->showColorBar();
  saveRelativePosition();
  update( MailViewer::Force );
}


void MailViewerPrivate::slotMimeTreeContextMenuRequested( const QPoint& pos )
{
  QModelIndex index = mMimePartTree->indexAt( pos );
  if ( index.isValid() ) {
     KMime::Content *content = static_cast<KMime::Content*>( index.internalPointer() );
     showContextMenu( content, pos );
  }
}

void MailViewerPrivate::slotAttachmentOpenWith()
{
  QItemSelectionModel *selectionModel = mMimePartTree->selectionModel();
  QModelIndexList selectedRows = selectionModel->selectedRows();

  Q_FOREACH(QModelIndex index, selectedRows)
  {
     KMime::Content *content = static_cast<KMime::Content*>( index.internalPointer() );
     attachmentOpenWith( content );
 }
}

void MailViewerPrivate::slotAttachmentOpen()
{

  QItemSelectionModel *selectionModel = mMimePartTree->selectionModel();
  QModelIndexList selectedRows = selectionModel->selectedRows();

  Q_FOREACH(QModelIndex index, selectedRows)
  {
    KMime::Content *content = static_cast<KMime::Content*>( index.internalPointer() );
    attachmentOpen( content );
  }
}


void MailViewerPrivate::slotAttachmentSaveAs()
{
  KMime::Content::List contents = selectedContents();

  if ( contents.isEmpty() )
     return;

  saveAttachments( contents );
}


void MailViewerPrivate::slotAttachmentSaveAll()
{
  KMime::Content::List contents = allContents( mMessage );

  for ( KMime::Content::List::iterator it = contents.begin();
        it != contents.end(); ) {
    // only body parts which have a filename or a name parameter (except for
    // the root node for which name is set to the message's subject) are
    // considered attachments
    KMime::Content* content = *it;
    if ( content->contentDisposition()->filename().trimmed().isEmpty() &&
          ( content->contentType()->name().trimmed().isEmpty() ||
            content == mMessage ) ) {
      KMime::Content::List::iterator delIt = it;
      ++it;
      contents.erase( delIt );
    } else {
      ++it;
    }
  }

  if ( contents.isEmpty() ) {
    KMessageBox::information( mMainWindow, i18n("Found no attachments to save.") );
    return;
  }

  saveAttachments( contents );

}


void MailViewerPrivate::slotAttachmentView()
{
  KMime::Content::List contents = selectedContents();

  Q_FOREACH(KMime::Content *content, contents)
  {
    slotAtmView( content );
  }

}

void MailViewerPrivate::slotAttachmentProperties()
{
  KMime::Content::List contents = selectedContents();

  if ( contents.isEmpty() )
     return;

  Q_FOREACH( KMime::Content *content, contents ) {
    KPIM::AttachmentPropertiesDialog *dialog = new KPIM::AttachmentPropertiesDialog( content, mMainWindow );
    dialog->setAttribute( Qt::WA_DeleteOnClose );
    dialog->show();
  }
}


void MailViewerPrivate::slotAttachmentCopy()
{
  KMime::Content::List contents = selectedContents();

  if ( contents.isEmpty() )
    return;

  QList<QUrl> urls;
  Q_FOREACH( KMime::Content *content, contents) {
    KUrl kUrl = writeMessagePartToTempFile( content );
    QUrl url = QUrl::fromPercentEncoding( kUrl.toEncoded() );
    if ( !url.isValid() )
      continue;
    urls.append( url );
  }

  if ( urls.isEmpty() )
    return;

  QMimeData *mimeData = new QMimeData;
  mimeData->setUrls( urls );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );
}


void MailViewerPrivate::slotAttachmentDelete()
{
  KMime::Content::List contents = selectedContents();
  if ( contents.isEmpty() )
    return;

  bool showWarning = true;
  Q_FOREACH( KMime::Content *content, contents ) {
    if ( !deleteAttachment( content, showWarning ) )
      return;
    showWarning = false;
  }
}


void MailViewerPrivate::slotAttachmentEdit()
{
  KMime::Content::List contents = selectedContents();
  if ( contents.isEmpty() )
    return;

  bool showWarning = true;
  Q_FOREACH( KMime::Content *content, contents ) {
    if ( !editAttachment( content, showWarning ) )
      return;
    showWarning = false;
  }
}


void MailViewerPrivate::slotAttachmentEditDone( EditorWatcher* editorWatcher )
{
  QString name = editorWatcher->url().fileName();
  if ( editorWatcher->fileChanged() ) {
    QFile file( name );
    if ( file.open( QIODevice::ReadOnly ) ) {
      QByteArray data = file.readAll();
      KMime::Content *node = mEditorWatchers[editorWatcher];
      node->setBody( data );
      file.close();

      mMessageItem.setPayloadFromData( mMessage->encodedContent() );
      Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( mMessageItem );
    }
  }
  mEditorWatchers.remove( editorWatcher );
  QFile::remove( name );
}

void MailViewerPrivate::slotLevelQuote( int l )
{
  kDebug() << "Old Level:" << mLevelQuote << "New Level:" << l;

  mLevelQuote = l;
  saveRelativePosition();
  update( MailViewer::Force );
}

void MailViewerPrivate::slotTouchMessage()
{
/*FIXME(Andras) port it
  if ( !message() )
    return;

  if ( !message()->status().isNew() && !message()->status().isUnread() )
    return;
  SerNumList serNums;
  serNums.append( message()->getMsgSerNum() );
  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusRead(), serNums );
  command->start();
  // should we send an MDN?
  if ( mNoMDNsWhenEncrypted &&
       message()->encryptionState() != KMMsgNotEncrypted &&
       message()->encryptionState() != KMMsgEncryptionStateUnknown )
    return;

    KMFolder *folder = message()->parent();
  if ( folder &&
       ( folder->isOutbox() || folder->isSent() || folder->isTrash() ||
         folder->isDrafts() || folder->isTemplates() ) )
    return;
    if ( KMMessage * receipt = message()->createMDN( MDN::ManualAction,
                                                    MDN::Displayed,
                                                    true  )//true == allow GUI
          if ( !kmkernel->msgSender()->send( receipt ) ) // send or queue
      KMessageBox::error( mMainWindow, i18n("Could not send MDN.") );
 */
}

void MailViewerPrivate::slotHandleAttachment( int choice )
{
    /*FIXME(Andras) port to akonadi
  mAtmUpdate = true;
  partNode* node = mMessage ? mMessage->findId( mAtmCurrent ) : 0;
  if ( choice == KMHandleAttachmentCommand::Delete ) {
    slotDeleteAttachment( node );
  } else if ( choice == KMHandleAttachmentCommand::Edit ) {
    slotEditAttachment( node );
  } else if ( choice == KMHandleAttachmentCommand::Copy ) {
    if ( !node )
      return;
    QList<QUrl> urls;
    KUrl kUrl = tempFileUrlFromNode( node );
    QUrl url = QUrl::fromPercentEncoding( kUrl.toEncoded() );

    if ( !url.isValid() )
      return;
    urls.append( url );

    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls( urls );
    QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );
  }
  else {
    KMHandleAttachmentCommand* command = new KMHandleAttachmentCommand(
        node, message(), mAtmCurrent, mAtmCurrentName,
        KMHandleAttachmentCommand::AttachmentAction( choice ), KService::Ptr( 0 ), this );
    connect( command, SIGNAL( showAttachment( int, const QString& ) ),
        this, SLOT( slotAtmView( int, const QString& ) ) );
    command->start();
  }
   */
}

void MailViewerPrivate::slotCopySelectedText()
{
  QString selection = mViewer->selectedText();
  selection.replace( QChar::Nbsp, ' ' );
  QApplication::clipboard()->setText( selection );
}

void MailViewerPrivate::selectAll()
{
  mViewer->selectAll();
}

void MailViewerPrivate::slotUrlClicked()
{
  kDebug() << "Clicked on " << mUrlClicked;
    /*FIXME Andras
  KMMainWidget *mainWidget = dynamic_cast<KMMainWidget*>(mMainWindow);
  uint identity = 0;
  if ( message() && message()->parent() ) {
    identity = message()->parent()->identity();
  }

  KMCommand *command = new KMUrlClickedCommand( mUrlClicked, identity, this,
                                                false, mainWidget );
  command->start();*/
}


void MailViewerPrivate::slotUrlCopy()
{
  // we don't necessarily need a mainWidget for KMUrlCopyCommand so
  // it doesn't matter if the dynamic_cast fails.
  /* FIXME(Andras) port it
  KMCommand *command =
    new KMUrlCopyCommand( mUrlClicked,
                          dynamic_cast<KMMainWidget*>( mMainWindow ) );
  command->start();
  */
}


void MailViewerPrivate::slotUrlSave()
{
    /*FIXME(Andras) port to akonadi
  KMCommand *command = new KMUrlSaveCommand( mUrlClicked, mMainWindow );
  command->start();
    */
}

void MailViewerPrivate::slotSaveMessage()
{
   KUrl url = KFileDialog::getSaveUrl( KUrl::fromPath( mMessage->subject()->asUnicodeString().trimmed()
                                  .replace( QDir::separator(), '_' ) ),
                                  "*.mbox", mMainWindow );

   if ( url.isEmpty() )
     return;

  QByteArray data( mMessage->encodedContent() );
  QDataStream ds;
  QFile file;
  KTemporaryFile tf;
  if ( url.isLocalFile() )
  {
    // save directly
    file.setFileName( url.toLocalFile() );
    if ( !file.open( QIODevice::WriteOnly ) )
    {
      KMessageBox::error( mMainWindow,
                          i18nc( "1 = file name, 2 = error string",
                                 "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                                 file.fileName(),
                                 QString::fromLocal8Bit( strerror( errno ) ) ),
                          i18n( "Error saving attachment" ) );
      return;
    }

    //TODO handle huge attachment (and on demand attachment loading), especially saving them to
    //remote destination

/*FIXME(Andras) port it
    // #79685 by default use the umask the user defined, but let it be configurable
    if ( GlobalSettings::self()->disregardUmask() )
      fchmod( file.handle(), S_IRUSR | S_IWUSR );
*/
    ds.setDevice( &file );
  } else
  {
    // tmp file for upload
    tf.open();
    ds.setDevice( &tf );
  }

  if ( ds.writeRawData( data.data(), data.size() ) == -1)
  {
    QFile *f = static_cast<QFile *>( ds.device() );
    KMessageBox::error( mMainWindow,
                        i18nc( "1 = file name, 2 = error string",
                               "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                               f->fileName(),
                               f->errorString() ),
                        i18n( "Error saving attachment" ) );
    return;
  }

  if ( !url.isLocalFile() )
  {
    // QTemporaryFile::fileName() is only defined while the file is open
    QString tfName = tf.fileName();
    tf.close();
    if ( !KIO::NetAccess::upload( tfName, url, mMainWindow ) )
    {
      KMessageBox::error( mMainWindow,
                          i18nc( "1 = file name, 2 = error string",
                                 "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                                 url.prettyUrl(),
                                 KIO::NetAccess::lastErrorString() ),
                          i18n( "Error saving attachment" ) );
      return;
    }
  } else
    file.close();
  return;
}


void MailViewerPrivate::saveRelativePosition()
{
  const QScrollArea *scrollview = mViewer->view();
  mSavedRelativePosition = static_cast<float>( scrollview->widget()->pos().y() ) /
                           scrollview->widget()->size().height();
}

//TODO(Andras) inline them
bool MailViewerPrivate::htmlMail() const
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}

bool MailViewerPrivate::htmlLoadExternal() const
{
  return ((mHtmlLoadExternal && !mHtmlLoadExtOverride) ||
          (!mHtmlLoadExternal && mHtmlLoadExtOverride));
}

void MailViewerPrivate::setHtmlOverride( bool override )
{
  mHtmlOverride = override;
}

bool MailViewerPrivate::htmlOverride() const
{
  return mHtmlOverride;
}

void MailViewerPrivate::setHtmlLoadExtOverride( bool override )
{
  mHtmlLoadExtOverride = override;
}

bool MailViewerPrivate::htmlLoadExtOverride() const
{
  return mHtmlLoadExtOverride;
}

void MailViewerPrivate::setDecryptMessageOverwrite( bool overwrite )
{
  mDecrytMessageOverwrite = overwrite;
}

bool MailViewerPrivate::showSignatureDetails() const
{
  return mShowSignatureDetails;
}

void MailViewerPrivate::setShowSignatureDetails( bool showDetails )
{
  mShowSignatureDetails = showDetails;
}

bool MailViewerPrivate::showAttachmentQuicklist() const
{
  return mShowAttachmentQuicklist;
}

void MailViewerPrivate::setShowAttachmentQuicklist( bool showAttachmentQuicklist  )
{
  mShowAttachmentQuicklist = showAttachmentQuicklist;
}

void MailViewerPrivate::scrollToAttachment( const KMime::Content *node )
{
  DOM::Document doc = mViewer->htmlDocument();

  // The anchors for this are created in ObjectTreeParser::parseObjectTree()
  mViewer->gotoAnchor( QString::fromLatin1( "att%1" ).arg( node->index().toString() ) );

  // Remove any old color markings which might be there
  const KMime::Content *root = node->topLevel();
  int totalChildCount = allContents( root ).size();
  for ( int i = 0; i <= totalChildCount + 1; i++ ) {
    DOM::Element attachmentDiv = doc.getElementById( QString( "attachmentDiv%1" ).arg( i + 1 ) );
    if ( !attachmentDiv.isNull() )
      attachmentDiv.removeAttribute( "style" );
  }

  // Now, color the div of the attachment in yellow, so that the user sees what happened.
  // We created a special marked div for this in writeAttachmentMarkHeader() in ObjectTreeParser,
  // find and modify that now.


  kDebug() << "'ANDRIS::looking for " << QString( "attachmentDiv%1" ).arg( node->index().toString() );
  DOM::Element attachmentDiv = doc.getElementById( QString( "attachmentDiv%1" ).arg( node->index().toString() ) );
  if ( attachmentDiv.isNull() ) {
    kWarning() << "Could not find attachment div for attachment" << node->index().toString();
    return;
  }
  attachmentDiv.setAttribute( "style", QString( "border:2px solid %1" )
      .arg( cssHelper()->pgpWarnColor().name() ) );

  // Update rendering, otherwise the rendering is not updated when the user clicks on an attachment
  // that causes scrolling and the open attachment dialog
  doc.updateRendering();
  q->update();
}

void MailViewerPrivate::setUseFixedFont( bool useFixedFont )
{
  mUseFixedFont = useFixedFont;
  if ( mToggleFixFontAction )
  {
    mToggleFixFontAction->setChecked( mUseFixedFont );
  }
}


#include "mailviewer_p.moc"
