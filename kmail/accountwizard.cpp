/*******************************************************************************
**
** Filename   : accountwizard.cpp
** Created on : 07 February, 2005
** Copyright  : (c) 2005 Tobias Koenig
** Email      : tokoe@kde.org
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
*******************************************************************************/

#include <kdialog.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <klistbox.h>
#include <klocale.h>

#include <tqcheckbox.h>
#include <tqdir.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqvbox.h>

#include "kmacctlocal.h"
#include "kmkernel.h"
#include "popaccount.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "kmacctmaildir.h"
#include "accountmanager.h"
using KMail::AccountManager;

#include "globalsettings.h"
#include "kmservertest.h"
#include "kmtransport.h"
#include "libkpimidentities/identity.h"
#include "libkpimidentities/identitymanager.h"
#include "protocols.h"

#include "accountwizard.h"

enum Capabilities
{
  Plain      =   1,
  Login      =   2,
  CRAM_MD5   =   4,
  Digest_MD5 =   8,
  Anonymous  =  16,
  APOP       =  32,
  Pipelining =  64,
  TOP        = 128,
  UIDL       = 256,
  STLS       = 512, // TLS for POP
  STARTTLS   = 512, // TLS for IMAP
  GSSAPI     = 1024,
  NTLM       = 2048,
  AllCapa    = 0xffffffff
};

class AccountTypeBox : public KListBox
{
  public:
    enum Type { Local, POP3, IMAP, dIMAP, Maildir };

    AccountTypeBox( TQWidget *parent )
      : KListBox( parent, "AccountTypeBox" )
    {
      mTypeList << i18n( "Local mailbox" );
      mTypeList << i18n( "POP3" );
      mTypeList << i18n( "IMAP" );
      mTypeList << i18n( "Disconnected IMAP" );
      mTypeList << i18n( "Maildir mailbox" );

      insertStringList( mTypeList );
    }

    void setType( Type type )
    {
      setCurrentItem( (int)type );
    }

    Type type() const
    {
      return (Type)currentItem();
    }

  private:
    TQStringList mTypeList;
};

AccountWizard::AccountWizard( KMKernel *kernel, TQWidget *parent )
  : KWizard( parent, "KWizard" ), mKernel( kernel ),
    mAccount( 0 ), mTransportInfo( 0 ), mServerTest( 0 )
{
  setupWelcomePage();
  setupAccountTypePage();
  setupAccountInformationPage();
  setupLoginInformationPage();
  setupServerInformationPage();
}

void AccountWizard::start( KMKernel *kernel, TQWidget *parent )
{
  KConfigGroup wizardConfig( KMKernel::config(), "AccountWizard" );

  if ( wizardConfig.readBoolEntry( "ShowOnStartup", true ) ) {
    AccountWizard wizard( kernel, parent );
    int result = wizard.exec();
    if ( result == TQDialog::Accepted ) {
      wizardConfig.writeEntry( "ShowOnStartup", false );
      kernel->slotConfigChanged();
    }
  }
}

