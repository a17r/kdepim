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
#ifndef KDEPIM_EXCHANGE_UPLOAD_H
#define KDEPIM_EXCHANGE_UPLOAD_H

#include <tqstring.h>
#include <tqwidget.h>
#include <kio/job.h>

#include <kdepimmacros.h>

#include <libkcal/calendar.h>
#include <libkcal/event.h>

namespace KPIM {

class ExchangeAccount;

class KDE_EXPORT ExchangeUpload : public TQObject {
    Q_OBJECT
  public:
    ExchangeUpload( KCal::Event* event, ExchangeAccount* account, const TQString& timeZoneId, TQWidget* window=0 );
    ~ExchangeUpload();

  private slots:
    void slotPatchResult( KIO::Job * );
    void slotPropFindResult( KIO::Job * );
    void slotFindUidResult( KIO::Job * );

  signals:
    void startDownload();
    void finishDownload();
    void finished( ExchangeUpload* worker, int result, const TQString& moreInfo );

  private:
    void tryExist();
    void startUpload( const KURL& url );
    void findUid( TQString const& uid );
    
    ExchangeAccount* mAccount;
    KCal::Event* m_currentUpload;
    int m_currentUploadNumber;
    TQString mTimeZoneId;
    TQWidget* mWindow;
};

}

#endif
