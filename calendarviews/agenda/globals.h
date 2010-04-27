/*
  This file is part of KOrganizer.

  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>

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

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef GLOBALS_H
#define GLOBALS_H

#include <kcomponentdata.h>
#include <QPixmap>

class KCalendarSystem;
class KConfig;
class QPixmap;
namespace KPIM {
  class ReminderClient;
}
namespace KHolidays {
  class HolidayRegion;
}

namespace EventViews
{

class EventViewGlobals
{
  public:
    static EventViewGlobals *self();

    KConfig *config() const;

    static bool reverseLayout();

    const KCalendarSystem *calendarSystem() const;

    ~EventViewGlobals();

    QPixmap smallIcon( const QString &name ) const;

    QStringList holiday( const QDate &qd ) const;
    bool isWorkDay( const QDate &qd ) const;
    int getWorkWeekMask();

    /**
       Set which holidays the user wants to use.
       @param h a HolidayRegion object initialized with the desired locale.
       We capture this object, so you must not delete it.
    */
    void setHolidays( KHolidays::HolidayRegion *h );

    /** return the HolidayRegion object or 0 if none has been defined
    */
    KHolidays::HolidayRegion *holidays() const;

    const KComponentData &componentData() const { return mOwnInstance; }

  protected:
    EventViewGlobals();

  private:
    static EventViewGlobals *mSelf;

    KComponentData mOwnInstance;

    KHolidays::HolidayRegion *mHolidays;
};

}

#endif
