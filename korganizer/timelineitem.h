/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KORG_TIMELINEITEM_H
#define KORG_TIMELINEITEM_H

#include <kdgantt/KDGanttViewTaskItem.h>

class KDGanttView;

namespace KCal {
  class Calendar;
  class ResourceCalendar;
  class Incidence;
}

namespace KOrg {

class TimelineItem : public KDGanttViewTaskItem
{
  public:
    TimelineItem( const QString &label, KDGanttView* parent );

    void insertIncidence( KCal::Incidence *incidence,
                          const QDateTime &start = QDateTime(),
                          const QDateTime &end = QDateTime() );
};

class TimelineSubItem : public KDGanttViewTaskItem
{
  public:
    TimelineSubItem( KCal::Incidence *incidence, TimelineItem *parent );
  private:
    KCal::Incidence *mIncidence;
};

}

#endif
