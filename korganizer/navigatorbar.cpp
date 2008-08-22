/*
  This file is part of KOrganizer.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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

#include "navigatorbar.h"
#include "koglobals.h"
#include "koprefs.h"

#include <kdebug.h>
#include <kcalendarsystem.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QString>
#include <QToolButton>

ActiveLabel::ActiveLabel( QWidget *parent ) : QLabel( parent )
{
}

void ActiveLabel::mouseReleaseEvent( QMouseEvent * )
{
  emit clicked();
}

NavigatorBar::NavigatorBar( QWidget *parent )
  : QWidget( parent ), mHasMinWidth( false )
{
  QFont tfont = font();
  tfont.setPointSize( 10 );
  tfont.setBold( false );

  bool isRTL = KOGlobals::self()->reverseLayout();

  mPrevYear = createNavigationButton(
    isRTL ? "arrow-right-double" : "arrow-left-double",
    i18n( "Scroll backward to the previous year" ),
    i18n( "Click this button to scroll the display to the "
          "same approximate day of the previous year" ) );

  mPrevMonth = createNavigationButton(
    isRTL ? "arrow-right" : "arrow-left",
    i18n( "Scroll backward to the previous month" ),
    i18n( "Click this button to scroll the display to the "
          "same approximate date of the previous month" ) );

  mNextMonth = createNavigationButton(
    isRTL ? "arrow-left" : "arrow-right",
    i18n( "Scroll forward to the next month" ),
    i18n( "Click this button to scroll the display to the "
          "same approximate date of the next month" ) );

  mNextYear = createNavigationButton(
    isRTL ? "arrow-left-double" : "arrow-right-double",
    i18n( "Scroll forward to the next year" ),
    i18n( "Click this button to scroll the display to the "
          "same approximate day of the next year" ) );

  // Create month name button
  mMonth = new ActiveLabel( this );
  mMonth->setFont( tfont );
  mMonth->setAlignment( Qt::AlignCenter );
  mMonth->setMinimumHeight( mPrevYear->sizeHint().height() );
  mMonth->setToolTip( i18n( "Select a month" ) );

  // Create year button
  mYear = new ActiveLabel( this );
  mYear->setFont( tfont );
  mYear->setAlignment( Qt::AlignCenter );
  mYear->setMinimumHeight( mPrevYear->sizeHint().height() );
  mYear->setToolTip( i18n( "Select a year" ) );

  // set up control frame layout
  QHBoxLayout *ctrlLayout = new QHBoxLayout( this );
  ctrlLayout->addWidget( mPrevYear, 3 );
  ctrlLayout->addWidget( mPrevMonth, 3 );
  ctrlLayout->addWidget( mMonth, 3 );
  ctrlLayout->addWidget( mYear, 3 );
  ctrlLayout->addWidget( mNextMonth, 3 );
  ctrlLayout->addWidget( mNextYear, 3 );

  connect( mPrevYear, SIGNAL(clicked()), SIGNAL(goPrevYear()) );
  connect( mPrevMonth, SIGNAL(clicked()), SIGNAL(goPrevMonth()) );
  connect( mNextMonth, SIGNAL(clicked()), SIGNAL(goNextMonth()) );
  connect( mNextYear, SIGNAL(clicked()), SIGNAL(goNextYear()) );
  connect( mMonth, SIGNAL(clicked()), SLOT(selectMonth()) );
  connect( mYear, SIGNAL(clicked()), SLOT(selectYear()) );
}

NavigatorBar::~NavigatorBar()
{
}

void NavigatorBar::showButtons( bool left, bool right )
{
  if ( left ) {
    mPrevYear->show();
    mPrevMonth->show();
  } else {
    mPrevYear->hide();
    mPrevMonth->hide();
  }

  if ( right ) {
    mNextYear->show();
    mNextMonth->show();
  } else {
    mNextYear->hide();
    mNextMonth->hide();
  }

}

void NavigatorBar::selectDates( const KCal::DateList &dateList )
{
  if ( dateList.count() > 0 ) {
    mDate = dateList.first();

    const KCalendarSystem *calSys = KOGlobals::self()->calendarSystem();

    if ( !mHasMinWidth ) {
      // Set minimum width to width of widest month name label
      int i;
      int maxwidth = 0;

      for ( i = 1; i <= calSys->monthsInYear( mDate ); ++i ) {
        QString m = calSys->monthName( i, calSys->year( mDate ) );
        int w = QFontMetrics( mMonth->font() ).width( QString( "%1" ).arg( m ) );
        if ( w > maxwidth ) {
          maxwidth = w;
        }
      }
      mMonth->setMinimumWidth( maxwidth );

      mHasMinWidth = true;
    }

    // compute the labels at the top of the navigator
    mMonth->setText( i18nc( "monthname", "%1", calSys->monthName( mDate ) ) );
    mYear->setText( i18nc( "4 digit year", "%1", calSys->yearString( mDate ) ) );
  }
}

void NavigatorBar::selectMonth()
{
  // every year can have different month names (in some calendar systems)
  const KCalendarSystem *calSys = KOGlobals::self()->calendarSystem();

  int i;
  int month = calSys->month( mDate );
  int year = calSys->year( mDate );
  int months = calSys->monthsInYear( mDate );

  QMenu *menu = new QMenu( mMonth );
  QList<QAction *>act;

  QAction *activateAction = 0;
  for ( i=1; i <= months; ++i ) {
    QAction *monthAction = menu->addAction( calSys->monthName( i, year ) );
    act.append( monthAction );
    if ( i == month ) {
      activateAction = monthAction;
    }
  }
  if ( activateAction ) {
    menu->setActiveAction( activateAction );
  }
  QAction *selectedAct = menu->exec( mMonth->mapToGlobal( QPoint( 0, 0 ) ) );
  if ( selectedAct && ( selectedAct != activateAction ) ) {
    for ( i=0; i < months; i++ ) {
      if ( act[i] == selectedAct ) {
        emit goMonth( i + 1 );
      }
    }
  }
  qDeleteAll( act );
  act.clear();
  delete menu;
}

void NavigatorBar::selectYear()
{
  const KCalendarSystem *calSys = KOGlobals::self()->calendarSystem();

  int i;
  int year = calSys->year( mDate );
  int years = 11;  // odd number (show a few years ago -> a few years from now)
  int minYear = year - ( years / 3 );

  QMenu *menu = new QMenu( mYear );
  QList<QAction *>act;

  QString yearStr;
  QAction *activateAction = 0;
  int y = minYear;
  for ( i=0; i < years; i++ ) {
    QAction *yearAction = menu->addAction( yearStr.setNum( y ) );
    act.append( yearAction );
    if ( y == year ) {
      activateAction = yearAction;
    }
    y++;
  }
  if ( activateAction ) {
    menu->setActiveAction( activateAction );
  }
  QAction *selectedAct = menu->exec( mYear->mapToGlobal( QPoint( 0, 0 ) ) );
  if ( selectedAct && ( selectedAct != activateAction ) ) {
    int y = minYear;
    for ( i=0; i < years; ++i ) {
      if ( act[i] == selectedAct ) {
        emit goYear( y );
      }
      y++;
    }
  }
  qDeleteAll( act );
  act.clear();
  delete menu;
}

QToolButton *NavigatorBar::createNavigationButton( const QString &icon,
                                                   const QString &toolTip,
                                                   const QString &whatsThis )
{
  QToolButton *button = new QToolButton( this );

  button->setIcon(
    KIconLoader::global()->loadIcon( icon, KIconLoader::Desktop, KIconLoader::SizeSmall ) );

  // By the default the button has a very wide minimum size (for whatever
  // reasons). Override this, so that the date navigator doesn't need to be
  // so wide anymore. The minimum size is dominated by the other elements of
  // the date navigator then.
  button->setMinimumSize( 10, 10 );

  button->setToolTip( toolTip );
  button->setWhatsThis( whatsThis );

  return button;
}

#include "navigatorbar.moc"
