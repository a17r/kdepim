// kmfilteraction.cpp
// The process methods really should use an enum instead of an int
// -1 -> status unchanged, 0 -> success, 1 -> failure, 2-> critical failure
// (GoOn),                 (Ok),         (ErrorButGoOn), (CriticalError)

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfilteraction.h"

#include "kmcommands.h"
#include "kmmsgpart.h"
#include "kmfiltermgr.h"
#include "kmfolderindex.h"
#include "kmfoldermgr.h"
#include "messagesender.h"
#include "kmmainwidget.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identitycombo.h>
#include <libkdepim/kfileio.h>
#include <libkdepim/collectingprocess.h>
using KPIM::CollectingProcess;
#include <mimelib/message.h>
#include "kmfawidgets.h"
#include "folderrequester.h"
using KMail::FolderRequester;
#include "kmmsgbase.h"
#include "templateparser.h"
#include "messageproperty.h"
#include "actionscheduler.h"
using KMail::MessageProperty;
using KMail::ActionScheduler;
#include "regexplineedit.h"
using KMail::RegExpLineEdit;
#include <kregexp3.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <kaudioplayer.h>
#include <kurlrequester.h>

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqtextcodec.h>
#include <tqtimer.h>
#include <tqobject.h>
#include <tqstylesheet.h>
#include <assert.h>


//=============================================================================
//
// KMFilterAction
//
//=============================================================================

KMFilterAction::KMFilterAction( const char* aName, const TQString aLabel )
{
  mName = aName;
  mLabel = aLabel;
}

KMFilterAction::~KMFilterAction()
{
}

void KMFilterAction::processAsync(KMMessage* msg) const
{
  ActionScheduler *handler = MessageProperty::filterHandler( msg );
  ReturnCode result = process( msg );
  if (handler)
    handler->actionMessage( result );
}

bool KMFilterAction::requiresBody(KMMsgBase*) const
{
  return true;
}

KMFilterAction* KMFilterAction::newAction()
{
  return 0;
}

TQWidget* KMFilterAction::createParamWidget(TQWidget* parent) const
{
  return new TQWidget(parent);
}

void KMFilterAction::applyParamWidgetValue(TQWidget*)
{
}

void KMFilterAction::setParamWidgetValue( TQWidget * ) const
{
}

void KMFilterAction::clearParamWidget( TQWidget * ) const
{
}

bool KMFilterAction::folderRemoved(KMFolder*, KMFolder*)
{
  return false;
}

int KMFilterAction::tempOpenFolder(KMFolder* aFolder)
{
  return kmkernel->filterMgr()->tempOpenFolder(aFolder);
}

void KMFilterAction::sendMDN( KMMessage * msg, KMime::MDN::DispositionType d,
                              const TQValueList<KMime::MDN::DispositionModifier> & m ) {
  if ( !msg ) return;

  /* createMDN requires Return-Path and Disposition-Notification-To
   * if it is not set in the message we assume that the notification should go to the
   * sender
   */
  const TQString returnPath = msg->headerField( "Return-Path" );
  const TQString dispNoteTo = msg->headerField( "Disposition-Notification-To" );
  if ( returnPath.isEmpty() )
    msg->setHeaderField( "Return-Path", msg->from() );
  if ( dispNoteTo.isEmpty() )
    msg->setHeaderField( "Disposition-Notification-To", msg->from() );

  KMMessage * mdn = msg->createMDN( KMime::MDN::AutomaticAction, d, false, m );
  if ( mdn && !kmkernel->msgSender()->send( mdn, KMail::MessageSender::SendLater ) ) {
    kdDebug(5006) << "KMFilterAction::sendMDN(): sending failed." << endl;
    //delete mdn;
  }

  //restore orignial header
  if ( returnPath.isEmpty() )
    msg->removeHeaderField( "Return-Path" );
  if ( dispNoteTo.isEmpty() )
    msg->removeHeaderField( "Disposition-Notification-To" );
}


//=============================================================================
//
// KMFilterActionWithNone
//
//=============================================================================

KMFilterActionWithNone::KMFilterActionWithNone( const char* aName, const TQString aLabel )
  : KMFilterAction( aName, aLabel )
{
}

const TQString KMFilterActionWithNone::displayString() const
{
  return label();
}


//=============================================================================
//
// KMFilterActionWithUOID
//
//=============================================================================

KMFilterActionWithUOID::KMFilterActionWithUOID( const char* aName, const TQString aLabel )
  : KMFilterAction( aName, aLabel ), mParameter( 0 )
{
}

void KMFilterActionWithUOID::argsFromString( const TQString argsStr )
{
  mParameter = argsStr.stripWhiteSpace().toUInt();
}

const TQString KMFilterActionWithUOID::argsAsString() const
{
  return TQString::number( mParameter );
}

const TQString KMFilterActionWithUOID::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}


//=============================================================================
//
// KMFilterActionWithString
//
//=============================================================================

KMFilterActionWithString::KMFilterActionWithString( const char* aName, const TQString aLabel )
  : KMFilterAction( aName, aLabel )
{
}

TQWidget* KMFilterActionWithString::createParamWidget( TQWidget* parent ) const
{
  TQLineEdit *le = new KLineEdit(parent);
  le->setText( mParameter );
  return le;
}

void KMFilterActionWithString::applyParamWidgetValue( TQWidget* paramWidget )
{
  mParameter = ((TQLineEdit*)paramWidget)->text();
}

void KMFilterActionWithString::setParamWidgetValue( TQWidget* paramWidget ) const
{
  ((TQLineEdit*)paramWidget)->setText( mParameter );
}

void KMFilterActionWithString::clearParamWidget( TQWidget* paramWidget ) const
{
  ((TQLineEdit*)paramWidget)->clear();
}

void KMFilterActionWithString::argsFromString( const TQString argsStr )
{
  mParameter = argsStr;
}

const TQString KMFilterActionWithString::argsAsString() const
{
  return mParameter;
}

const TQString KMFilterActionWithString::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}

//=============================================================================
//
// class KMFilterActionWithStringList
//
//=============================================================================

KMFilterActionWithStringList::KMFilterActionWithStringList( const char* aName, const TQString aLabel )
  : KMFilterActionWithString( aName, aLabel )
{
}

TQWidget* KMFilterActionWithStringList::createParamWidget( TQWidget* parent ) const
{
  TQComboBox *cb = new TQComboBox( false, parent );
  cb->insertStringList( mParameterList );
  setParamWidgetValue( cb );
  return cb;
}

void KMFilterActionWithStringList::applyParamWidgetValue( TQWidget* paramWidget )
{
  mParameter = ((TQComboBox*)paramWidget)->currentText();
}

void KMFilterActionWithStringList::setParamWidgetValue( TQWidget* paramWidget ) const
{
  int idx = mParameterList.findIndex( mParameter );
  ((TQComboBox*)paramWidget)->setCurrentItem( idx >= 0 ? idx : 0 );
}

