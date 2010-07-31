/*
    This file is part of libkpimexchange
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <kapplication.h>
#include <kurl.h>
#include <kdebug.h>
#include <kcursor.h>
#include <klocale.h>

// These for test() method
#include <kio/http.h>
#include <kio/davjob.h>
// #include "libkdepim/resources/resourcemanager.h"
// #include "libkdepim/resources/calendar/resourcecalendar.h"


#include "exchangeclient.h"
#include "exchangeaccount.h"
#include "exchangeprogress.h"
#include "exchangeupload.h"
#include "exchangedownload.h"
#include "exchangedelete.h"
//#include "exchangemonitor.h"
#include "utils.h"

using namespace KPIM;

ExchangeClient::ExchangeClient( ExchangeAccount *account,
                                const TQString &timeZoneId )
  : mWindow( 0 ), mTimeZoneId( timeZoneId )
{
  kdDebug() << "Creating ExchangeClient...\n";
  mAccount = account;
  if ( timeZoneId.isNull() ) {
    setTimeZoneId( "UTC" );
  }
}

ExchangeClient::~ExchangeClient()
{
  kdDebug() << "ExchangeClient destructor" << endl;
}

void ExchangeClient::setWindow(TQWidget *window)
{
  mWindow = window;
}

TQWidget *ExchangeClient::window() const
{
  return mWindow;
}

void ExchangeClient::setTimeZoneId( const TQString& timeZoneId )
{
  mTimeZoneId = timeZoneId;
}

TQString ExchangeClient::timeZoneId()
{
  return mTimeZoneId;
}

void ExchangeClient::test()
{
//  if ( !mAccount->authenticate( mWindow ) ) return;
  kdDebug() << "Entering test()" << endl;
  KURL baseURL = KURL( "http://mail.tbm.tudelft.nl/janb/Calendar" );
  KURL url( "webdav://mail.tbm.tudelft.nl/exchange/" );

/*
  KRES::Manager<KCal::ResourceCalendar>* manager = new KRES::Manager<KCal::ResourceCalendar>( "calendar" );
  KCal::ResourceCalendar* resource = manager->standardResource();

  kdDebug(5800) << "Opening resource " + resource->resourceName() << endl;
  bool result = resource->open();
  kdDebug() << "Result: " << result << endl;

  resource->subscribeEvents( TQDate( 2002, 12, 18 ), TQDate( 2002, 12, 19 ) );
*/
//  mAccount->tryFindMailbox();
/*
  TQString query = 
  "<propfind xmlns=\"DAV:\" xmlns:h=\"urn:schemas:httpmail:\">\r\n"
  "  <allprop/>\r\n"
  "</propfind>\r\n";

  KIO::DavJob* job = new KIO::DavJob( url, (int) KIO::DAV_PROPFIND, query, false );
  job->addMetaData( "davDepth", "0" );
*/
//  ExchangeMonitor* monitor = new ExchangeMonitor( mAccount );
}

void ExchangeClient::test2()
{
  kdDebug() << "Entering test2()" << endl;
}
/*
ExchangeMonitor* ExchangeClient::monitor( int pollMode, const TQHostAddress& ownInterface ) 
{
  return new ExchangeMonitor( mAccount, pollMode, ownInterface  );
}
*/
void ExchangeClient::download( KCal::Calendar *calendar, const TQDate &start,
                               const TQDate &end, bool showProgress )
{
  kdDebug() << "ExchangeClient::download1()" << endl;

  if ( !mAccount->authenticate( mWindow ) ) {
    emit downloadFinished( 0, i18n("Authentication error") ); 
    return;
  }

  ExchangeDownload *worker = new ExchangeDownload( mAccount, mWindow );
  worker->download( calendar, start, end, showProgress );
  connect( worker,
           TQT_SIGNAL( finished( ExchangeDownload *, int, const TQString & ) ),
           TQT_SLOT( slotDownloadFinished( ExchangeDownload *, int,
                                       const TQString & ) ) );
}

void ExchangeClient::download( const TQDate &start, const TQDate &end,
                               bool showProgress )
{
  kdDebug() << "ExchangeClient::download2()" << endl;

  if ( !mAccount->authenticate( mWindow ) ) {
    emit downloadFinished( 0, i18n("Authentication error") ); 
    return;
  }

  ExchangeDownload *worker = new ExchangeDownload( mAccount, mWindow );
  worker->download( start, end, showProgress );
  connect( worker,
           TQT_SIGNAL( finished( ExchangeDownload *, int, const TQString & ) ), 
           TQT_SLOT( slotDownloadFinished( ExchangeDownload *, int,
                                       const TQString & ) ) );
  connect( worker, TQT_SIGNAL( gotEvent( KCal::Event *, const KURL & ) ), 
           TQT_SIGNAL( event( KCal::Event *, const KURL & ) ) );
}

