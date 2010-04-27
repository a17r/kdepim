/*
  This file is part of KOrganizer.

  Copyright (c) 2002,2003 Cornelius Schumacher <schumacher@kde.org>

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

#include "globals.h"
#include "prefs.h"

#include <kholidays/holidays.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kcalendarsystem.h>

#include <QApplication>
#include <QPixmap>
#include <QIcon>

using namespace KHolidays;
using namespace EventViews;

EventViewGlobals *EventViewGlobals::mSelf = 0;

EventViewGlobals *EventViewGlobals::self()
{
  if ( !mSelf ) {
    mSelf = new EventViewGlobals;
  }

  return mSelf;
}

EventViewGlobals::EventViewGlobals()
  : mOwnInstance( "korganizer" ), mHolidays( 0 )
{
  KIconLoader::global()->addAppDir( "kdepim" );
}

KConfig *EventViewGlobals::config() const
{
  KSharedConfig::Ptr c = mOwnInstance.config();
  return c.data();
}

EventViewGlobals::~EventViewGlobals()
{
  delete mHolidays;
}

const KCalendarSystem *EventViewGlobals::calendarSystem() const
{
  return KGlobal::locale()->calendar();
}

bool EventViewGlobals::reverseLayout()
{
  return QApplication::isRightToLeft();
}

QPixmap EventViewGlobals::smallIcon( const QString &name ) const
{
  return SmallIcon( name );
}

QStringList EventViewGlobals::holiday( const QDate &date ) const
{
  QStringList hdays;

  if ( !mHolidays ) {
    return hdays;
  }
  const Holiday::List list = mHolidays->holidays( date );
  for ( int i = 0; i < list.count(); ++i ) {
    hdays.append( list.at( i ).text() );
  }
  return hdays;
}

bool EventViewGlobals::isWorkDay( const QDate &date ) const
{
  int mask( ~( Prefs::instance()->mWorkWeekMask ) );

  bool nonWorkDay = ( mask & ( 1 << ( date.dayOfWeek() - 1 ) ) );
  if ( Prefs::instance()->mExcludeHolidays && mHolidays ) {
    const Holiday::List list = mHolidays->holidays( date );
    for ( int i = 0; i < list.count(); ++i ) {
      nonWorkDay = nonWorkDay || ( list.at( i ).dayType() == Holiday::NonWorkday );
    }
  }
  return !nonWorkDay;
}

int EventViewGlobals::getWorkWeekMask()
{
  return Prefs::instance()->mWorkWeekMask;
}

void EventViewGlobals::setHolidays( HolidayRegion *h )
{
  delete mHolidays;
  mHolidays = h;
}

HolidayRegion *EventViewGlobals::holidays() const
{
  return mHolidays;
}