void KMFilterActionWithStringList::clearParamWidget( TQWidget* paramWidget ) const
{
  ((TQComboBox*)paramWidget)->setCurrentItem(0);
}

void KMFilterActionWithStringList::argsFromString( const TQString argsStr )
{
  int idx = mParameterList.findIndex( argsStr );
  if ( idx < 0 ) {
    mParameterList.append( argsStr );
    idx = mParameterList.count() - 1;
  }
  mParameter = *mParameterList.at( idx );
}


//=============================================================================
//
// class KMFilterActionWithFolder
//
//=============================================================================

KMFilterActionWithFolder::KMFilterActionWithFolder( const char* aName, const TQString aLabel )
  : KMFilterAction( aName, aLabel )
{
  mFolder = 0;
}

TQWidget* KMFilterActionWithFolder::createParamWidget( TQWidget* parent ) const
{
  FolderRequester *req = new FolderRequester( parent,
      kmkernel->getKMMainWidget()->folderTree() );
  setParamWidgetValue( req );
  return req;
}

void KMFilterActionWithFolder::applyParamWidgetValue( TQWidget* paramWidget )
{
  mFolder = ((FolderRequester *)paramWidget)->folder();
  mFolderName = ((FolderRequester *)paramWidget)->folderId();
}

void KMFilterActionWithFolder::setParamWidgetValue( TQWidget* paramWidget ) const
{
  if ( mFolder )
    ((FolderRequester *)paramWidget)->setFolder( mFolder );
  else
    ((FolderRequester *)paramWidget)->setFolder( mFolderName );
}

void KMFilterActionWithFolder::clearParamWidget( TQWidget* paramWidget ) const
{
  ((FolderRequester *)paramWidget)->setFolder( kmkernel->draftsFolder() );
}

void KMFilterActionWithFolder::argsFromString( const TQString argsStr )
{
  mFolder = kmkernel->folderMgr()->findIdString( argsStr );
  if (!mFolder)
     mFolder = kmkernel->dimapFolderMgr()->findIdString( argsStr );
  if (!mFolder)
     mFolder = kmkernel->imapFolderMgr()->findIdString( argsStr );
  if (mFolder)
     mFolderName = mFolder->idString();
  else
     mFolderName = argsStr;
}

const TQString KMFilterActionWithFolder::argsAsString() const
{
  TQString result;
  if ( mFolder )
    result = mFolder->idString();
  else
    result = mFolderName;
  return result;
}

const TQString KMFilterActionWithFolder::displayString() const
{
  TQString result;
  if ( mFolder )
    result = mFolder->prettyURL();
  else
    result = mFolderName;
  return label() + " \"" + TQStyleSheet::escape( result ) + "\"";
}

bool KMFilterActionWithFolder::folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder )
{
  if ( aFolder == mFolder ) {
    mFolder = aNewFolder;
    if ( aNewFolder )
      mFolderName = mFolder->idString();
    return true;
  } else
    return false;
}

//=============================================================================
//
// class KMFilterActionWithAddress
//
//=============================================================================

KMFilterActionWithAddress::KMFilterActionWithAddress( const char* aName, const TQString aLabel )
  : KMFilterActionWithString( aName, aLabel )
{
}

TQWidget* KMFilterActionWithAddress::createParamWidget( TQWidget* parent ) const
{
  KMFilterActionWithAddressWidget *w = new KMFilterActionWithAddressWidget(parent);
  w->setText( mParameter );
  return w;
}

void KMFilterActionWithAddress::applyParamWidgetValue( TQWidget* paramWidget )
{
  mParameter = ((KMFilterActionWithAddressWidget*)paramWidget)->text();
}

void KMFilterActionWithAddress::setParamWidgetValue( TQWidget* paramWidget ) const
{
  ((KMFilterActionWithAddressWidget*)paramWidget)->setText( mParameter );
}

void KMFilterActionWithAddress::clearParamWidget( TQWidget* paramWidget ) const
{
  ((KMFilterActionWithAddressWidget*)paramWidget)->clear();
}

//=============================================================================
//
// class KMFilterActionWithCommand
//
//=============================================================================

KMFilterActionWithCommand::KMFilterActionWithCommand( const char* aName, const TQString aLabel )
  : KMFilterActionWithUrl( aName, aLabel )
{
}

TQWidget* KMFilterActionWithCommand::createParamWidget( TQWidget* parent ) const
{
  return KMFilterActionWithUrl::createParamWidget( parent );
}

void KMFilterActionWithCommand::applyParamWidgetValue( TQWidget* paramWidget )
{
  KMFilterActionWithUrl::applyParamWidgetValue( paramWidget );
}

void KMFilterActionWithCommand::setParamWidgetValue( TQWidget* paramWidget ) const
{
  KMFilterActionWithUrl::setParamWidgetValue( paramWidget );
}

void KMFilterActionWithCommand::clearParamWidget( TQWidget* paramWidget ) const
{
  KMFilterActionWithUrl::clearParamWidget( paramWidget );
}

TQString KMFilterActionWithCommand::substituteCommandLineArgsFor( KMMessage *aMsg, TQPtrList<KTempFile> & aTempFileList ) const
{
  TQString result = mParameter;
  TQValueList<int> argList;
  TQRegExp r( "%[0-9-]+" );

  // search for '%n'
  int start = -1;
  while ( ( start = r.search( result, start + 1 ) ) > 0 ) {
    int len = r.matchedLength();
    // and save the encountered 'n' in a list.
    bool OK = false;
    int n = result.mid( start + 1, len - 1 ).toInt( &OK );
    if ( OK )
      argList.append( n );
  }

  // sort the list of n's
  qHeapSort( argList );

  // and use TQString::arg to substitute filenames for the %n's.
  int lastSeen = -2;
  TQString tempFileName;
  for ( TQValueList<int>::Iterator it = argList.begin() ; it != argList.end() ; ++it ) {
    // setup temp files with check for duplicate %n's
    if ( (*it) != lastSeen ) {
      KTempFile *tf = new KTempFile();
      if ( tf->status() != 0 ) {
        tf->close();
        delete tf;
        kdDebug(5006) << "KMFilterActionWithCommand: Could not create temp file!" << endl;
        return TQString::null;
      }
      tf->setAutoDelete(true);
      aTempFileList.append( tf );
      tempFileName = tf->name();
      if ((*it) == -1)
        KPIM::kCStringToFile( aMsg->asString(), tempFileName, //###
                          false, false, false );
      else if (aMsg->numBodyParts() == 0)
        KPIM::kByteArrayToFile( aMsg->bodyDecodedBinary(), tempFileName,
                          false, false, false );
      else {
        KMMessagePart msgPart;
        aMsg->bodyPart( (*it), &msgPart );
        KPIM::kByteArrayToFile( msgPart.bodyDecodedBinary(), tempFileName,
                          false, false, false );
      }
      tf->close();
    }
    // TQString( "%0 and %1 and %1" ).arg( 0 ).arg( 1 )
    // returns "0 and 1 and %1", so we must call .arg as
    // many times as there are %n's, regardless of their multiplicity.
    if ((*it) == -1) result.replace( "%-1", tempFileName );
    else result = result.arg( tempFileName );
  }

  // And finally, replace the %{foo} with the content of the foo
  // header field:
  TQRegExp header_rx( "%\\{([a-z0-9-]+)\\}", false );
  int idx = 0;
  while ( ( idx = header_rx.search( result, idx ) ) != -1 ) {
    TQString replacement = KProcess::quote( aMsg->headerField( header_rx.cap(1).latin1() ) );
    result.replace( idx, header_rx.matchedLength(), replacement );
    idx += replacement.length();
  }

  return result;
}