void ExchangeClient::upload( KCal::Event *event )
{
  kdDebug() << "ExchangeClient::upload()" << endl;

  if ( !mAccount->authenticate( mWindow ) ) {
    emit uploadFinished( 0, i18n("Authentication error") ); 
    return;
  }

  ExchangeUpload *worker = new ExchangeUpload( event, mAccount, mTimeZoneId,
                                               mWindow );
  connect( worker, TQT_SIGNAL( finished( ExchangeUpload *, int, const TQString & ) ),
           TQT_SLOT( slotUploadFinished( ExchangeUpload *, int, const TQString & ) ) );
}

void ExchangeClient::remove( KCal::Event *event )
{
  if ( !mAccount->authenticate( mWindow ) ) {
    emit removeFinished( 0, i18n("Authentication error") ); 
    return;
  }

  ExchangeDelete *worker = new ExchangeDelete( event, mAccount, mWindow );
  connect( worker, TQT_SIGNAL( finished( ExchangeDelete *, int, const TQString & ) ),
           TQT_SLOT( slotRemoveFinished( ExchangeDelete *, int, const TQString & ) ) );
}

void ExchangeClient::slotDownloadFinished( ExchangeDownload *worker,
                                           int result, const TQString &moreInfo )
{
  emit downloadFinished( result, moreInfo );
  worker->deleteLater();
}

void ExchangeClient::slotDownloadFinished( ExchangeDownload* worker, int result, const TQString& moreInfo, TQPtrList<KCal::Event>& events ) 
{
  emit downloadFinished( result, moreInfo, events );
  worker->deleteLater();
}

void ExchangeClient::slotUploadFinished( ExchangeUpload* worker, int result, const TQString& moreInfo ) 
{
  kdDebug() << "ExchangeClient::slotUploadFinished()" << endl;
  emit uploadFinished( result, moreInfo );
  worker->deleteLater();
}

void ExchangeClient::slotRemoveFinished( ExchangeDelete* worker, int result, const TQString& moreInfo ) 
{
  kdDebug() << "ExchangeClient::slotRemoveFinished()" << endl;
  emit removeFinished( result, moreInfo );
  worker->deleteLater();
}

int ExchangeClient::downloadSynchronous( KCal::Calendar *calendar,
                                         const TQDate &start, const TQDate &end,
                                         bool showProgress )
{
  kdDebug() << "ExchangeClient::downloadSynchronous()" << endl;

  mClientState = WaitingForResult;
  connect( this, TQT_SIGNAL( downloadFinished( int, const TQString & ) ), 
           TQT_SLOT( slotSyncFinished( int, const TQString & ) ) );

  download( calendar, start, end, showProgress );

  // TODO: Remove this busy loop
  TQApplication::setOverrideCursor
  ( KCursor::waitCursor() );
  do {
    qApp->processEvents();
  } while ( mClientState == WaitingForResult );
  TQApplication::restoreOverrideCursor();  

  disconnect( this, TQT_SIGNAL( downloadFinished( int, const TQString & ) ), 
              this, TQT_SLOT( slotSyncFinished( int, const TQString & ) ) );

  return mSyncResult;
}

int ExchangeClient::uploadSynchronous( KCal::Event* event )
{
  mClientState = WaitingForResult;
  connect( this, TQT_SIGNAL( uploadFinished( int, const TQString & ) ), 
           TQT_SLOT( slotSyncFinished( int, const TQString & ) ) );

  upload( event );

  // TODO: Remove this busy loop
  TQApplication::setOverrideCursor( KCursor::waitCursor() );
  do {
    qApp->processEvents();
  } while ( mClientState == WaitingForResult );
  TQApplication::restoreOverrideCursor();  
  disconnect( this, TQT_SIGNAL( uploadFinished( int, const TQString & ) ), 
              this, TQT_SLOT( slotSyncFinished( int, const TQString & ) ) );
  return mSyncResult;
}

int ExchangeClient::removeSynchronous( KCal::Event* event )
{
  mClientState = WaitingForResult;
  connect( this, TQT_SIGNAL( removeFinished( int, const TQString & ) ), 
           TQT_SLOT( slotSyncFinished( int, const TQString & ) ) );

  remove( event );

  // TODO: Remove this busy loop
  TQApplication::setOverrideCursor( KCursor::waitCursor() );
  do {
    qApp->processEvents();
  } while ( mClientState == WaitingForResult );
  TQApplication::restoreOverrideCursor();  
  disconnect( this, TQT_SIGNAL( removeFinished( int, const TQString & ) ), 
              this, TQT_SLOT( slotSyncFinished( int, const TQString & ) ) );
  return mSyncResult;
}

void ExchangeClient::slotSyncFinished( int result, const TQString &moreInfo )
{
  kdDebug() << "Exchangeclient::slotSyncFinished("<<result<<","<<moreInfo<<")" << endl;
  if ( mClientState == WaitingForResult ) {
    mClientState = HaveResult;
    mSyncResult = result;
    mDetailedErrorString = moreInfo;
  }
}

TQString ExchangeClient::detailedErrorString()
{
  return mDetailedErrorString;
}

#include "exchangeclient.moc"