void AccountWizard::showPage( TQWidget *page )
{
  if ( page == mWelcomePage ) {
    // do nothing
  } else if ( page == mAccountTypePage ) {
    if ( mTypeBox->currentItem() == -1 )
      mTypeBox->setType( AccountTypeBox::POP3 );
  } else if ( page == mAccountInformationPage ) {
    if ( mRealName->text().isEmpty() && mEMailAddress->text().isEmpty() &&
         mOrganization->text().isEmpty() ) {
      KPIM::IdentityManager *manager = mKernel->identityManager();
      const KPIM::Identity &identity = manager->defaultIdentity();

      mRealName->setText( identity.fullName() );
      mEMailAddress->setText( identity.emailAddr() );
      mOrganization->setText( identity.organization() );
    }
  } else if ( page == mLoginInformationPage ) {
    if ( mLoginName->text().isEmpty() ) {
      // try to extract login from email address
      TQString email = mEMailAddress->text();
      int pos = email.find( '@' );
      if ( pos != -1 )
        mLoginName->setText( email.left( pos ) );

      // take the whole email as login otherwise?!?
    }
  } else if ( page == mServerInformationPage ) {
    if ( mTypeBox->type() == AccountTypeBox::Local ||
         mTypeBox->type() == AccountTypeBox::Maildir ) {
      mIncomingServerWdg->hide();
      mIncomingLocationWdg->show();
      mIncomingLabel->setText( i18n( "Location:" ) );

      if ( mTypeBox->type() == AccountTypeBox::Local )
        mIncomingLocation->setText( TQDir::homeDirPath() + "/inbox" );
      else
        mIncomingLocation->setText( TQDir::homeDirPath() + "/Mail/" );
    } else {
      mIncomingLocationWdg->hide();
      mIncomingServerWdg->show();
      mIncomingLabel->setText( i18n( "Incoming server:" ) );
    }

    setFinishEnabled( mServerInformationPage, true );
  }

  TQWizard::showPage( page );
}

void AccountWizard::setupWelcomePage()
{
  mWelcomePage = new TQVBox( this );
  ((TQVBox*)mWelcomePage)->setSpacing( KDialog::spacingHint() );

  TQLabel *label = new TQLabel( i18n( "Welcome to KMail" ), mWelcomePage );
  TQFont font = label->font();
  font.setBold( true );
  label->setFont( font );

  new TQLabel( i18n( "<qt>It seems you have started KMail for the first time. "
                    "You can use this wizard to setup your mail accounts. Just "
                    "enter the connection data that you received from your email provider "
                    "into the following pages.</qt>" ), mWelcomePage );

  addPage( mWelcomePage, i18n( "Welcome" ) );
}

void AccountWizard::setupAccountTypePage()
{
  mAccountTypePage = new TQVBox( this );
  ((TQVBox*)mAccountTypePage)->setSpacing( KDialog::spacingHint() );

  new TQLabel( i18n( "Select what kind of account you would like to create" ), mAccountTypePage );

  mTypeBox = new AccountTypeBox( mAccountTypePage );

  addPage( mAccountTypePage, i18n( "Account Type" ) );
}

void AccountWizard::setupAccountInformationPage()
{
  mAccountInformationPage = new TQWidget( this );
  TQGridLayout *layout = new TQGridLayout( mAccountInformationPage, 3, 2,
                                         KDialog::marginHint(), KDialog::spacingHint() );

  TQLabel *label = new TQLabel( i18n( "Real name:" ), mAccountInformationPage );
  mRealName = new KLineEdit( mAccountInformationPage );
  label->setBuddy( mRealName );

  layout->addWidget( label, 0, 0 );
  layout->addWidget( mRealName, 0, 1 );

  label = new TQLabel( i18n( "E-mail address:" ), mAccountInformationPage );
  mEMailAddress = new KLineEdit( mAccountInformationPage );
  label->setBuddy( mEMailAddress );

  layout->addWidget( label, 1, 0 );
  layout->addWidget( mEMailAddress, 1, 1 );

  label = new TQLabel( i18n( "Organization:" ), mAccountInformationPage );
  mOrganization = new KLineEdit( mAccountInformationPage );
  label->setBuddy( mOrganization );

  layout->addWidget( label, 2, 0 );
  layout->addWidget( mOrganization, 2, 1 );

  addPage( mAccountInformationPage, i18n( "Account Information" ) );
}