KMFilterAction::ReturnCode KMFilterActionWithCommand::genericProcess(KMMessage* aMsg, bool withOutput) const
{
  Q_ASSERT( aMsg );

  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  // KProcess doesn't support a TQProcess::launch() equivalent, so
  // we must use a temp file :-(
  KTempFile * inFile = new KTempFile;
  inFile->setAutoDelete(true);

  TQPtrList<KTempFile> atmList;
  atmList.setAutoDelete(true);
  atmList.append( inFile );

  TQString commandLine = substituteCommandLineArgsFor( aMsg , atmList );
  if ( commandLine.isEmpty() )
    return ErrorButGoOn;

  // The parentheses force the creation of a subshell
  // in which the user-specified command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine =  "(" + commandLine + ") <" + inFile->name();

  // write message to file
  TQString tempFileName = inFile->name();
  KPIM::kCStringToFile( aMsg->asString(), tempFileName, //###
                  false, false, false );
  inFile->close();

  CollectingProcess shProc;
  shProc.setUseShell(true);
  shProc << commandLine;

  // run process:
  if ( !shProc.start( KProcess::Block,
                      withOutput ? KProcess::Stdout
                                 : KProcess::NoCommunication ) )
    return ErrorButGoOn;

  if ( !shProc.normalExit() || shProc.exitStatus() != 0 ) {
    return ErrorButGoOn;
  }

  if ( withOutput ) {
    // read altered message:
    TQByteArray msgText = shProc.collectedStdout();

    if ( !msgText.isEmpty() ) {
    /* If the pipe through alters the message, it could very well
       happen that it no longer has a X-UID header afterwards. That is
       unfortunate, as we need to removed the original from the folder
       using that, and look it up in the message. When the (new) message
       is uploaded, the header is stripped anyhow. */
      TQString uid = aMsg->headerField("X-UID");
      aMsg->fromByteArray( msgText );
      aMsg->setHeaderField("X-UID",uid);
    }
    else
      return ErrorButGoOn;
  }
  return GoOn;
}


//=============================================================================
//
//   Specific  Filter  Actions
//
//=============================================================================

//=============================================================================
// KMFilterActionSendReceipt - send receipt
// Return delivery receipt.
//=============================================================================
class KMFilterActionSendReceipt : public KMFilterActionWithNone
{
public:
  KMFilterActionSendReceipt();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionSendReceipt::newAction(void)
{
  return (new KMFilterActionSendReceipt);
}

KMFilterActionSendReceipt::KMFilterActionSendReceipt()
  : KMFilterActionWithNone( "confirm delivery", i18n("Confirm Delivery") )
{
}

KMFilterAction::ReturnCode KMFilterActionSendReceipt::process(KMMessage* msg) const
{
  KMMessage *receipt = msg->createDeliveryReceipt();
  if ( !receipt ) return ErrorButGoOn;

  // Queue message. This is a) so that the user can check
  // the receipt before sending and b) for speed reasons.
  kmkernel->msgSender()->send( receipt, KMail::MessageSender::SendLater );

  return GoOn;
}



//=============================================================================
// KMFilterActionSetTransport - set transport to...
// Specify mail transport (smtp server) to be used when replying to a message
//=============================================================================
class KMFilterActionTransport: public KMFilterActionWithString
{
public:
  KMFilterActionTransport();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionTransport::newAction(void)
{
  return (new KMFilterActionTransport);
}

KMFilterActionTransport::KMFilterActionTransport()
  : KMFilterActionWithString( "set transport", i18n("Set Transport To") )
{
}

KMFilterAction::ReturnCode KMFilterActionTransport::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
  msg->setHeaderField( "X-KMail-Transport", mParameter );
  return GoOn;
}


//=============================================================================
// KMFilterActionReplyTo - set Reply-To to
// Set the Reply-to header in a message
//=============================================================================
class KMFilterActionReplyTo: public KMFilterActionWithString
{
public:
  KMFilterActionReplyTo();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionReplyTo::newAction(void)
{
  return (new KMFilterActionReplyTo);
}

KMFilterActionReplyTo::KMFilterActionReplyTo()
  : KMFilterActionWithString( "set Reply-To", i18n("Set Reply-To To") )
{
  mParameter = "";
}

KMFilterAction::ReturnCode KMFilterActionReplyTo::process(KMMessage* msg) const
{
  msg->setHeaderField( "Reply-To", mParameter );
  return GoOn;
}



//=============================================================================
// KMFilterActionIdentity - set identity to
// Specify Identity to be used when replying to a message
//=============================================================================
class KMFilterActionIdentity: public KMFilterActionWithUOID
{
public:
  KMFilterActionIdentity();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction();

