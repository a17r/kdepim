// kmfolderdia.cpp

#include <assert.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qregexp.h>

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "kmmainwin.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include "kmidentity.h"
#include "kmfolderimap.h"
#include "kmheaders.h"

#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
			       QWidget *aParent, const QString& aCap):
  KDialogBase( KDialogBase::Plain,
               aCap, KDialogBase::Ok|KDialogBase::Cancel,
               KDialogBase::Ok, aParent, "KMFolderDialog", TRUE ),
  folder((KMAcctFolder*)aFolder),mFolderDir( aFolderDir )
{
  mFolder = aFolder;
  kdDebug(5006)<<"KMFolderDialog::KMFolderDialog()\n";

  QFrame *page = plainPage();

  QVBoxLayout *topLayout =  new QVBoxLayout( page, 0, spacingHint(),
                                             "topLayout" );

  QGroupBox *fpGroup = new QGroupBox( i18n("Folder Position"), page, "fpGroup" );
  fpGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( fpGroup );

  QHBoxLayout *hl = new QHBoxLayout( fpGroup->layout() );
  hl->setSpacing( 6 );

  QLabel *label = new QLabel( i18n("&Name:"), fpGroup );
  hl->addWidget( label );

  nameEdit = new QLineEdit( fpGroup );
  nameEdit->setFocus();
  nameEdit->setText(folder ? folder->label() : i18n("unnamed"));
  nameEdit->setMinimumSize(nameEdit->sizeHint());
  nameEdit->selectAll();
  label->setBuddy( nameEdit );
  hl->addWidget( nameEdit );

  QLabel *label2 = new QLabel( i18n("&Belongs to:" ), fpGroup );
  hl->addWidget( label2 );

  fileInFolder = new QComboBox(fpGroup);
  hl->addWidget( fileInFolder );
  label2->setBuddy( fileInFolder );

  mtGroup = new QGroupBox( i18n("Folder Type"), page, "mtGroup" );
  mtGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( mtGroup );

  QHBoxLayout *ml = new QHBoxLayout( mtGroup->layout() );
  ml->setSpacing( 6 );

  QLabel *label_type = new QLabel( i18n("Mailbox Format:" ), mtGroup );
  ml->addWidget( label_type );
  mailboxType = new QComboBox(mtGroup);
  mailboxType->insertItem("mbox", 0);
  mailboxType->insertItem("maildir", 1);
  {
    KConfig *config = kapp->config();
    KConfigGroupSaver saver(config, "General");
    int type = config->readNumEntry("default-mailbox-format", 1);
    if ( type < 0 || type > 1 ) type = 1;
    mailboxType->setCurrentItem( type );
  }
  if (aFolder) mailboxType->setEnabled(false);
  ml->addWidget( mailboxType );
  ml->addStretch( 1 );

  QStringList str;
//  if (!folder) kernel->imapFolderMgr()->createI18nFolderList( &str, &mFolders );
  kernel->imapFolderMgr()->createI18nFolderList( &str, &mFolders );
  kernel->folderMgr()->createFolderList( &str, &mFolders  );
  str.prepend( i18n( "Top Level" ));
  QGuardedPtr<KMFolder> curFolder;
  int i = 1;
  while (mFolders.at(i - 1) != mFolders.end()) {
    curFolder = *mFolders.at(i - 1);
    if (curFolder->isSystemFolder() && curFolder->protocol() != "imap") {
      mFolders.remove(mFolders.at(i-1));
      str.remove(str.at(i));
    } else
      ++i;
  }
  fileInFolder->insertStringList( str );
	// we want to know if the activated changes
	connect( fileInFolder, SIGNAL(activated(int)), SLOT(slotUpdateItems(int)) );

  if (aFolder && (aFolder->protocol() == "imap")) {
    label->setEnabled( false );
    nameEdit->setEnabled( false );
    label2->setEnabled( false );
    fileInFolder->setEnabled( false );
  }

  // Mailing-list data tab
  //
  QGroupBox *mlGroup = new QGroupBox(  i18n("Associated Mailing List" ), page );
  mlGroup->setColumnLayout( 0,  Qt::Vertical );
  QGridLayout *mlLayout = new QGridLayout(mlGroup->layout());
  mlLayout->setSpacing( 6 );
  holdsMailingList = new QCheckBox( i18n("&Folder holds a mailing list"), mlGroup);
  QObject::connect( holdsMailingList, SIGNAL(toggled(bool)),
                    SLOT(slotHoldsML(bool)) );
  topLayout->addWidget( mlGroup );
  mlLayout->addMultiCellWidget(holdsMailingList, 0, 0, 0, 1);

  mlLayout->setColStretch(0, 1);
  mlLayout->setColStretch(1, 100);

  label = new QLabel( i18n("&Post Address:"), mlGroup );
  mlLayout->addWidget( label, 1, 0 );
  mailingListPostAddress = new QLineEdit( mlGroup );
  label->setBuddy( mailingListPostAddress );
  mlLayout->addWidget( mailingListPostAddress, 1, 1 );
  mailingListPostAddress->setEnabled(false);

  //
  // Expiry data.
  //
  expGroup = new QGroupBox(i18n("Old Message Expiry"), page);
  expGroup->setColumnLayout(0, Qt::Vertical);
  QGridLayout *expLayout = new QGridLayout(expGroup->layout());
  expLayout->setSpacing(6);

  // Checkbox for setting whether expiry is enabled on this folder.
  expireFolder = new QCheckBox(i18n("Expire old messages in this folder"), expGroup);
  QObject::connect(expireFolder, SIGNAL(toggled(bool)), SLOT(slotExpireFolder(bool)));
  topLayout->addWidget(expGroup);
  expLayout->addMultiCellWidget(expireFolder, 0, 0, 0, 1);

  // Expiry time for read documents.
  label = new QLabel(i18n("Expire read email after"), expGroup);
  expLayout->addWidget(label, 1, 0);
  readExpiryTime = new KIntNumInput(expGroup);
  readExpiryTime->setRange(1, 500, 1, false);
  label->setBuddy(readExpiryTime);
  expLayout->addWidget(readExpiryTime, 1, 1);

  readExpiryUnits = new QComboBox(expGroup);
  readExpiryUnits->insertItem(i18n("Never"));
  readExpiryUnits->insertItem(i18n("day(s)"));
  readExpiryUnits->insertItem(i18n("week(s)"));
  readExpiryUnits->insertItem(i18n("month(s)"));
  expLayout->addWidget(readExpiryUnits, 1, 2);


  // Expiry time for unread documents.
  label = new QLabel(i18n("Expire unread email after"), expGroup);
  expLayout->addWidget(label, 2, 0);
  unreadExpiryTime = new KIntNumInput(expGroup);
  unreadExpiryTime->setRange(1, 500, 1, false);
  label->setBuddy(unreadExpiryTime);
  expLayout->addWidget(unreadExpiryTime, 2, 1);

  unreadExpiryUnits = new QComboBox(expGroup);
  unreadExpiryUnits->insertItem(i18n("Never"));
  unreadExpiryUnits->insertItem(i18n("day(s)"));
  unreadExpiryUnits->insertItem(i18n("week(s)"));
  unreadExpiryUnits->insertItem(i18n("month(s)"));
  expLayout->addWidget(unreadExpiryUnits, 2, 2);


  expLayout->setColStretch(0, 3);
  expLayout->setColStretch(0, 100);


  QGroupBox *idGroup = new QGroupBox(  i18n("Identity" ), page );
  idGroup->setColumnLayout( 0, Qt::Vertical );
  QHBoxLayout *idLayout = new QHBoxLayout(idGroup->layout());
  idLayout->setSpacing( 6 );
  topLayout->addWidget( idGroup );

  label = new QLabel( i18n("&Sender:"), idGroup );
  idLayout->addWidget( label );
  identity = new QComboBox( idGroup );
  identity->insertStringList( KMIdentity::identities() );
  label->setBuddy( identity );
  idLayout->addWidget( identity, 3 );

  QGroupBox *mcGroup = new QGroupBox(  i18n("Misc" ), page );
  mcGroup->setColumnLayout( 0, Qt::Vertical );
  QHBoxLayout *mcLayout = new QHBoxLayout(mcGroup->layout());
  topLayout->addWidget( mcGroup );

  markAnyMessage = new QCheckBox( i18n( "&Mark any message in this folder" ), mcGroup );
  mcLayout->addWidget( markAnyMessage );
  mcGroup->hide();

  QGroupBox* senderGroup = new QGroupBox( i18n("Show Sender/Receiver"), page, "senderGroup" );
  senderGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( senderGroup );

  QHBoxLayout *sl = new QHBoxLayout( senderGroup->layout() );
  sl->setSpacing( 6 );

  QLabel *sender_label = new QLabel( i18n("Show:" ), senderGroup );
  sl->addWidget( sender_label );
  senderType = new QComboBox(senderGroup);
  senderType->insertItem(i18n("Default"), 0);
  senderType->insertItem(i18n("Sender"), 1);
  senderType->insertItem(i18n("Receiver"), 2);

	QString whoField;
	if (aFolder) whoField = aFolder->userWhoField();
	if (whoField.isEmpty()) senderType->setCurrentItem(0);
	if (whoField == "From") senderType->setCurrentItem(1);
	if (whoField == "To") senderType->setCurrentItem(2);
  
  sl->addWidget( senderType );
  sl->addStretch( 1 );

  for( i = 1; mFolders.at(i - 1) != mFolders.end(); ++i ) {
    curFolder = *mFolders.at(i - 1);
    if (curFolder->child() == aFolderDir) {
      fileInFolder->setCurrentItem( i );
			slotUpdateItems( i );
		}	
  }

//   hl = new QHBoxLayout();
//   topLayout->addLayout( hl );

//   label = new QLabel( i18n("Admin Address:"), page );
//   hl->addWidget( label );
//   mailingListAdminAddress = new QLineEdit( page );
//   mailingListAdminAddress->setMinimumSize(mailingListAdminAddress->sizeHint());
//   hl->addWidget( mailingListAdminAddress );

  if (folder)
  {
    mailingListPostAddress->setText(folder->mailingListPostAddress());
//     mailingListAdminAddress->setText(folder->mailingListAdminAddress());
    mailingListPostAddress->setEnabled(folder->isMailingList());
//     mailingListAdminAddress->setEnabled(folder->isMailingList());
    // mailingListIdentity->setEnabled(folder->isMailingList());
    holdsMailingList->setChecked(folder->isMailingList());
    // markAnyMessage->setChecked( folder->isAnyMessageMarked() );

    if (folder->protocol() == "maildir")
      mailboxType->setCurrentItem(1);
    else
      mailboxType->setCurrentItem(0);

    for (int i=0; i < identity->count(); ++i)
      if (identity->text(i) == folder->identity()) {
         identity->setCurrentItem(i);
         break;
      }

    // Set the status of widgets to represent the folder
	// properties for auto expiry of old email.
    expireFolder->setChecked(folder->isAutoExpire());
    if (!folder->isAutoExpire()) {
      readExpiryTime->setEnabled(false);
      readExpiryUnits->setEnabled(false);
      unreadExpiryTime->setEnabled(false);
      unreadExpiryUnits->setEnabled(false);
    }
		// Legal values for units are 1=days, 2=weeks, 3=months.
		// Should really do something better than hardcoding this everywhere.
		if (folder->getReadExpireUnits() >= 0 && folder->getReadExpireUnits() < expireMaxUnits) {
	      readExpiryUnits->setCurrentItem(folder->getReadExpireUnits());
		}
		if (folder->getUnreadExpireUnits() >= 0 && folder->getUnreadExpireUnits() < expireMaxUnits) {
	      unreadExpiryUnits->setCurrentItem(folder->getUnreadExpireUnits());
		}
		int age = folder->getReadExpireAge();
		if (age >= 1 && age <= 500) {
	      readExpiryTime->setValue(age);
		} else {
		  readExpiryTime->setValue(7);
	    }
		age = folder->getUnreadExpireAge();
		if (age >= 1 && age <= 500) {
	      unreadExpiryTime->setValue(age);
		} else {
		  unreadExpiryTime->setValue(28);
		}

  } else {
    // Default values for everything if there isn't a folder
	// object yet.
    readExpiryTime->setEnabled(false);
    readExpiryUnits->setEnabled(false);
    unreadExpiryTime->setEnabled(false);
    unreadExpiryUnits->setEnabled(false);
    readExpiryTime->setValue(7);
    unreadExpiryTime->setValue(28);

  }

  // Musn't be able to edit details for a system folder.
  // Make sure we don't bomb out if there isn't a folder
  // object yet (i.e. just about to create new folder).

  if (aFolder && aFolder->protocol() == "imap") {
    expGroup->hide();
    mtGroup->hide();
		if (aFolder->isSystemFolder())
			senderGroup->hide();
  }
  else if (folder && folder->isSystemFolder()) {
    fpGroup->hide();
    mtGroup->hide();
    mlGroup->hide();
    idGroup->hide();
    mcGroup->hide();
		senderGroup->hide();
  }

  kdDebug(5006)<<"Exiting KMFolderDialog::KMFolderDialog()\n";
}