void AccountWizard::setupLoginInformationPage()
{
  mLoginInformationPage = new TQWidget( this );
  TQGridLayout *layout = new TQGridLayout( mLoginInformationPage, 2, 2,
                                         KDialog::marginHint(), KDialog::spacingHint() );

  TQLabel *label = new TQLabel( i18n( "Login name:" ), mLoginInformationPage );
  mLoginName = new KLineEdit( mLoginInformationPage );
  label->setBuddy( mLoginName );

  layout->addWidget( label, 0, 0 );
  layout->addWidget( mLoginName, 0, 1 );

  label = new TQLabel( i18n( "Password:" ), mLoginInformationPage );
  mPassword = new KLineEdit( mLoginInformationPage );
  mPassword->setEchoMode( TQLineEdit::Password );
  label->setBuddy( mPassword );

  layout->addWidget( label, 1, 0 );
  layout->addWidget( mPassword, 1, 1 );

  addPage( mLoginInformationPage, i18n( "Login Information" ) );
}

void AccountWizard::setupServerInformationPage()
{
  mServerInformationPage = new TQWidget( this );
  TQGridLayout *layout = new TQGridLayout( mServerInformationPage, 3, 2,
                                         KDialog::marginHint(), KDialog::spacingHint() );

  mIncomingLabel = new TQLabel( mServerInformationPage );

  mIncomingServerWdg = new TQVBox( mServerInformationPage );
  mIncomingServer = new KLineEdit( mIncomingServerWdg );
  mIncomingUseSSL = new TQCheckBox( i18n( "Use secure connection (SSL)" ), mIncomingServerWdg );

  mIncomingLocationWdg = new TQHBox( mServerInformationPage );
  mIncomingLocation = new KLineEdit( mIncomingLocationWdg );
  mChooseLocation = new TQPushButton( i18n( "Choose..." ), mIncomingLocationWdg );

  connect( mChooseLocation, TQT_SIGNAL( clicked() ),
           this, TQT_SLOT( chooseLocation() ) );

  layout->addWidget( mIncomingLabel, 0, 0, AlignTop );
  layout->addWidget( mIncomingLocationWdg, 0, 1 );
  layout->addWidget( mIncomingServerWdg, 0, 1 );

  TQLabel *label = new TQLabel( i18n( "Outgoing server:" ), mServerInformationPage );
  mOutgoingServer = new KLineEdit( mServerInformationPage );
  label->setBuddy( mOutgoingServer );

  layout->addWidget( label, 1, 0 );
  layout->addWidget( mOutgoingServer, 1, 1 );

  mOutgoingUseSSL = new TQCheckBox( i18n( "Use secure connection (SSL)" ), mServerInformationPage );
  layout->addWidget( mOutgoingUseSSL, 2, 1 );

  mLocalDelivery = new TQCheckBox( i18n( "Use local delivery" ),
                                  mServerInformationPage );
  layout->addWidget( mLocalDelivery, 3, 0 );

  connect( mLocalDelivery, TQT_SIGNAL( toggled( bool ) ),
           mOutgoingServer, TQT_SLOT( setDisabled( bool ) ) );

  addPage( mServerInformationPage, i18n( "Server Information" ) );
}

void AccountWizard::chooseLocation()
{
  TQString location;

  if ( mTypeBox->type() == AccountTypeBox::Local ) {
    location = KFileDialog::getSaveFileName( TQString(), TQString(), this );
  } else if ( mTypeBox->type() == AccountTypeBox::Maildir ) {
    location = KFileDialog::getExistingDirectory( TQString(), this );
  }

  if ( !location.isEmpty() )
    mIncomingLocation->setText( location );
}

TQString AccountWizard::accountName() const
{
  // create account name
  TQString name( i18n( "None" ) );

  TQString email = mEMailAddress->text();
  int pos = email.find( '@' );
  if ( pos != -1 ) {
    name = email.mid( pos + 1 );
    name[ 0 ] = name[ 0 ].upper();
  }

  return name;
}

TQLabel *AccountWizard::createInfoLabel( const TQString &msg )
{
  TQLabel *label = new TQLabel( msg, this );
  label->setFrameStyle( TQFrame::Panel | TQFrame::Raised );
  label->resize( fontMetrics().width( msg ) + 20, label->height() * 2 );
  label->move( width() / 2 - label->width() / 2, height() / 2 - label->height() / 2 );
  label->show();

  return label;
}