  TQWidget * createParamWidget( TQWidget * parent ) const;
  void applyParamWidgetValue( TQWidget * parent );
  void setParamWidgetValue( TQWidget * parent ) const;
  void clearParamWidget( TQWidget * param ) const;
};

KMFilterAction* KMFilterActionIdentity::newAction()
{
  return (new KMFilterActionIdentity);
}

KMFilterActionIdentity::KMFilterActionIdentity()
  : KMFilterActionWithUOID( "set identity", i18n("Set Identity To") )
{
  mParameter = kmkernel->identityManager()->defaultIdentity().uoid();
}

KMFilterAction::ReturnCode KMFilterActionIdentity::process(KMMessage* msg) const
{
  msg->setHeaderField( "X-KMail-Identity", TQString::number( mParameter ) );
  return GoOn;
}

TQWidget * KMFilterActionIdentity::createParamWidget( TQWidget * parent ) const
{
  KPIM::IdentityCombo * ic = new KPIM::IdentityCombo( kmkernel->identityManager(), parent );
  ic->setCurrentIdentity( mParameter );
  return ic;
}

void KMFilterActionIdentity::applyParamWidgetValue( TQWidget * paramWidget )
{
  KPIM::IdentityCombo * ic = dynamic_cast<KPIM::IdentityCombo*>( paramWidget );
  assert( ic );
  mParameter = ic->currentIdentity();
}

void KMFilterActionIdentity::clearParamWidget( TQWidget * paramWidget ) const
{
  KPIM::IdentityCombo * ic = dynamic_cast<KPIM::IdentityCombo*>( paramWidget );
  assert( ic );
  ic->setCurrentItem( 0 );
  //ic->setCurrentIdentity( kmkernel->identityManager()->defaultIdentity() );
}

void KMFilterActionIdentity::setParamWidgetValue( TQWidget * paramWidget ) const
{
  KPIM::IdentityCombo * ic = dynamic_cast<KPIM::IdentityCombo*>( paramWidget );
  assert( ic );
  ic->setCurrentIdentity( mParameter );
}

//=============================================================================
// KMFilterActionSetStatus - set status to
// Set the status of messages
//=============================================================================
class KMFilterActionSetStatus: public KMFilterActionWithStringList
{
public:
  KMFilterActionSetStatus();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;

  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const TQString argsStr );
  virtual const TQString argsAsString() const;
  virtual const TQString displayString() const;
};


static const KMMsgStatus stati[] =
{
  KMMsgStatusFlag,
  KMMsgStatusRead,
  KMMsgStatusUnread,
  KMMsgStatusReplied,
  KMMsgStatusForwarded,
  KMMsgStatusOld,
  KMMsgStatusNew,
  KMMsgStatusWatched,
  KMMsgStatusIgnored,
  KMMsgStatusSpam,
  KMMsgStatusHam
};
static const int StatiCount = sizeof( stati ) / sizeof( KMMsgStatus );

KMFilterAction* KMFilterActionSetStatus::newAction()
{
  return (new KMFilterActionSetStatus);
}

KMFilterActionSetStatus::KMFilterActionSetStatus()
  : KMFilterActionWithStringList( "set status", i18n("Mark As") )
{
  // if you change this list, also update
  // KMFilterActionSetStatus::stati above
  mParameterList.append( "" );
  mParameterList.append( i18n("msg status","Important") );
  mParameterList.append( i18n("msg status","Read") );
  mParameterList.append( i18n("msg status","Unread") );
  mParameterList.append( i18n("msg status","Replied") );
  mParameterList.append( i18n("msg status","Forwarded") );
  mParameterList.append( i18n("msg status","Old") );
  mParameterList.append( i18n("msg status","New") );
  mParameterList.append( i18n("msg status","Watched") );
  mParameterList.append( i18n("msg status","Ignored") );
  mParameterList.append( i18n("msg status","Spam") );
  mParameterList.append( i18n("msg status","Ham") );

  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionSetStatus::process(KMMessage* msg) const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  KMMsgStatus status = stati[idx-1] ;
  msg->setStatus( status );
  return GoOn;
}

bool KMFilterActionSetStatus::requiresBody(KMMsgBase*) const
{
  return false;
}

void KMFilterActionSetStatus::argsFromString( const TQString argsStr )
{
  if ( argsStr.length() == 1 ) {
    for ( int i = 0 ; i < StatiCount ; i++ )
      if ( KMMsgBase::statusToStr(stati[i])[0] == argsStr[0] ) {
        mParameter = *mParameterList.at(i+1);
        return;
      }
  }
  mParameter = *mParameterList.at(0);
}

const TQString KMFilterActionSetStatus::argsAsString() const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return TQString::null;

  KMMsgStatus status = stati[idx-1];
  return KMMsgBase::statusToStr(status);
}

const TQString KMFilterActionSetStatus::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionFakeDisposition - send fake MDN
// Sends a fake MDN or forces an ignore.
//=============================================================================
class KMFilterActionFakeDisposition: public KMFilterActionWithStringList
{
public:
  KMFilterActionFakeDisposition();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction() {
    return (new KMFilterActionFakeDisposition);
  }

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const TQString argsStr );
  virtual const TQString argsAsString() const;
  virtual const TQString displayString() const;
};


// if you change this list, also update
// the count in argsFromString
static const KMime::MDN::DispositionType mdns[] =
{
  KMime::MDN::Displayed,
  KMime::MDN::Deleted,
  KMime::MDN::Dispatched,
  KMime::MDN::Processed,
  KMime::MDN::Denied,
  KMime::MDN::Failed,
};
static const int numMDNs = sizeof mdns / sizeof *mdns;


KMFilterActionFakeDisposition::KMFilterActionFakeDisposition()
  : KMFilterActionWithStringList( "fake mdn", i18n("Send Fake MDN") )
{
  // if you change this list, also update
  // mdns above
  mParameterList.append( "" );
  mParameterList.append( i18n("MDN type","Ignore") );
  mParameterList.append( i18n("MDN type","Displayed") );
  mParameterList.append( i18n("MDN type","Deleted") );
  mParameterList.append( i18n("MDN type","Dispatched") );
  mParameterList.append( i18n("MDN type","Processed") );
  mParameterList.append( i18n("MDN type","Denied") );
  mParameterList.append( i18n("MDN type","Failed") );

  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionFakeDisposition::process(KMMessage* msg) const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  if ( idx == 1 ) // ignore
    msg->setMDNSentState( KMMsgMDNIgnore );
  else // send
    sendMDN( msg, mdns[idx-2] ); // skip first two entries: "" and "ignore"
  return GoOn;
}

void KMFilterActionFakeDisposition::argsFromString( const TQString argsStr )
{
  if ( argsStr.length() == 1 ) {
    if ( argsStr[0] == 'I' ) { // ignore
      mParameter = *mParameterList.at(1);
      return;
    }
    for ( int i = 0 ; i < numMDNs ; i++ )
      if ( char(mdns[i]) == argsStr[0] ) { // send
        mParameter = *mParameterList.at(i+2);
        return;
      }
  }
  mParameter = *mParameterList.at(0);
}

const TQString KMFilterActionFakeDisposition::argsAsString() const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return TQString::null;

  return TQString( TQChar( idx < 2 ? 'I' : char(mdns[idx-2]) ) );
}

const TQString KMFilterActionFakeDisposition::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionRemoveHeader - remove header
// Remove all instances of the given header field.
//=============================================================================
class KMFilterActionRemoveHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionRemoveHeader();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual TQWidget* createParamWidget( TQWidget* parent ) const;
  virtual void setParamWidgetValue( TQWidget* paramWidget ) const;

  static KMFilterAction* newAction();
};

KMFilterAction* KMFilterActionRemoveHeader::newAction()
{
  return (new KMFilterActionRemoveHeader);
}

KMFilterActionRemoveHeader::KMFilterActionRemoveHeader()
  : KMFilterActionWithStringList( "remove header", i18n("Remove Header") )
{
  mParameterList << ""
                 << "Reply-To"
                 << "Delivered-To"
                 << "X-KDE-PR-Message"
                 << "X-KDE-PR-Package"
                 << "X-KDE-PR-Keywords";
  mParameter = *mParameterList.at(0);
}

