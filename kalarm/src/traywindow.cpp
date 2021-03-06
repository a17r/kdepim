/*
 *  traywindow.cpp  -  the KDE system tray applet
 *  Program:  kalarm
 *  Copyright © 2002-2016 by David Jarvie <djarvie@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kalarm.h"   //krazy:exclude=includes (kalarm.h must be first)
#include "traywindow.h"

#include "alarmcalendar.h"
#include "alarmlistview.h"
#include "functions.h"
#include "kalarmapp.h"
#include "mainwindow.h"
#include "messagewin.h"
#include "newalarmaction.h"
#include "prefdlg.h"
#include "preferences.h"
#include "synchtimer.h"
#include "templatemenuaction.h"

#include <kalarmcal/alarmtext.h>

#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <KLocalizedString>
#include <kmessagebox.h>
#include <kstandardaction.h>
#include <kstandardguiitem.h>
#include <kiconeffect.h>
#include <kconfig.h>
#include <KIconLoader>
#include <KAboutData>

#include <QMenu>
#include <QList>
#include <QTimer>
#include <QLocale>
#include "kalarm_debug.h"

#include <stdlib.h>
#include <limits.h>

using namespace KAlarmCal;

struct TipItem
{
    QDateTime  dateTime;
    QString    text;
};


/*=============================================================================
= Class: TrayWindow
= The KDE system tray window.
=============================================================================*/