void AccountWizard::accept()
{
  // store identity information
  KPIM::IdentityManager *manager = mKernel->identityManager();
  KPIM::Identity &identity = manager->modifyIdentityForUoid( manager->defaultIdentity().uoid() );

  identity.setFullName( mRealName->text() );
  identity.setEmailAddr( mEMailAddress->text() );
  identity.setOrganization( mOrganization->text() );

  manager->commit();

  TQTimer::singleShot( 0, this, TQT_SLOT( createTransport() ) );
}

void AccountWizard::createTransport()
{
  // create outgoing account
  KConfigGroup general( KMKernel::config(), "General" );

  uint numTransports = general.readNumEntry( "transports", 0 );

  for ( uint i = 1 ; i <= numTransports ; i++ ) {
    KMTransportInfo *info = new KMTransportInfo();
    info->readConfig( i );
    mTransportInfoList.append( info );
  }

  mTransportInfo = new KMTransportInfo();

  if ( mLocalDelivery->isChecked() ) { // local delivery
    mTransportInfo->type = "sendmail";
    mTransportInfo->name = i18n( "Sendmail" );
    mTransportInfo->host = "/usr/sbin/sendmail"; // TODO: search for sendmail in PATH
    mTransportInfo->auth = false;
    mTransportInfo->setStorePasswd( false );

    TQTimer::singleShot( 0, this, TQT_SLOT( transportCreated() ) );
  } else { // delivery via SMTP
    mTransportInfo->type = "smtp";
    mTransportInfo->name = accountName();
    mTransportInfo->host = mOutgoingServer->text();
    mTransportInfo->user = mLoginName->text();
    mTransportInfo->setPasswd( mPassword->text() );

    int port = (mOutgoingUseSSL->isChecked() ? 465 : 25);
    checkSmtpCapabilities( mTransportInfo->host, port );
  }
}

void AccountWizard::transportCreated()
{
  mTransportInfoList.append( mTransportInfo );

  KConfigGroup general( KMKernel::config(), "General" );
  general.writeEntry( "transports", mTransportInfoList.count() );

  for ( uint i = 0 ; i < mTransportInfoList.count() ; i++ )
    mTransportInfo->writeConfig( i + 1 );

    // No default transport? => set the first transport as the default
  if ( GlobalSettings::self()->defaultTransport().isEmpty() ) {
    KConfigGroup general( KMKernel::config(), "General" );

    if ( mTransportInfoList.count() > 0 ) {
      KMTransportInfo info;
      info.readConfig( 1 );
      KConfigGroup composer( KMKernel::config(), "Composer" );
      GlobalSettings::self()->setDefaultTransport( info.name );
      GlobalSettings::self()->setCurrentTransport( info.name );
    }
  }

  mTransportInfoList.setAutoDelete( true );
  mTransportInfoList.clear();

  TQTimer::singleShot( 0, this, TQT_SLOT( createAccount() ) );
}