TQWidget* KMFilterActionRemoveHeader::createParamWidget( TQWidget* parent ) const
{
  TQComboBox *cb = new TQComboBox( true/*editable*/, parent );
  cb->setInsertionPolicy( TQComboBox::AtBottom );
  setParamWidgetValue( cb );
  return cb;
}

KMFilterAction::ReturnCode KMFilterActionRemoveHeader::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  while ( !msg->headerField( mParameter.latin1() ).isEmpty() )
    msg->removeHeaderField( mParameter.latin1() );
  return GoOn;
}

void KMFilterActionRemoveHeader::setParamWidgetValue( TQWidget* paramWidget ) const
{
  TQComboBox * cb = dynamic_cast<TQComboBox*>(paramWidget);
  Q_ASSERT( cb );

  int idx = mParameterList.findIndex( mParameter );
  cb->clear();
  cb->insertStringList( mParameterList );
  if ( idx < 0 ) {
    cb->insertItem( mParameter );
    cb->setCurrentItem( cb->count() - 1 );
  } else {
    cb->setCurrentItem( idx );
  }
}


//=============================================================================
// KMFilterActionAddHeader - add header
// Add a header with the given value.
//=============================================================================
class KMFilterActionAddHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionAddHeader();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual TQWidget* createParamWidget( TQWidget* parent ) const;
  virtual void setParamWidgetValue( TQWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( TQWidget* paramWidget );
  virtual void clearParamWidget( TQWidget* paramWidget ) const;

  virtual const TQString argsAsString() const;
  virtual void argsFromString( const TQString argsStr );

  virtual const TQString displayString() const;

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionAddHeader);
  }
private:
  TQString mValue;
};

KMFilterActionAddHeader::KMFilterActionAddHeader()
  : KMFilterActionWithStringList( "add header", i18n("Add Header") )
{
  mParameterList << ""
                 << "Reply-To"
                 << "Delivered-To"
                 << "X-KDE-PR-Message"
                 << "X-KDE-PR-Package"
                 << "X-KDE-PR-Keywords";
  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionAddHeader::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  msg->setHeaderField( mParameter.latin1(), mValue );
  return GoOn;
}

