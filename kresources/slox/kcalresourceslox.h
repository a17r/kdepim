/*
    This file is part of kdepim.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef KCALRESOURCESLOX_H
#define KCALRESOURCESLOX_H

#include "webdavhandler.h"

#include <qptrlist.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qdom.h>

#include <kurl.h>
#include <kconfig.h>
#include <kdirwatch.h>

#include <libkcal/incidence.h>
#include <libkcal/todo.h>
#include <libkcal/calendarlocal.h>
#include <libkcal/icalformat.h>
#include <libkcal/resourcecached.h>

namespace KIO {
class FileCopyJob;
class Job;
class DavJob;
}

/**
  This class provides a calendar stored as a remote file.
*/
class KCalResourceSlox : public KCal::ResourceCached
{
    Q_OBJECT

    friend class KCalResourceSloxConfig;

  public:
    /**
      Reload policy.
      
      @see setReloadPolicy(), reloadPolicy()
    */
    enum { ReloadNever, ReloadOnStartup, ReloadOnceADay, ReloadAlways };
  
    /**
      Create resource from configuration information stored in KConfig object.
    */
    KCalResourceSlox( const KConfig * );
    KCalResourceSlox( const KURL &url );
    ~KCalResourceSlox();

    void readConfig( const KConfig *config );
    void writeConfig( KConfig *config );

    /**
      Return name of file used as cache for remote file.
    */
    QString cacheFile();

    bool load();

    bool save();

    KABC::Lock *lock();

    bool isSaving();

    void dump() const;

    QString errorMessage();

    void setDownloadUrl( const KURL &u ) { mDownloadUrl = u; }
    KURL downloadUrl() const { return mDownloadUrl; }

  protected slots:
    void slotLoadEventsResult( KIO::Job * );
    void slotLoadTodosResult( KIO::Job * );
    void slotSaveJobResult( KIO::Job * );
    
    void slotProgress( KIO::Job *job, unsigned long percent );

  protected:
    bool doOpen();

    /** clears out the current calendar, freeing all used memory etc. etc. */
    void doClose();

    /** this method should be called whenever a Event is modified directly
     * via it's pointer.  It makes sure that the calendar is internally
     * consistent. */
    virtual void update( KCal::IncidenceBase *incidence );

    void requestEvents();
    void requestTodos();
 
    void parseMembersAttribute( const QDomElement &e,
                                KCal::Incidence *incidence );
    void parseIncidenceAttribute( const QDomElement &e,
                                  KCal::Incidence *incidence );
    void parseTodoAttribute( const QDomElement &e, KCal::Todo *todo );
    void parseEventAttribute( const QDomElement &e, KCal::Event *event );

    void emitEndProgress();
 
  private:
    void init();

    KURL mDownloadUrl;
    KURL mUploadUrl;

    int mReloadPolicy;

    KCal::ICalFormat mFormat;

    bool mOpen;

    KIO::DavJob *mLoadEventsJob;
    KIO::DavJob *mLoadTodosJob;
    KIO::FileCopyJob *mUploadJob;

    QDateTime mLastEventSync;
    QDateTime mLastTodoSync;
    
    KABC::Lock *mLock;

    QString mErrorMessage;

    WebdavHandler mWebdavHandler;
};

#endif