void AccountWizard::createAccount()
{
  // create incoming account
  AccountManager *acctManager = mKernel->acctMgr();

  int port = 0;

  switch ( mTypeBox->type() ) {
    case AccountTypeBox::Local:
    {
      mAccount = acctManager->create( "local", i18n( "Local Account" ) );
      static_cast<KMAcctLocal*>( mAccount )->setLocation( mIncomingLocation->text() );
      break;
    }
    case AccountTypeBox::POP3:
    {
      mAccount = acctManager->create( "pop", accountName() );
      KMail::PopAccount *acct = static_cast<KMail::PopAccount*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      port = mIncomingUseSSL->isChecked() ? 995 : 110;
      break;
    }
    case AccountTypeBox::IMAP:
    {
      mAccount = acctManager->create( "imap", accountName() );
      KMAcctImap *acct = static_cast<KMAcctImap*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      port = mIncomingUseSSL->isChecked() ? 993 : 143;
      break;
    }
    case AccountTypeBox::dIMAP:
    {
      mAccount = acctManager->create( "cachedimap", accountName() );
      KMAcctCachedImap *acct = static_cast<KMAcctCachedImap*>( mAccount );
      acct->setLogin( mLoginName->text() );
      acct->setPasswd( mPassword->text() );
      acct->setHost( mIncomingServer->text() );
      port = mIncomingUseSSL->isChecked() ? 993 : 143;
      break;
    }
    case AccountTypeBox::Maildir:
    {
      mAccount = acctManager->create( "maildir", i18n( "Local Account" ) );
      static_cast<KMAcctMaildir*>( mAccount )->setLocation( mIncomingLocation->text() );
      break;
    }
  }

  if ( mTypeBox->type() == AccountTypeBox::POP3 )
    checkPopCapabilities( mIncomingServer->text(), port );
  else if ( mTypeBox->type() == AccountTypeBox::IMAP || mTypeBox->type() == AccountTypeBox::dIMAP )
    checkImapCapabilities( mIncomingServer->text(), port );
  else
    TQTimer::singleShot( 0, this, TQT_SLOT( accountCreated() ) );
}

void AccountWizard::accountCreated()
{
  if ( mAccount )
  {
    mKernel->acctMgr()->add( mAccount );
    mKernel->cleanupImapFolders();
  }

  finished();
}

void AccountWizard::finished()
{
  GlobalSettings::self()->writeConfig();

  TQWizard::accept();
}

// ----- Security Checks --------------

void AccountWizard::checkPopCapabilities( const TQString &server, int port )
{
  delete mServerTest;
  mServerTest = new KMServerTest( POP_PROTOCOL, server, port );

  connect( mServerTest, TQT_SIGNAL( capabilities( const TQStringList&, const TQStringList& ) ),
           this, TQT_SLOT( popCapabilities( const TQStringList&, const TQStringList& ) ) );

  mAuthInfoLabel = createInfoLabel( i18n( "Check for supported security capabilities of %1..." ).arg( server ) );
}

void AccountWizard::checkImapCapabilities( const TQString &server, int port )
{
  delete mServerTest;
  mServerTest = new KMServerTest( IMAP_PROTOCOL, server, port );

  connect( mServerTest, TQT_SIGNAL( capabilities( const TQStringList&, const TQStringList& ) ),
           this, TQT_SLOT( imapCapabilities( const TQStringList&, const TQStringList& ) ) );

  mAuthInfoLabel = createInfoLabel( i18n( "Check for supported security capabilities of %1..." ).arg( server ) );
}

void AccountWizard::checkSmtpCapabilities( const TQString &server, int port )
{
  delete mServerTest;
  mServerTest = new KMServerTest( SMTP_PROTOCOL, server, port );

  connect( mServerTest, TQT_SIGNAL( capabilities( const TQStringList&, const TQStringList&,
                                              const TQString&, const TQString&, const TQString& ) ),
           this, TQT_SLOT( smtpCapabilities( const TQStringList&, const TQStringList&,
                                         const TQString&, const TQString&, const TQString& ) ) );

  mAuthInfoLabel = createInfoLabel( i18n( "Check for supported security capabilities of %1..." ).arg( server ) );
}