TQWidget* KMFilterActionAddHeader::createParamWidget( TQWidget* parent ) const
{
  TQWidget *w = new TQWidget( parent );
  TQHBoxLayout *hbl = new TQHBoxLayout( w );
  hbl->setSpacing( 4 );
  TQComboBox *cb = new TQComboBox( true, w, "combo" );
  cb->setInsertionPolicy( TQComboBox::AtBottom );
  hbl->addWidget( cb, 0 /* stretch */ );
  TQLabel *l = new TQLabel( i18n("With value:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );
  TQLineEdit *le = new KLineEdit( w, "ledit" );
  hbl->addWidget( le, 1 );
  setParamWidgetValue( w );
  return w;
}

void KMFilterActionAddHeader::setParamWidgetValue( TQWidget* paramWidget ) const
{
  int idx = mParameterList.findIndex( mParameter );
  TQComboBox *cb = (TQComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  cb->clear();
  cb->insertStringList( mParameterList );
  if ( idx < 0 ) {
    cb->insertItem( mParameter );
    cb->setCurrentItem( cb->count() - 1 );
  } else {
    cb->setCurrentItem( idx );
  }
  TQLineEdit *le = (TQLineEdit*)paramWidget->child("ledit");
  Q_ASSERT( le );
  le->setText( mValue );
}

void KMFilterActionAddHeader::applyParamWidgetValue( TQWidget* paramWidget )
{
  TQComboBox *cb = (TQComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  TQLineEdit *le = (TQLineEdit*)paramWidget->child("ledit");
  Q_ASSERT( le );
  mValue = le->text();
}

void KMFilterActionAddHeader::clearParamWidget( TQWidget* paramWidget ) const
{
  TQComboBox *cb = (TQComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  cb->setCurrentItem(0);
  TQLineEdit *le = (TQLineEdit*)paramWidget->child("ledit");
  Q_ASSERT( le );
  le->clear();
}

const TQString KMFilterActionAddHeader::argsAsString() const
{
  TQString result = mParameter;
  result += '\t';
  result += mValue;

  return result;
}

const TQString KMFilterActionAddHeader::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}

void KMFilterActionAddHeader::argsFromString( const TQString argsStr )
{
  TQStringList l = TQStringList::split( '\t', argsStr, true /*allow empty entries*/ );
  TQString s;
  if ( l.count() < 2 ) {
    s = l[0];
    mValue = "";
  } else {
    s = l[0];
    mValue = l[1];
  }

  int idx = mParameterList.findIndex( s );
  if ( idx < 0 ) {
    mParameterList.append( s );
    idx = mParameterList.count() - 1;
  }
  mParameter = *mParameterList.at( idx );
}


//=============================================================================
// KMFilterActionRewriteHeader - rewrite header
// Rewrite a header using a regexp.
//=============================================================================
class KMFilterActionRewriteHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionRewriteHeader();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual TQWidget* createParamWidget( TQWidget* parent ) const;
  virtual void setParamWidgetValue( TQWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( TQWidget* paramWidget );
  virtual void clearParamWidget( TQWidget* paramWidget ) const;

  virtual const TQString argsAsString() const;
  virtual void argsFromString( const TQString argsStr );

  virtual const TQString displayString() const;

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionRewriteHeader);
  }
private:
  KRegExp3 mRegExp;
  TQString mReplacementString;
};

KMFilterActionRewriteHeader::KMFilterActionRewriteHeader()
  : KMFilterActionWithStringList( "rewrite header", i18n("Rewrite Header") )
{
  mParameterList << ""
                 << "Subject"
                 << "Reply-To"
                 << "Delivered-To"
                 << "X-KDE-PR-Message"
                 << "X-KDE-PR-Package"
                 << "X-KDE-PR-Keywords";
  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionRewriteHeader::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() || !mRegExp.isValid() )
    return ErrorButGoOn;

  KRegExp3 rx = mRegExp; // KRegExp3::replace is not const.

  TQString newValue = rx.replace( msg->headerField( mParameter.latin1() ),
                                     mReplacementString );

  msg->setHeaderField( mParameter.latin1(), newValue );
  return GoOn;
}

TQWidget* KMFilterActionRewriteHeader::createParamWidget( TQWidget* parent ) const
{
  TQWidget *w = new TQWidget( parent );
  TQHBoxLayout *hbl = new TQHBoxLayout( w );
  hbl->setSpacing( 4 );

  TQComboBox *cb = new TQComboBox( true, w, "combo" );
  cb->setInsertionPolicy( TQComboBox::AtBottom );
  hbl->addWidget( cb, 0 /* stretch */ );

  TQLabel *l = new TQLabel( i18n("Replace:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  RegExpLineEdit *rele = new RegExpLineEdit( w, "search" );
  hbl->addWidget( rele, 1 );

  l = new TQLabel( i18n("With:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  TQLineEdit *le = new KLineEdit( w, "replace" );
  hbl->addWidget( le, 1 );

  setParamWidgetValue( w );
  return w;
}

void KMFilterActionRewriteHeader::setParamWidgetValue( TQWidget* paramWidget ) const
{
  int idx = mParameterList.findIndex( mParameter );
  TQComboBox *cb = (TQComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );

  cb->clear();
  cb->insertStringList( mParameterList );
  if ( idx < 0 ) {
    cb->insertItem( mParameter );
    cb->setCurrentItem( cb->count() - 1 );
  } else {
    cb->setCurrentItem( idx );
  }

  RegExpLineEdit *rele = (RegExpLineEdit*)paramWidget->child("search");
  Q_ASSERT( rele );
  rele->setText( mRegExp.pattern() );

  TQLineEdit *le = (TQLineEdit*)paramWidget->child("replace");
  Q_ASSERT( le );
  le->setText( mReplacementString );
}

void KMFilterActionRewriteHeader::applyParamWidgetValue( TQWidget* paramWidget )
{
  TQComboBox *cb = (TQComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  RegExpLineEdit *rele = (RegExpLineEdit*)paramWidget->child("search");
  Q_ASSERT( rele );
  mRegExp.setPattern( rele->text() );

  TQLineEdit *le = (TQLineEdit*)paramWidget->child("replace");
  Q_ASSERT( le );
  mReplacementString = le->text();
}

void KMFilterActionRewriteHeader::clearParamWidget( TQWidget* paramWidget ) const
{
  TQComboBox *cb = (TQComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  cb->setCurrentItem(0);

  RegExpLineEdit *rele = (RegExpLineEdit*)paramWidget->child("search");
  Q_ASSERT( rele );
  rele->clear();

  TQLineEdit *le = (TQLineEdit*)paramWidget->child("replace");
  Q_ASSERT( le );
  le->clear();
}

const TQString KMFilterActionRewriteHeader::argsAsString() const
{
  TQString result = mParameter;
  result += '\t';
  result += mRegExp.pattern();
  result += '\t';
  result += mReplacementString;

  return result;
}

const TQString KMFilterActionRewriteHeader::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}

void KMFilterActionRewriteHeader::argsFromString( const TQString argsStr )
{
  TQStringList l = TQStringList::split( '\t', argsStr, true /*allow empty entries*/ );
  TQString s;

  s = l[0];
  mRegExp.setPattern( l[1] );
  mReplacementString = l[2];

  int idx = mParameterList.findIndex( s );
  if ( idx < 0 ) {
    mParameterList.append( s );
    idx = mParameterList.count() - 1;
  }
  mParameter = *mParameterList.at( idx );
}


//=============================================================================
// KMFilterActionMove - move into folder
// File message into another mail folder
//=============================================================================
class KMFilterActionMove: public KMFilterActionWithFolder
{
public:
  KMFilterActionMove();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionMove::newAction(void)
{
  return (new KMFilterActionMove);
}

KMFilterActionMove::KMFilterActionMove()
  : KMFilterActionWithFolder( "transfer", i18n("Move Into Folder") )
{
}

KMFilterAction::ReturnCode KMFilterActionMove::process(KMMessage* msg) const
{
  if ( !mFolder )
    return ErrorButGoOn;

  ActionScheduler *handler = MessageProperty::filterHandler( msg );
  if (handler) {
    MessageProperty::setFilterFolder( msg, mFolder );
  } else {
    // The old filtering system does not support online imap targets.
    // Skip online imap targets when using the old system.
    KMFolder *check;
    check = kmkernel->imapFolderMgr()->findIdString( argsAsString() );
    if (mFolder && (check != mFolder)) {
      MessageProperty::setFilterFolder( msg, mFolder );
    }
  }
  return GoOn;
}

bool KMFilterActionMove::requiresBody(KMMsgBase*) const
{
    return false; //iff mFolder->folderMgr == msgBase->parent()->folderMgr;
}


//=============================================================================
// KMFilterActionCopy - copy into folder
// Copy message into another mail folder
//=============================================================================
class KMFilterActionCopy: public KMFilterActionWithFolder
{
public:
  KMFilterActionCopy();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual void processAsync(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionCopy::newAction(void)
{
  return (new KMFilterActionCopy);
}

KMFilterActionCopy::KMFilterActionCopy()
  : KMFilterActionWithFolder( "copy", i18n("Copy Into Folder") )
{
}

KMFilterAction::ReturnCode KMFilterActionCopy::process(KMMessage* msg) const
{
  // TODO opening and closing the folder is a trade off.
  // Perhaps Copy is a seldomly used action for now,
  // but I gonna look at improvements ASAP.
  if ( !mFolder )
    return ErrorButGoOn;
  if ( mFolder && mFolder->open( "filtercopy" ) != 0 )
    return ErrorButGoOn;

  // copy the message 1:1
  KMMessage* msgCopy = new KMMessage( new DwMessage( *msg->asDwMessage() ) );

  int index;
  int rc = mFolder->addMsg(msgCopy, &index);
  if (rc == 0 && index != -1)
    mFolder->unGetMsg( index );
  mFolder->close("filtercopy");

  return GoOn;
}

void KMFilterActionCopy::processAsync(KMMessage* msg) const
{
  // FIXME remove the debug output
  kdDebug(5006) << "##### KMFilterActionCopy::processAsync(KMMessage* msg)" << endl;
  ActionScheduler *handler = MessageProperty::filterHandler( msg );

  KMCommand *cmd = new KMCopyCommand( mFolder, msg );
  TQObject::connect( cmd, TQT_SIGNAL( completed( KMCommand * ) ),
                    handler, TQT_SLOT( copyMessageFinished( KMCommand * ) ) );
  cmd->start();
}

bool KMFilterActionCopy::requiresBody(KMMsgBase*) const
{
    return true;
}


//=============================================================================
// KMFilterActionForward - forward to
// Forward message to another user
//=============================================================================
class KMFilterActionForward: public KMFilterActionWithAddress
{
public:
  KMFilterActionForward();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionForward::newAction(void)
{
  return (new KMFilterActionForward);
}

KMFilterActionForward::KMFilterActionForward()
  : KMFilterActionWithAddress( "forward", i18n("Forward To") )
{
}

KMFilterAction::ReturnCode KMFilterActionForward::process(KMMessage* aMsg) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  // avoid endless loops when this action is used in a filter
  // which applies to sent messages
  if ( KMMessage::addressIsInAddressList( mParameter, aMsg->to() ) )
    return ErrorButGoOn;

  // Create the forwarded message by hand to make forwarding of messages with
  // attachments work.
  // Note: This duplicates a lot of code from KMMessage::createForward() and
  //       KMComposeWin::applyChanges().
  // ### FIXME: Remove the code duplication again.

  KMMessage* msg = new KMMessage;

  msg->initFromMessage( aMsg );

  // TQString st = TQString::fromUtf8( aMsg->createForwardBody() );

  TemplateParser parser( msg, TemplateParser::Forward,
    aMsg->body(), false, false, false, false);
  parser.process( aMsg );

  QCString
    encoding = KMMsgBase::autoDetectCharset( aMsg->charset(),
                                             KMMessage::preferredCharsets(),
                                             msg->body() );
  if( encoding.isEmpty() )
    encoding = "utf-8";
  TQCString str = KMMsgBase::codecForName( encoding )->fromUnicode( msg->body() );

  msg->setCharset( encoding );
  msg->setTo( mParameter );
  msg->setSubject( "Fwd: " + aMsg->subject() );

  bool isQP = kmkernel->msgSender()->sendQuotedPrintable();

  if( aMsg->numBodyParts() == 0 )
  {
    msg->setAutomaticFields( true );
    msg->setHeaderField( "Content-Type", "text/plain" );
    // msg->setCteStr( isQP ? "quoted-printable": "8bit" );
    TQValueList<int> dummy;
    msg->setBodyAndGuessCte(str, dummy, !isQP);
    msg->setCharset( encoding );
    if( isQP )
      msg->setBodyEncoded( str );
    else
      msg->setBody( str );
  }
  else
  {
    KMMessagePart bodyPart, msgPart;

    msg->removeHeaderField( "Content-Type" );
    msg->removeHeaderField( "Content-Transfer-Encoding" );
    msg->setAutomaticFields( true );
    msg->setBody( "This message is in MIME format.\n\n" );

    bodyPart.setTypeStr( "text" );
    bodyPart.setSubtypeStr( "plain" );
    // bodyPart.setCteStr( isQP ? "quoted-printable": "8bit" );
    TQValueList<int> dummy;
    bodyPart.setBodyAndGuessCte(str, dummy, !isQP);
    bodyPart.setCharset( encoding );
    bodyPart.setBodyEncoded( str );
    msg->addBodyPart( &bodyPart );

    for( int i = 0; i < aMsg->numBodyParts(); i++ )
    {
      aMsg->bodyPart( i, &msgPart );
      if( i > 0 || qstricmp( msgPart.typeStr(), "text" ) != 0 )
        msg->addBodyPart( &msgPart );
    }
  }
  msg->cleanupHeader();
  msg->link( aMsg, KMMsgStatusForwarded );

  sendMDN( aMsg, KMime::MDN::Dispatched );

  if ( !kmkernel->msgSender()->send( msg, KMail::MessageSender::SendLater ) ) {
    kdDebug(5006) << "KMFilterAction: could not forward message (sending failed)" << endl;
    return ErrorButGoOn; // error: couldn't send
  }
  return GoOn;
}


//=============================================================================
// KMFilterActionRedirect - redirect to
// Redirect message to another user
//=============================================================================
class KMFilterActionRedirect: public KMFilterActionWithAddress
{
public:
  KMFilterActionRedirect();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionRedirect::newAction(void)
{
  return (new KMFilterActionRedirect);
}

KMFilterActionRedirect::KMFilterActionRedirect()
  : KMFilterActionWithAddress( "redirect", i18n("Redirect To") )
{
}

KMFilterAction::ReturnCode KMFilterActionRedirect::process(KMMessage* aMsg) const
{
  KMMessage* msg;
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  msg = aMsg->createRedirect( mParameter );

  sendMDN( aMsg, KMime::MDN::Dispatched );

  if ( !kmkernel->msgSender()->send( msg, KMail::MessageSender::SendLater ) ) {
    kdDebug(5006) << "KMFilterAction: could not redirect message (sending failed)" << endl;
    return ErrorButGoOn; // error: couldn't send
  }
  return GoOn;
}


//=============================================================================
// KMFilterActionExec - execute command
// Execute a shell command
//=============================================================================
class KMFilterActionExec : public KMFilterActionWithCommand
{
public:
  KMFilterActionExec();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionExec::newAction(void)
{
  return (new KMFilterActionExec());
}

KMFilterActionExec::KMFilterActionExec()
  : KMFilterActionWithCommand( "execute", i18n("Execute Command") )
{
}

KMFilterAction::ReturnCode KMFilterActionExec::process(KMMessage *aMsg) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, false ); // ignore output
}

//=============================================================================
// KMFilterActionExtFilter - use external filter app
// External message filter: executes a shell command with message
// on stdin; altered message is expected on stdout.
//=============================================================================

#include <weaver.h>
class PipeJob : public KPIM::ThreadWeaver::Job
{
  public:
    PipeJob(TQObject* parent = 0 , const char* name = 0, KMMessage* aMsg = 0, TQString cmd = 0, TQString tempFileName = 0 )
      : Job (parent, name),
        mTempFileName(tempFileName),
        mCmd(cmd),
        mMsg( aMsg )
    {
    }

    ~PipeJob() {}
    virtual void processEvent( KPIM::ThreadWeaver::Event *ev )
    {
      KPIM::ThreadWeaver::Job::processEvent( ev );
      if ( ev->action() == KPIM::ThreadWeaver::Event::JobFinished )
        deleteLater( );
    }
  protected:
    void run()
    {
      KPIM::ThreadWeaver::debug (1, "PipeJob::run: doing it .\n");
      FILE *p;
      TQByteArray ba;

      // backup the serial number in case the header gets lost
      TQString origSerNum = mMsg->headerField( "X-KMail-Filtered" );

      p = popen(TQFile::encodeName(mCmd), "r");
      int len =100;
      char buffer[100];
      // append data to ba:
      while (true)  {
        if (! fgets( buffer, len, p ) ) break;
        int oldsize = ba.size();
        ba.resize( oldsize + strlen(buffer) );
        qmemmove( ba.begin() + oldsize, buffer, strlen(buffer) );
      }
      pclose(p);
      if ( !ba.isEmpty() ) {
        KPIM::ThreadWeaver::debug (1, "PipeJob::run: %s", TQString(ba).latin1() );
        KMFolder *filterFolder =  mMsg->parent();
        ActionScheduler *handler = MessageProperty::filterHandler( mMsg->getMsgSerNum() );

        mMsg->fromByteArray( ba );
        if ( !origSerNum.isEmpty() )
          mMsg->setHeaderField( "X-KMail-Filtered", origSerNum );
        if ( filterFolder && handler ) {
          bool oldStatus = handler->ignoreChanges( true );
          filterFolder->take( filterFolder->find( mMsg ) );
          filterFolder->addMsg( mMsg );
          handler->ignoreChanges( oldStatus );
        } else {
          kdDebug(5006) << "Warning: Cannot refresh the message from the external filter." << endl;
        }
      }

      KPIM::ThreadWeaver::debug (1, "PipeJob::run: done.\n" );
      // unlink the tempFile
      TQFile::remove(mTempFileName);
    }
    TQString mTempFileName;
    TQString mCmd;
    KMMessage *mMsg;
};

class KMFilterActionExtFilter: public KMFilterActionWithCommand
{
public:
  KMFilterActionExtFilter();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual void processAsync(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionExtFilter::newAction(void)
{
  return (new KMFilterActionExtFilter);
}

KMFilterActionExtFilter::KMFilterActionExtFilter()
  : KMFilterActionWithCommand( "filter app", i18n("Pipe Through") )
{
}
KMFilterAction::ReturnCode KMFilterActionExtFilter::process(KMMessage* aMsg) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, true ); // use output
}

void KMFilterActionExtFilter::processAsync(KMMessage* aMsg) const
{

  ActionScheduler *handler = MessageProperty::filterHandler( aMsg->getMsgSerNum() );
  KTempFile * inFile = new KTempFile;
  inFile->setAutoDelete(false);

  TQPtrList<KTempFile> atmList;
  atmList.setAutoDelete(true);
  atmList.append( inFile );

  TQString commandLine = substituteCommandLineArgsFor( aMsg , atmList );
  if ( commandLine.isEmpty() )
    handler->actionMessage( ErrorButGoOn );

  // The parentheses force the creation of a subshell
  // in which the user-specified command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine =  "(" + commandLine + ") <" + inFile->name();

  // write message to file
  TQString tempFileName = inFile->name();
  KPIM::kCStringToFile( aMsg->asString(), tempFileName, //###
      false, false, false );
  inFile->close();

  PipeJob *job = new PipeJob(0, 0, aMsg, commandLine, tempFileName);
  TQObject::connect ( job, TQT_SIGNAL( done() ), handler, TQT_SLOT( actionMessage() ) );
  kmkernel->weaver()->enqueue(job);
}

//=============================================================================
// KMFilterActionExecSound - execute command
// Execute a sound
//=============================================================================
class KMFilterActionExecSound : public KMFilterActionWithTest
{
public:
  KMFilterActionExecSound();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;
  static KMFilterAction* newAction(void);
};

KMFilterActionWithTest::KMFilterActionWithTest( const char* aName, const TQString aLabel )
  : KMFilterAction( aName, aLabel )
{
}

KMFilterActionWithTest::~KMFilterActionWithTest()
{
}

TQWidget* KMFilterActionWithTest::createParamWidget( TQWidget* parent ) const
{
  KMSoundTestWidget *le = new KMSoundTestWidget(parent);
  le->setUrl( mParameter );
  return le;
}


void KMFilterActionWithTest::applyParamWidgetValue( TQWidget* paramWidget )
{
  mParameter = ((KMSoundTestWidget*)paramWidget)->url();
}

void KMFilterActionWithTest::setParamWidgetValue( TQWidget* paramWidget ) const
{
  ((KMSoundTestWidget*)paramWidget)->setUrl( mParameter );
}

void KMFilterActionWithTest::clearParamWidget( TQWidget* paramWidget ) const
{
  ((KMSoundTestWidget*)paramWidget)->clear();
}

void KMFilterActionWithTest::argsFromString( const TQString argsStr )
{
  mParameter = argsStr;
}

const TQString KMFilterActionWithTest::argsAsString() const
{
  return mParameter;
}

const TQString KMFilterActionWithTest::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}


KMFilterActionExecSound::KMFilterActionExecSound()
  : KMFilterActionWithTest( "play sound", i18n("Play Sound") )
{
}

KMFilterAction* KMFilterActionExecSound::newAction(void)
{
  return (new KMFilterActionExecSound());
}

KMFilterAction::ReturnCode KMFilterActionExecSound::process(KMMessage*) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
  TQString play = mParameter;
  TQString file = TQString::fromLatin1("file:");
  if (mParameter.startsWith(file))
    play = mParameter.mid(file.length());
  KAudioPlayer::play(TQFile::encodeName(play));
  return GoOn;
}

bool KMFilterActionExecSound::requiresBody(KMMsgBase*) const
{
  return false;
}

KMFilterActionWithUrl::KMFilterActionWithUrl( const char* aName, const TQString aLabel )
  : KMFilterAction( aName, aLabel )
{
}

KMFilterActionWithUrl::~KMFilterActionWithUrl()
{
}

TQWidget* KMFilterActionWithUrl::createParamWidget( TQWidget* parent ) const
{
  KURLRequester *le = new KURLRequester(parent);
  le->setURL( mParameter );
  return le;
}


void KMFilterActionWithUrl::applyParamWidgetValue( TQWidget* paramWidget )
{
  mParameter = ((KURLRequester*)paramWidget)->url();
}

void KMFilterActionWithUrl::setParamWidgetValue( TQWidget* paramWidget ) const
{
  ((KURLRequester*)paramWidget)->setURL( mParameter );
}

void KMFilterActionWithUrl::clearParamWidget( TQWidget* paramWidget ) const
{
  ((KURLRequester*)paramWidget)->clear();
}

void KMFilterActionWithUrl::argsFromString( const TQString argsStr )
{
  mParameter = argsStr;
}

const TQString KMFilterActionWithUrl::argsAsString() const
{
  return mParameter;
}

const TQString KMFilterActionWithUrl::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + TQStyleSheet::escape( argsAsString() ) + "\"";
}


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
void KMFilterActionDict::init(void)
{
  insert( KMFilterActionMove::newAction );
  insert( KMFilterActionCopy::newAction );
  insert( KMFilterActionIdentity::newAction );
  insert( KMFilterActionSetStatus::newAction );
  insert( KMFilterActionFakeDisposition::newAction );
  insert( KMFilterActionTransport::newAction );
  insert( KMFilterActionReplyTo::newAction );
  insert( KMFilterActionForward::newAction );
  insert( KMFilterActionRedirect::newAction );
  insert( KMFilterActionSendReceipt::newAction );
  insert( KMFilterActionExec::newAction );
  insert( KMFilterActionExtFilter::newAction );
  insert( KMFilterActionRemoveHeader::newAction );
  insert( KMFilterActionAddHeader::newAction );
  insert( KMFilterActionRewriteHeader::newAction );
  insert( KMFilterActionExecSound::newAction );
  // Register custom filter actions below this line.
}
// The int in the TQDict constructor (41) must be a prime
// and should be greater than the double number of KMFilterAction types
KMFilterActionDict::KMFilterActionDict()
  : TQDict<KMFilterActionDesc>(41)
{
  mList.setAutoDelete(true);
  init();
}

void KMFilterActionDict::insert( KMFilterActionNewFunc aNewFunc )
{
  KMFilterAction *action = aNewFunc();
  KMFilterActionDesc* desc = new KMFilterActionDesc;
  desc->name = action->name();
  desc->label = action->label();
  desc->create = aNewFunc;
  TQDict<KMFilterActionDesc>::insert( desc->name, desc );
  TQDict<KMFilterActionDesc>::insert( desc->label, desc );
  mList.append( desc );
  delete action;
}
