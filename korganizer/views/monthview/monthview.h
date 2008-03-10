/*
  This file is part of KOrganizer.

  Copyright (c) 2008 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef VIEW_H
#define VIEW_H

#include "koeventview.h"

class KOEventPopupMenu;

namespace KOrg {

class MonthGraphicsView;
class MonthScene;

/**
  New month view.
*/
class KONewMonthView : public KOEventView
{
  Q_OBJECT
  public:
    explicit KONewMonthView( Calendar *calendar, QWidget *parent = 0 );
    ~KONewMonthView();

    virtual int currentDateCount();
    virtual Incidence::List selectedIncidences()
    { return Incidence::List(); }

    /** Returns dates of the currently selected events */
    virtual DateList selectedDates();

    virtual bool eventDurationHint( QDateTime &startDt, QDateTime &endDt, bool &allDay );

    QDate startDate() const { return mStartDate; }
    QDate endDate() const { return mEndDate; }

  public slots:
    virtual void updateView();
    virtual void showDates( const QDate &start, const QDate &end );
    virtual void showIncidences( const Incidence::List &incidenceList );

    void changeIncidenceDisplay( Incidence *, int );

  protected:
    int maxDatesHint();

  private:
    void addIncidence( Incidence *incidence );
    // Compute and update the whole view
    void reloadIncidences();

    MonthGraphicsView *mView;
    MonthScene *mScene;

    QDate mStartDate;
    QDate mEndDate;

    int mCurrentMonth;

    KOEventPopupMenu *mViewPopup;

    friend class MonthScene;
    friend class MonthGraphicsView;
};

}

#endif