void AccountWizard::popCapabilities( const TQStringList &capaNormalList,
                                     const TQStringList &capaSSLList )
{
  uint capaNormal = popCapabilitiesFromStringList( capaNormalList );
  uint capaTLS = 0;

  if ( capaNormal & STLS )
    capaTLS = capaNormal;

  uint capaSSL = popCapabilitiesFromStringList( capaSSLList );

  KMail::NetworkAccount *account = static_cast<KMail::NetworkAccount*>( mAccount );

  bool useSSL = !capaSSLList.isEmpty();
  bool useTLS = capaTLS != 0;

  account->setUseSSL( useSSL );
  account->setUseTLS( useTLS );

  uint capa = (useSSL ? capaSSL : (useTLS ? capaTLS : capaNormal));

  if ( capa & Plain )
    account->setAuth( "PLAIN" );
  else if ( capa & Login )
    account->setAuth( "LOGIN" );
  else if ( capa & CRAM_MD5 )
    account->setAuth( "CRAM-MD5" );
  else if ( capa & Digest_MD5 )
    account->setAuth( "DIGEST-MD5" );
  else if ( capa & NTLM )
    account->setAuth( "NTLM" );
  else if ( capa & GSSAPI )
    account->setAuth( "GSSAPI" );
  else if ( capa & APOP )
    account->setAuth( "APOP" );
  else
    account->setAuth( "USER" );

  account->setPort( useSSL ? 995 : 110 );

  mServerTest->deleteLater();
  mServerTest = 0;

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  accountCreated();
}


void AccountWizard::imapCapabilities( const TQStringList &capaNormalList,
                                      const TQStringList &capaSSLList )
{
  uint capaNormal = imapCapabilitiesFromStringList( capaNormalList );
  uint capaTLS = 0;
  if ( capaNormal & STARTTLS )
    capaTLS = capaNormal;

  uint capaSSL = imapCapabilitiesFromStringList( capaSSLList );

  KMail::NetworkAccount *account = static_cast<KMail::NetworkAccount*>( mAccount );

  bool useSSL = !capaSSLList.isEmpty();
  bool useTLS = (capaTLS != 0);

  account->setUseSSL( useSSL );
  account->setUseTLS( useTLS );

  uint capa = (useSSL ? capaSSL : (useTLS ? capaTLS : capaNormal));

  if ( capa & CRAM_MD5 )
    account->setAuth( "CRAM-MD5" );
  else if ( capa & Digest_MD5 )
    account->setAuth( "DIGEST-MD5" );
  else if ( capa & NTLM )
    account->setAuth( "NTLM" );
  else if ( capa & GSSAPI )
    account->setAuth( "GSSAPI" );
  else if ( capa & Anonymous )
    account->setAuth( "ANONYMOUS" );
  else if ( capa & Login )
    account->setAuth( "LOGIN" );
  else if ( capa & Plain )
    account->setAuth( "PLAIN" );
  else
    account->setAuth( "*" );

  account->setPort( useSSL ? 993 : 143 );

  mServerTest->deleteLater();
  mServerTest = 0;

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  accountCreated();
}

void AccountWizard::smtpCapabilities( const TQStringList &capaNormal,
                                      const TQStringList &capaSSL,
                                      const TQString &authNone,
                                      const TQString &authSSL,
                                      const TQString &authTLS )
{
  uint authBitsNone, authBitsSSL, authBitsTLS;

  if ( authNone.isEmpty() && authSSL.isEmpty() && authTLS.isEmpty() ) {
    // slave doesn't seem to support "* AUTH METHODS" metadata (or server can't do AUTH)
    authBitsNone = authMethodsFromStringList( capaNormal );
    if ( capaNormal.findIndex( "STARTTLS" ) != -1 )
      authBitsTLS = authBitsNone;
    else
      authBitsTLS = 0;
    authBitsSSL = authMethodsFromStringList( capaSSL );
  } else {
    authBitsNone = authMethodsFromString( authNone );
    authBitsSSL = authMethodsFromString( authSSL );
    authBitsTLS = authMethodsFromString( authTLS );
  }

  uint authBits = 0;
  if ( capaNormal.findIndex( "STARTTLS" ) != -1 ) {
    mTransportInfo->encryption = "TLS";
    authBits = authBitsTLS;
  } else if ( !capaSSL.isEmpty() ) {
    mTransportInfo->encryption = "SSL";
    authBits = authBitsSSL;
  } else {
    mTransportInfo->encryption = "NONE";
    authBits = authBitsNone;
  }

  if ( authBits & Login )
    mTransportInfo->authType = "LOGIN";
  else if ( authBits & CRAM_MD5 )
    mTransportInfo->authType = "CRAM-MD5";
  else if ( authBits & Digest_MD5 )
    mTransportInfo->authType = "DIGEST-MD5";
  else if ( authBits & NTLM )
    mTransportInfo->authType = "NTLM";
  else if ( authBits & GSSAPI )
    mTransportInfo->authType = "GSSAPI";
  else
    mTransportInfo->authType = "PLAIN";

  mTransportInfo->port = ( !capaSSL.isEmpty() ? "465" : "25" );

  mServerTest->deleteLater();
  mServerTest = 0;

  delete mAuthInfoLabel;
  mAuthInfoLabel = 0;

  transportCreated();
}