TrayWindow::TrayWindow(MainWindow* parent)
    : KStatusNotifierItem(parent),
      mAssocMainWindow(parent),
      mAlarmsModel(Q_NULLPTR),
      mStatusUpdateTimer(new QTimer(this)),
      mHaveDisabledAlarms(false)
{
    qCDebug(KALARM_LOG);
    setToolTipIconByName(QStringLiteral("kalarm"));
    setToolTipTitle(KAboutData::applicationData().displayName());
    setIconByName(QStringLiteral("kalarm"));
    // Load the disabled icon for use by setIconByPixmap()
    // - setIconByName() doesn't work for this one!
    mIconDisabled.addPixmap(KIconLoader::global()->loadIcon(QStringLiteral("kalarm-disabled"), KIconLoader::Panel));
    setStatus(KStatusNotifierItem::Active);
    // Set up the context menu
    mActionEnabled = KAlarm::createAlarmEnableAction(this);
    addAction(QStringLiteral("tAlarmsEnable"), mActionEnabled);
    contextMenu()->addAction(mActionEnabled);
    connect(theApp(), &KAlarmApp::alarmEnabledToggled, this, &TrayWindow::setEnabledStatus);
    contextMenu()->addSeparator();

    mActionNew = new NewAlarmAction(false, i18nc("@action", "&New Alarm"), this);
    addAction(QStringLiteral("tNew"), mActionNew);
    contextMenu()->addAction(mActionNew);
    connect(mActionNew, &NewAlarmAction::selected, this, &TrayWindow::slotNewAlarm);
    connect(mActionNew->fromTemplateAlarmAction(), &TemplateMenuAction::selected, this, &TrayWindow::slotNewFromTemplate);
    contextMenu()->addSeparator();

    QAction* a = KAlarm::createStopPlayAction(this);
    addAction(QStringLiteral("tStopPlay"), a);
    contextMenu()->addAction(a);
    QObject::connect(theApp(), &KAlarmApp::audioPlaying, a, &QAction::setVisible);
    QObject::connect(theApp(), &KAlarmApp::audioPlaying, this, &TrayWindow::updateStatus);

    a = KAlarm::createSpreadWindowsAction(this);
    addAction(QStringLiteral("tSpread"), a);
    contextMenu()->addAction(a);
    contextMenu()->addSeparator();
    contextMenu()->addAction(KStandardAction::preferences(this, SLOT(slotPreferences()), this));

    // Disable standard quit behaviour. We have to intercept the quit even,
    // if the main window is hidden.
    QAction* act = action(QStringLiteral("quit"));
    if (act)
    {
        act->disconnect(SIGNAL(triggered(bool)), this, SLOT(maybeQuit()));
        connect(act, &QAction::triggered, this, &TrayWindow::slotQuit);
    }

    // Set icon to correspond with the alarms enabled menu status
    setEnabledStatus(theApp()->alarmsEnabled());

    connect(AlarmCalendar::resources(), &AlarmCalendar::haveDisabledAlarmsChanged, this, &TrayWindow::slotHaveDisabledAlarms);
    connect(this, &TrayWindow::activateRequested, this, &TrayWindow::slotActivateRequested);
    connect(this, &TrayWindow::secondaryActivateRequested, this, &TrayWindow::slotSecondaryActivateRequested);
    slotHaveDisabledAlarms(AlarmCalendar::resources()->haveDisabledAlarms());

    // Hack: KSNI does not let us know when it is about to show the tooltip,
    // so we need to update it whenever something change in it.

    // This timer ensures that updateToolTip() is not called several times in a row
    mToolTipUpdateTimer = new QTimer(this);
    mToolTipUpdateTimer->setInterval(0);
    mToolTipUpdateTimer->setSingleShot(true);
    connect(mToolTipUpdateTimer, &QTimer::timeout, this, &TrayWindow::updateToolTip);

    // Update every minute to show accurate deadlines
    MinuteTimer::connect(mToolTipUpdateTimer, SLOT(start()));

    // Update when alarms are modified
    connect(AlarmListModel::all(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            mToolTipUpdateTimer, SLOT(start()));
    connect(AlarmListModel::all(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            mToolTipUpdateTimer, SLOT(start()));
    connect(AlarmListModel::all(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            mToolTipUpdateTimer, SLOT(start()));
    connect(AlarmListModel::all(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
            mToolTipUpdateTimer, SLOT(start()));
    connect(AlarmListModel::all(), SIGNAL(modelReset()),
            mToolTipUpdateTimer, SLOT(start()));

    // Set auto-hide status when next alarm or preferences change
    mStatusUpdateTimer->setSingleShot(true);
    connect(mStatusUpdateTimer, &QTimer::timeout, this, &TrayWindow::updateStatus);
    connect(AlarmCalendar::resources(), &AlarmCalendar::earliestAlarmChanged, this, &TrayWindow::updateStatus);
    Preferences::connect(SIGNAL(autoHideSystemTrayChanged(int)), this, SLOT(updateStatus()));
    updateStatus();

    // Update when tooltip preferences are modified
    Preferences::connect(SIGNAL(tooltipPreferencesChanged()), mToolTipUpdateTimer, SLOT(start()));
}

TrayWindow::~TrayWindow()
{
    qCDebug(KALARM_LOG);
    theApp()->removeWindow(this);
    Q_EMIT deleted();
}

/******************************************************************************
* Called when the "New Alarm" menu item is selected to edit a new alarm.
*/
void TrayWindow::slotNewAlarm(EditAlarmDlg::Type type)
{
    KAlarm::editNewAlarm(type);
}

/******************************************************************************
* Called when the "New Alarm" menu item is selected to edit a new alarm from a
* template.
*/
void TrayWindow::slotNewFromTemplate(const KAEvent* event)
{
    KAlarm::editNewAlarm(event);
}

/******************************************************************************
* Called when the "Configure KAlarm" menu item is selected.
*/
void TrayWindow::slotPreferences()
{
    KAlarmPrefDlg::display();
}

/******************************************************************************
* Called when the Quit context menu item is selected.
* Note that KAlarmApp::doQuit()  must be called by the event loop, not directly
* from the menu item, since otherwise the tray icon will be deleted while still
* processing the menu, resulting in a crash.
* Ideally, the connect() call setting up this slot in the constructor would use
* Qt::QueuedConnection, but the slot is never called in that case.
*/
void TrayWindow::slotQuit()
{
    // Note: QTimer::singleShot(0, ...) never calls the slot.
    QTimer::singleShot(1, this, &TrayWindow::slotQuitAfter);
}
void TrayWindow::slotQuitAfter()
{
    theApp()->doQuit(static_cast<QWidget*>(parent()));
}

/******************************************************************************
* Called when the Alarms Enabled action status has changed.
* Updates the alarms enabled menu item check state, and the icon pixmap.
*/
void TrayWindow::setEnabledStatus(bool status)
{
    qCDebug(KALARM_LOG) << (int)status;
    updateIcon();
    updateStatus();
    updateToolTip();
}

/******************************************************************************
* Called when individual alarms are enabled or disabled.
* Set the enabled icon to show or hide a disabled indication.
*/
void TrayWindow::slotHaveDisabledAlarms(bool haveDisabled)
{
    qCDebug(KALARM_LOG) << haveDisabled;
    mHaveDisabledAlarms = haveDisabled;
    updateIcon();
    updateToolTip();
}

/******************************************************************************
* A left click displays the KAlarm main window.
*/
void TrayWindow::slotActivateRequested()
{
    // Left click: display/hide the first main window
    if (mAssocMainWindow  &&  mAssocMainWindow->isVisible())
    {
        mAssocMainWindow->raise();
        mAssocMainWindow->activateWindow();
    }
}

/******************************************************************************
* A middle button click displays the New Alarm window.
*/
void TrayWindow::slotSecondaryActivateRequested()
{
    if (mActionNew->isEnabled())
        mActionNew->trigger();    // display a New Alarm dialog
}

/******************************************************************************
* Adjust icon auto-hide status according to when the next alarm is due.
* The icon is always shown if audio is playing, to give access to the 'stop'
* menu option.
*/
void TrayWindow::updateStatus()
{
    mStatusUpdateTimer->stop();
    int period =  Preferences::autoHideSystemTray();
    // If the icon is always to be shown (AutoHideSystemTray = 0),
    // or audio is playing, show the icon.
    bool active = !period || MessageWin::isAudioPlaying();
    if (!active)
    {
        // Show the icon only if the next active alarm complies
        active = theApp()->alarmsEnabled();
        if (active)
        {
            KAEvent* event = AlarmCalendar::resources()->earliestAlarm();
            active = static_cast<bool>(event);
            if (event  &&  period > 0)
            {
                KDateTime dt = event->nextTrigger(KAEvent::ALL_TRIGGER).effectiveKDateTime();
                qint64 delay = KDateTime::currentLocalDateTime().secsTo_long(dt);
                delay -= static_cast<qint64>(period) * 60;   // delay until icon to be shown
                active = (delay <= 0);
                if (!active)
                {
                    // First alarm trigger is too far in future, so tray icon is to
                    // be auto-hidden. Set timer for when it should be shown again.
                    delay *= 1000;   // convert to msec
                    int delay_int = static_cast<int>(delay);
                    if (delay_int != delay)
                        delay_int = INT_MAX;
                    mStatusUpdateTimer->setInterval(delay_int);
                    mStatusUpdateTimer->start();
                }
            }
        }
    }
    setStatus(active ? Active : Passive);
}

/******************************************************************************
* Adjust tooltip according to the app state.
* The tooltip text shows alarms due in the next 24 hours. The limit of 24
* hours is because only times, not dates, are displayed.
*/
void TrayWindow::updateToolTip()
{
    bool enabled = theApp()->alarmsEnabled();
    QString subTitle;
    if (enabled && Preferences::tooltipAlarmCount())
        subTitle = tooltipAlarmText();

    if (!enabled)
        subTitle = i18n("Disabled");
    else if (mHaveDisabledAlarms)
    {
        if (!subTitle.isEmpty())
            subTitle += QLatin1String("<br/>");
        subTitle += i18nc("@info:tooltip Brief: some alarms are disabled", "(Some alarms disabled)");
    }
    setToolTipSubTitle(subTitle);
}

/******************************************************************************
* Adjust icon according to the app state.
*/
void TrayWindow::updateIcon()
{
    if (theApp()->alarmsEnabled())
        setIconByName(mHaveDisabledAlarms ? QStringLiteral("kalarm-partdisabled") : QStringLiteral("kalarm"));
    else
        setIconByPixmap(mIconDisabled);
}

/******************************************************************************
* Return the tooltip text showing alarms due in the next 24 hours.
* The limit of 24 hours is because only times, not dates, are displayed.
*/
QString TrayWindow::tooltipAlarmText() const
{
    KAEvent event;
    const QString& prefix = Preferences::tooltipTimeToPrefix();
    int maxCount = Preferences::tooltipAlarmCount();
    KDateTime now = KDateTime::currentLocalDateTime();
    KDateTime tomorrow = now.addDays(1);

    // Get today's and tomorrow's alarms, sorted in time order
    int i, iend;
    QList<TipItem> items;
    QVector<KAEvent> events = KAlarm::getSortedActiveEvents(const_cast<TrayWindow*>(this), &mAlarmsModel);
    for (i = 0, iend = events.count();  i < iend;  ++i)
    {
        KAEvent* event = &events[i];
        if (event->actionSubType() == KAEvent::MESSAGE)
        {
            TipItem item;
            QDateTime dateTime = event->nextTrigger(KAEvent::DISPLAY_TRIGGER).effectiveKDateTime().toLocalZone().dateTime();
            if (dateTime > tomorrow.dateTime())
                break;   // ignore alarms after tomorrow at the current clock time
            item.dateTime = dateTime;

            // The alarm is due today, or early tomorrow
            if (Preferences::showTooltipAlarmTime())
            {
                item.text += QLocale().toString(item.dateTime.time(), QLocale::ShortFormat);
                item.text += QLatin1Char(' ');
            }
            if (Preferences::showTooltipTimeToAlarm())
            {
                int mins = (now.dateTime().secsTo(item.dateTime) + 59) / 60;
                if (mins < 0)
                    mins = 0;
                char minutes[3] = "00";
                minutes[0] = static_cast<char>((mins%60) / 10 + '0');
                minutes[1] = static_cast<char>((mins%60) % 10 + '0');
                if (Preferences::showTooltipAlarmTime())
                    item.text += i18nc("@info prefix + hours:minutes", "(%1%2:%3)", prefix, mins/60, QLatin1String(minutes));
                else
                    item.text += i18nc("@info prefix + hours:minutes", "%1%2:%3", prefix, mins/60, QLatin1String(minutes));
                item.text += QLatin1Char(' ');
            }
            item.text += AlarmText::summary(*event);

            // Insert the item into the list in time-sorted order
            int it = 0;
            for (int itend = items.count();  it < itend;  ++it)
            {
                if (item.dateTime <= items[it].dateTime)
                    break;
            }
            items.insert(it, item);
        }
    }
    qCDebug(KALARM_LOG);
    QString text;
    int count = 0;
    for (i = 0, iend = items.count();  i < iend;  ++i)
    {
        qCDebug(KALARM_LOG) << "--" << (count+1) << ")" << items[i].text;
        if (i > 0)
            text += QLatin1String("<br />");
        text += items[i].text;
        if (++count == maxCount)
            break;
    }
    return text;
}

/******************************************************************************
* Called when the associated main window is closed.
*/
void TrayWindow::removeWindow(MainWindow* win)
{
    if (win == mAssocMainWindow)
        mAssocMainWindow = Q_NULLPTR;
}

// vim: et sw=4:
