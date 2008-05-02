/*
    qgpgmejob.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004, 2007 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "qgpgmejob.h"
#include "qgpgmeprogresstokenmapper.h"

#include "libkleo/kleo/job.h"

#include <qgpgme/eventloopinteractor.h>
#include <qgpgme/dataprovider.h>

#include <gpgme++/context.h>
#include <gpgme++/data.h>
#include <gpgme++/exception.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kiconloader.h>

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QEventLoop>

#include <algorithm>

#include <assert.h>
#include <string.h>

namespace {
  class InvarianceChecker {
  public:
#ifdef NDEBUG
    InvarianceChecker( const Kleo::QGpgMEJob * ) {}
#else
    InvarianceChecker( const Kleo::QGpgMEJob * job )
      : _this( job )
    {
      assert( _this );
      _this->checkInvariants();
    }
    ~InvarianceChecker() {
      _this->checkInvariants();
    }
  private:
    const Kleo::QGpgMEJob * _this;
#endif
  };
}

Kleo::QGpgMEJob::QGpgMEJob( Kleo::Job * _this, GpgME::Context * context )
  : GpgME::ProgressProvider(),
    GpgME::PassphraseProvider(),
    mThis( _this ),
    mCtx( context ),
    mInData( 0 ),
    mInDataDataProvider( 0 ),
    mOutData( 0 ),
    mOutDataDataProvider( 0 ),
    mPatterns( 0 ),
    mReplacedPattern( 0 ),
    mNumPatterns( 0 ),
    mChunkSize( 1024 ),
    mPatternStartIndex( 0 ), mPatternEndIndex( 0 ),
    mEventLoop( 0 )
{
  InvarianceChecker check( this );
  assert( context );
  QObject::connect( QGpgME::EventLoopInteractor::instance(), SIGNAL(aboutToDestroy()),
		    _this, SLOT(slotCancel()) );
  context->setProgressProvider( this );
  // (mmutz) work around a gpgme bug in versions at least <= 0.9.0.
  //         These versions will return GPG_ERR_NOT_IMPLEMENTED from
  //         a CMS sign operation when a passphrase callback is set.
  if ( context->protocol() == GpgME::OpenPGP )
    context->setPassphraseProvider( this );
}

void Kleo::QGpgMEJob::checkInvariants() const {
#ifndef NDEBUG
  if ( mPatterns ) {
    assert( mPatterns[mNumPatterns] == 0 );
    if ( mPatternEndIndex > 0 ) {
      assert( mPatternEndIndex > mPatternStartIndex );
      assert( mPatternEndIndex - mPatternStartIndex == mChunkSize );
    } else {
      assert( mPatternEndIndex == mPatternStartIndex );
    }
    if ( mPatternEndIndex < mNumPatterns ) {
      assert( mPatterns[mPatternEndIndex] == 0 );
      assert( mReplacedPattern != 0 );
    } else {
      assert( mReplacedPattern == 0 );
    }
  } else {
    assert( mNumPatterns == 0 );
    assert( mPatternStartIndex == 0 );
    assert( mPatternEndIndex == 0 );
    assert( mReplacedPattern == 0 );
  }
#endif
}

Kleo::QGpgMEJob::~QGpgMEJob() {
  InvarianceChecker check( this );
  delete mCtx; mCtx = 0;
  delete mOutData; mOutData = 0;
  delete mOutDataDataProvider; mOutDataDataProvider = 0;
  delete mInData; mInData = 0;
  delete mInDataDataProvider; mInDataDataProvider = 0;
  deleteAllPatterns();
}

void Kleo::QGpgMEJob::resetQIODeviceDataObjects() {
    if ( const QGpgME::QIODeviceDataProvider * const dp = dynamic_cast<QGpgME::QIODeviceDataProvider*>( mOutDataDataProvider ) ) {
        delete mOutData; mOutData = 0;
        delete mOutDataDataProvider; mOutDataDataProvider = 0;
    }
    if ( const QGpgME::QIODeviceDataProvider * const dp = dynamic_cast<QGpgME::QIODeviceDataProvider*>( mInDataDataProvider ) ) {
        delete mInData; mInData = 0;
        delete mInDataDataProvider; mInDataDataProvider = 0;
    }
}

void Kleo::QGpgMEJob::deleteAllPatterns() {
  if ( mPatterns )
    for ( unsigned int i = 0 ; i < mNumPatterns ; ++i )
      free( (void*)mPatterns[i] );
  free( (void*)mReplacedPattern ); mReplacedPattern = 0;
  delete[] mPatterns; mPatterns = 0;
  mPatternEndIndex = mPatternStartIndex = mNumPatterns = 0;
}

void Kleo::QGpgMEJob::hookupContextToEventLoopInteractor() {
  mCtx->setManagedByEventLoopInteractor( true );
  QObject::connect( QGpgME::EventLoopInteractor::instance(),
		    SIGNAL(operationDoneEventSignal(GpgME::Context*,const GpgME::Error&)),
		    mThis, SLOT(slotOperationDoneEvent(GpgME::Context*,const GpgME::Error&)) );
}

void Kleo::QGpgMEJob::waitForFinished() {
    QEventLoop loop;
    mEventLoop = &loop;
    loop.exec( QEventLoop::ExcludeUserInputEvents );
    mEventLoop = 0;
}
    

void Kleo::QGpgMEJob::setPatterns( const QStringList & sl, bool allowEmpty ) {
  InvarianceChecker check( this );
  deleteAllPatterns();
  // create a new null-terminated C array of char* from patterns:
  mPatterns = new const char*[ sl.size() + 1 ];
  const char* * pat_it = mPatterns;
  mNumPatterns = 0;
  for ( QStringList::const_iterator it = sl.begin() ; it != sl.end() ; ++it ) {
    if ( (*it).isNull() )
      continue;
    if ( (*it).isEmpty() && !allowEmpty )
      continue;
    *pat_it++ = strdup( (*it).toUtf8().data() );
    ++mNumPatterns;
  }
  *pat_it++ = 0;
  mReplacedPattern = 0;
  mPatternEndIndex = mChunkSize = mNumPatterns;
}

void Kleo::QGpgMEJob::setChunkSize( unsigned int chunksize ) {
  InvarianceChecker check( this );
  if ( mReplacedPattern ) {
    mPatterns[mPatternEndIndex] = mReplacedPattern;
    mReplacedPattern = 0;
  }
  mChunkSize = std::min( chunksize, mNumPatterns );
  mPatternStartIndex = 0;
  mPatternEndIndex = mChunkSize;
  mReplacedPattern = mPatterns[mPatternEndIndex];
  mPatterns[mPatternEndIndex] = 0;
}

const char* * Kleo::QGpgMEJob::nextChunk() {
  InvarianceChecker check( this );
  if ( mReplacedPattern ) {
    mPatterns[mPatternEndIndex] = mReplacedPattern;
    mReplacedPattern = 0;
  }
  mPatternStartIndex += mChunkSize;
  mPatternEndIndex += mChunkSize;
  if ( mPatternEndIndex < mNumPatterns ) { // could safely be <=, but the last entry is NULL anyway
    mReplacedPattern = mPatterns[mPatternEndIndex];
    mPatterns[mPatternEndIndex] = 0;
  }
  return patterns();
}

const char* * Kleo::QGpgMEJob::patterns() const {
  InvarianceChecker check( this );
  if ( mPatternStartIndex < mNumPatterns )
    return mPatterns + mPatternStartIndex;
  return 0;
}

void Kleo::QGpgMEJob::setSigningKeys( const std::vector<GpgME::Key> & signers ) {
  mCtx->clearSigningKeys();
  for ( std::vector<GpgME::Key>::const_iterator it = signers.begin(), end = signers.end() ; it != end ; ++it ) {
    if ( it->isNull() )
      continue;
    if ( const GpgME::Error err = mCtx->addSigningKey( *it ) )
        return doThrow( err, i18n("Error adding signer %1.", QString::fromLatin1( it->primaryFingerprint() ) ) );
  }
}

void Kleo::QGpgMEJob::createInData( const boost::shared_ptr<QIODevice> & in ) {
  mInDataDataProvider = new QGpgME::QIODeviceDataProvider( in );
  mInData = new GpgME::Data( mInDataDataProvider );
  assert( !mInData->isNull() );
}

void Kleo::QGpgMEJob::createInData( const QByteArray & in ) {
  mInDataDataProvider = new QGpgME::QByteArrayDataProvider( in );
  mInData = new GpgME::Data( mInDataDataProvider );
  assert( !mInData->isNull() );
}

void Kleo::QGpgMEJob::createOutData( const boost::shared_ptr<QIODevice> & out ) {
  mOutDataDataProvider = new QGpgME::QIODeviceDataProvider( out );
  mOutData = new GpgME::Data( mOutDataDataProvider );
  assert( !mOutData->isNull() );
}

void Kleo::QGpgMEJob::createOutData() {
  mOutDataDataProvider = new QGpgME::QByteArrayDataProvider();
  mOutData = new GpgME::Data( mOutDataDataProvider );
  assert( !mOutData->isNull() );
}

QByteArray Kleo::QGpgMEJob::outData() const {
    if ( const QGpgME::QByteArrayDataProvider * const dp = dynamic_cast<QGpgME::QByteArrayDataProvider*>( mOutDataDataProvider ) )
        return dp->data();
    else
        return QByteArray();
}

void Kleo::QGpgMEJob::doThrow( const GpgME::Error & err, const QString & msg ) {
    mThis->deleteLater();
    throw GpgME::Exception( err, msg.toLocal8Bit().constData() );
}

static const unsigned int GetAuditLogFlags = GpgME::Context::AuditLogWithHelp|GpgME::Context::HtmlAuditLog;

static QString audit_log_as_html( GpgME::Context * ctx ) {
    if ( !ctx )
        return QString();
    QGpgME::QByteArrayDataProvider dp;
    GpgME::Data data( &dp );
    assert( !data.isNull() );
    if ( const GpgME::Error err = ctx->getAuditLog( data, GetAuditLogFlags ) )
        return QString();
    else
        return QString::fromUtf8( dp.data().data() );
}

void Kleo::QGpgMEJob::doSlotOperationDoneEvent( GpgME::Context * context, const GpgME::Error & e ) {
  if ( context == mCtx ) {
    getAuditLog();
    doEmitDoneSignal();
    doOperationDoneEvent( e );
    if ( mEventLoop )
      mEventLoop->quit();
    else
      mThis->deleteLater();
  }
}

void Kleo::QGpgMEJob::getAuditLog() {
    mAuditLogAsHtml = audit_log_as_html( mCtx );
}

void Kleo::QGpgMEJob::doSlotCancel() {
    if ( mPassphraseDialog.isVisible() )
    {
        mPassphraseDialog.reject();
    }
    else
    {
        mCtx->cancelPendingOperation();
    }
}

void Kleo::QGpgMEJob::showProgress( const char * what, int type, int current, int total ) {
  doEmitProgressSignal( QGpgMEProgressTokenMapper::instance()->map( what, type, current, total ), current, total );
}

char * Kleo::QGpgMEJob::getPassphrase( const char * useridHint, const char * /*description*/,
 				       bool previousWasBad, bool & canceled ) {
  // DF: here, description is the key fingerprint, twice, then "17 0". Not really descriptive.
  //     So I'm ignoring QString::fromLocal8Bit( description ) )
  QString msg = previousWasBad ?
                i18n( "You need a passphrase to unlock the secret key for user:<br/> %1 (retry)",
                      QString::fromUtf8( useridHint ) ) :
                i18n( "You need a passphrase to unlock the secret key for user:<br/> %1",
                      QString::fromUtf8( useridHint ) );
  msg += "<br/><br/>";
  //msg.prepend( "<qt>" );
  msg += i18n( "This dialog will reappear every time the passphrase is needed. For a more secure solution that also allows caching the passphrase, use gpg-agent." ) + "<br/>";
  const QString gpgAgent = KStandardDirs::findExe( "gpg-agent" );
  if ( !gpgAgent.isEmpty() ) {
    msg += i18n( "gpg-agent was found in %1, but does not appear to be running." ,
             gpgAgent );
  } else {
    msg += i18n( "gpg-agent is part of gnupg-%1, which you can download from %2" ,
             QString("1.9") ,
             QString("<a href=\"http://www.gnupg.org/download#gnupg2\">gnupg.org</a>") );
  }
  msg += "<br/>";
  msg += i18n( "For information on how to set up gpg-agent, see %1" ,
    QString("<a href=\"http://kmail.kde.org/kmail-pgpmime-howto.html\">http://kmail.kde.org/kmail-pgpmime-howto.html</a>") );
  msg += "<br/><br/>";
  msg += i18n( "Enter passphrase:" );
  mPassphraseDialog.setPrompt(msg);
  mPassphraseDialog.setPixmap( DesktopIcon( "pgp-keys", KIconLoader::SizeMedium ) );
  mPassphraseDialog.setCaption( i18n("Passphrase Dialog") );

  QPointer<QObject> that( mThis );
  const bool accepted = mPassphraseDialog.exec() == QDialog::Accepted; 
  assert( that );

  if ( !accepted ) {
    canceled = true;
    return 0;
  }

  canceled = false;
  // gpgme++ free()s it, and we need to copy as long as dlg isn't deleted :o
  return strdup( mPassphraseDialog.password().toLocal8Bit() );
}
