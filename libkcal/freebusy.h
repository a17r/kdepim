/*
    This file is part of libkcal.

    Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#ifndef KCAL_FREEBUSY_H
#define KCAL_FREEBUSY_H

#include <tqdatetime.h>
#include <tqvaluelist.h>
#include <tqptrlist.h>

#include "period.h"
#include "calendar.h"

#include "incidencebase.h"

namespace KCal {

  typedef TQValueList<Period> PeriodList;
/**
  This class provides information about free/busy time of a calendar user.
*/
class LIBKCAL_EXPORT FreeBusy : public IncidenceBase
{
  public:
    FreeBusy();
    FreeBusy( const TQDateTime &start, const TQDateTime &end );
    FreeBusy( Calendar *calendar, const TQDateTime &start,
              const TQDateTime &end );
    FreeBusy( PeriodList busyPeriods );

    ~FreeBusy();
    
    TQCString type() const { return "FreeBusy"; }

    virtual TQDateTime dtEnd() const;
    bool setDtEnd( const TQDateTime &end );

    PeriodList busyPeriods() const;

    /** Adds a period to the freebusy list and sorts the list.  */
    void addPeriod( const TQDateTime &start, const TQDateTime &end );
    void addPeriod( const TQDateTime &start, const Duration &dur );
    /** Adds a list of periods to the freebusy object and then sorts
     * that list. Use this if you are adding many items, instead of the
     * addPeriod method, to avoid sorting repeatedly.  */
    void addPeriods( const PeriodList & );
    void sortList();

    void merge( FreeBusy *freebusy );
    
  private:
    bool accept( Visitor &v ) { return v.visit( this ); }
    //This is used for creating a freebusy object for the current user
    bool addLocalPeriod( const TQDateTime &start, const TQDateTime &end );

    TQDateTime mDtEnd;
    PeriodList mBusyPeriods;
    Calendar *mCalendar;

    class Private;
    Private *d;
};

}

#endif