//-----------------------------------------------------------------------------
void KMFolderDialog::slotUpdateItems ( int current )
{
	KMFolder* selectedFolder = NULL;
	// check if the index is valid (the top level has no entrance in the mFolders)
	if (current > 0) selectedFolder = *mFolders.at(current - 1);
	if (selectedFolder && selectedFolder->protocol() == "imap")
	{
		// deactivate stuff that is not available for imap
		mtGroup->setEnabled( false );
		expGroup->setEnabled( false );
	} else {
		// activate it
		mtGroup->setEnabled( true );
		expGroup->setEnabled( true );
	}
}

//-----------------------------------------------------------------------------
void KMFolderDialog::slotOk()
{
  if (!mFolder)
  {
		// we got no folder so create one
    QString acctName;
    QString fldName, oldFldName;
    KMFolderDir *selectedFolderDir = &(kernel->folderMgr()->dir());
    KMFolder *selectedFolder = NULL;
    int curFolder = fileInFolder->currentItem();

    if (folder) oldFldName = folder->name();
    if (!nameEdit->text().isEmpty()) fldName = nameEdit->text();
    else fldName = oldFldName;
    fldName.replace(QRegExp("/"), "");
    fldName.replace(QRegExp("^\\."), "");
    if (fldName.isEmpty()) fldName = i18n("unnamed");
    if (curFolder != 0)
    {
      selectedFolder = *mFolders.at(curFolder - 1);
      selectedFolderDir = selectedFolder->createChildFolder();
    }

    QString message = i18n( "Failed to create folder '%1', folder already exists." ).arg(fldName);
    if ((selectedFolderDir->hasNamedFolder(fldName)) &&
      (!((folder) &&
      (selectedFolderDir == folder->parent()) &&
      (folder->name() == fldName)))) {
        KMessageBox::error( this, message );
      return;
    }

    message = i18n( "Cannot move a parent folder into a child folder." );
    KMFolderDir* folderDir = selectedFolderDir;


    // Buggy?
    if (folder && folder->child())
      while ((folderDir != &kernel->folderMgr()->dir()) &&
        (folderDir != folder->parent())){
        if (folderDir->findRef( folder ) != -1) {
          KMessageBox::error( this, message );
          return;
        }
        folderDir = folderDir->parent();
      }
    // End buggy?


    if (folder && folder->child() && (selectedFolderDir) &&
      (selectedFolderDir->path().find( folder->child()->path() + "/" ) == 0)) {
      KMessageBox::error( this, message );
      return;
    }

    if (folder && folder->child() && (selectedFolderDir == folder->child())) {
      KMessageBox::error( this, message );
      return;
    }

    if (!folder) {
      if (selectedFolder && selectedFolder->protocol() == "imap")
      {
        /* create a temporary folder to save the settings in the config-file
         * when the folder is created successfully the settings are read
         * otherwise the entry is automatically deleted at the next startup
         */
        folder = (KMAcctFolder*) new KMFolderImap(mFolderDir, fldName);
        static_cast<KMFolderImap*>(selectedFolder)->createFolder(fldName);
      } else if (mailboxType->currentItem() == 1) {
        folder = (KMAcctFolder*)kernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMaildir, selectedFolderDir );
      } else {
        folder = (KMAcctFolder*)kernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMbox, selectedFolderDir );
			}	
    }
    else if ((oldFldName != fldName) || (folder->parent() != selectedFolderDir))
    {
      if (folder->parent() != selectedFolderDir)
				folder->rename(fldName, selectedFolderDir );
      else
				folder->rename(fldName);

      kernel->folderMgr()->contentsChanged();
    }
  }

  if (folder)
  {
		// settings for mailingList
    folder->setMailingList( holdsMailingList->isChecked() );
    folder->setMailingListPostAddress( mailingListPostAddress->text() );
//   folder->setMailingListAdminAddress( mailingListAdminAddress->text() );
    folder->setMailingListAdminAddress( QString::null );

    folder->setIdentity( identity->currentText() );
// folder->setMarkAnyMessage( markAnyMessage->isChecked() );

    // Settings for auto expiry of old email messages.
    folder->setAutoExpire(expireFolder->isChecked());
    folder->setUnreadExpireAge(unreadExpiryTime->value());
    folder->setReadExpireAge(readExpiryTime->value());
    folder->setUnreadExpireUnits((ExpireUnits)unreadExpiryUnits->currentItem());
    folder->setReadExpireUnits((ExpireUnits)readExpiryUnits->currentItem());

    // set whoField
    if (senderType->currentItem() == 1)
      folder->setUserWhoField("From");
    else if (senderType->currentItem() == 2)
      folder->setUserWhoField("To");
    else
      folder->setUserWhoField(QString());
    folder->close();
  }
// reload the headers to show the changes if the folder was modified
  if (mFolder) static_cast<KMHeaders*>(this->parentWidget()->child("headers"))->setFolder(folder);

  KDialogBase::slotOk();
}

//-----------------------------------------------------------------------------
void KMFolderDialog::slotHoldsML( bool holdsML )
{
  if ( holdsML )
  {
    mailingListPostAddress->setEnabled(true);
//     mailingListAdminAddress->setEnabled(true);
  }
  else
  {
    mailingListPostAddress->setEnabled(false);
//     mailingListAdminAddress->setEnabled(false);
  }

//  mailingListIdentity->setEnabled(holdsML);
}

/**
 * Called when the 'auto expire' toggle is clicked.
 * Enables/disables all widgets related to this.
 */
void KMFolderDialog::slotExpireFolder(bool expire)
{
  if (expire) {
    readExpiryTime->setEnabled(true);
    readExpiryUnits->setEnabled(true);
    unreadExpiryTime->setEnabled(true);
    unreadExpiryUnits->setEnabled(true);
  } else {
    readExpiryTime->setEnabled(false);
    readExpiryUnits->setEnabled(false);
    unreadExpiryTime->setEnabled(false);
    unreadExpiryUnits->setEnabled(false);
  }
}
