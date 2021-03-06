/*
  This file is part of KOrganizer.

  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Author: Sergio Martins <sergio.martins@kdab.com>

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

#ifndef KORG_VIEWS_MONTHVIEW_H
#define KORG_VIEWS_MONTHVIEW_H

#include "koeventview.h"

namespace KOrg
{

class MonthView : public KOEventView
{
    Q_OBJECT
public:
    explicit MonthView(QWidget *parent = Q_NULLPTR);
    ~MonthView();

    int currentDateCount() const Q_DECL_OVERRIDE;
    int currentMonth() const;

    Akonadi::Item::List selectedIncidences() Q_DECL_OVERRIDE;

    /** Returns dates of the currently selected events */
    KCalCore::DateList selectedIncidenceDates() Q_DECL_OVERRIDE;

    QDateTime selectionStart() Q_DECL_OVERRIDE;

    QDateTime selectionEnd() Q_DECL_OVERRIDE;
    bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) Q_DECL_OVERRIDE;

    /**
     * Returns the average date in the view
     */
    QDate averageDate() const;

    bool usesFullWindow() Q_DECL_OVERRIDE;

    bool supportsDateRangeSelection() Q_DECL_OVERRIDE;

    CalendarSupport::CalPrinterBase::PrintType printType() const Q_DECL_OVERRIDE;

    int maxDatesHint() const Q_DECL_OVERRIDE;

    void setTypeAheadReceiver(QObject *o) Q_DECL_OVERRIDE;

    void setDateRange(const KDateTime &start, const KDateTime &end,
                      const QDate &preferredMonth = QDate()) Q_DECL_OVERRIDE;

    void setCalendar(const Akonadi::ETMCalendar::Ptr &cal) Q_DECL_OVERRIDE;

    void setIncidenceChanger(Akonadi::IncidenceChanger *changer) Q_DECL_OVERRIDE;

public Q_SLOTS:
    void updateView() Q_DECL_OVERRIDE;

    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) Q_DECL_OVERRIDE;

    void changeIncidenceDisplay(const Akonadi::Item &, Akonadi::IncidenceChanger::ChangeType) Q_DECL_OVERRIDE;

    void updateConfig() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void fullViewChanged(bool enabled);

private:
    void showDates(const QDate &start, const QDate &end,
                   const QDate &preferredMonth = QDate()) Q_DECL_OVERRIDE;

    class Private;
    Private *const d;
};

}
#endif