uint AccountWizard::popCapabilitiesFromStringList( const TQStringList & l )
{
  unsigned int capa = 0;

  for ( TQStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    TQString cur = (*it).upper();
    if ( cur == "PLAIN" )
      capa |= Plain;
    else if ( cur == "LOGIN" )
      capa |= Login;
    else if ( cur == "CRAM-MD5" )
      capa |= CRAM_MD5;
    else if ( cur == "DIGEST-MD5" )
      capa |= Digest_MD5;
    else if ( cur == "NTLM" )
      capa |= NTLM;
    else if ( cur == "GSSAPI" )
      capa |= GSSAPI;
    else if ( cur == "APOP" )
      capa |= APOP;
    else if ( cur == "STLS" )
      capa |= STLS;
  }

  return capa;
}

uint AccountWizard::imapCapabilitiesFromStringList( const TQStringList & l )
{
  unsigned int capa = 0;

  for ( TQStringList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
    TQString cur = (*it).upper();
    if ( cur == "AUTH=PLAIN" )
      capa |= Plain;
    else if ( cur == "AUTH=LOGIN" )
      capa |= Login;
    else if ( cur == "AUTH=CRAM-MD5" )
      capa |= CRAM_MD5;
    else if ( cur == "AUTH=DIGEST-MD5" )
      capa |= Digest_MD5;
    else if ( cur == "AUTH=NTLM" )
      capa |= NTLM;
    else if ( cur == "AUTH=GSSAPI" )
      capa |= GSSAPI;
    else if ( cur == "AUTH=ANONYMOUS" )
      capa |= Anonymous;
    else if ( cur == "STARTTLS" )
      capa |= STARTTLS;
  }

  return capa;
}

uint AccountWizard::authMethodsFromString( const TQString & s )
{
  unsigned int result = 0;

  TQStringList sl = TQStringList::split( '\n', s.upper() );
  for ( TQStringList::const_iterator it = sl.begin() ; it != sl.end() ; ++it )
    if (  *it == "SASL/LOGIN" )
      result |= Login;
    else if ( *it == "SASL/PLAIN" )
      result |= Plain;
    else if ( *it == "SASL/CRAM-MD5" )
      result |= CRAM_MD5;
    else if ( *it == "SASL/DIGEST-MD5" )
      result |= Digest_MD5;
    else if ( *it == "SASL/NTLM" )
      result |= NTLM;
    else if ( *it == "SASL/GSSAPI" )
      result |= GSSAPI;

  return result;
}

uint AccountWizard::authMethodsFromStringList( const TQStringList & sl )
{
  unsigned int result = 0;

  for ( TQStringList::const_iterator it = sl.begin() ; it != sl.end() ; ++it )
    if ( *it == "LOGIN" )
      result |= Login;
    else if ( *it == "PLAIN" )
      result |= Plain;
    else if ( *it == "CRAM-MD5" )
      result |= CRAM_MD5;
    else if ( *it == "DIGEST-MD5" )
      result |= Digest_MD5;
    else if ( *it == "NTLM" )
      result |= NTLM;
    else if ( *it == "GSSAPI" )
      result |= GSSAPI;

  return result;
}

#include "accountwizard.moc"
