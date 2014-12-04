/*
    This file is part of KOrganizer.
    Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <qtooltip.h>
#include <qfiledialog.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qbuttongroup.h>
#include <qvgroupbox.h>
#include <qwidgetstack.h>
#include <qdatetime.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>

#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <ktextedit.h>

#include <libkcal/incidenceformatter.h>
#include <libkcal/todo.h>

#include <libkdepim/kdateedit.h>

#include "koprefs.h"
#include "koglobals.h"
#include "ktimeedit.h"

#include "koeditorgeneraltodo.h"
#include "koeditorgeneraltodo.moc"

KOEditorGeneralTodo::KOEditorGeneralTodo( QObject *parent, const char *name )
  : KOEditorGeneral( parent, name )
{
  setType( "Todo" );
}

KOEditorGeneralTodo::~KOEditorGeneralTodo()
{
}

void KOEditorGeneralTodo::finishSetup()
{
  QWidget::setTabOrder( mSummaryEdit, mLocationEdit );
  QWidget::setTabOrder( mLocationEdit, mStartCheck );
  QWidget::setTabOrder( mStartCheck, mStartDateEdit );
  QWidget::setTabOrder( mStartDateEdit, mStartTimeEdit );
  QWidget::setTabOrder( mStartTimeEdit, mDueCheck );
  QWidget::setTabOrder( mDueCheck, mDueDateEdit );
  QWidget::setTabOrder( mDueDateEdit, mDueTimeEdit );
  QWidget::setTabOrder( mDueTimeEdit, mTimeButton );
  QWidget::setTabOrder( mTimeButton, mRecEditButton );
  QWidget::setTabOrder( mRecEditButton, mCompletedToggle );
  QWidget::setTabOrder( mCompletedToggle, mCompletedCombo );
  QWidget::setTabOrder( mCompletedCombo, mPriorityCombo );
  QWidget::setTabOrder( mPriorityCombo, mAlarmButton );
  QWidget::setTabOrder( mAlarmButton, mAlarmTimeEdit );
  QWidget::setTabOrder( mAlarmTimeEdit, mAlarmIncrCombo );
  QWidget::setTabOrder( mAlarmIncrCombo, mAlarmAdvancedButton );
  QWidget::setTabOrder( mAlarmAdvancedButton, mDescriptionEdit );
  QWidget::setTabOrder( mDescriptionEdit, mCategoriesButton );
  QWidget::setTabOrder( mCategoriesButton, mSecrecyCombo );

  mSummaryEdit->setFocus();
}

void KOEditorGeneralTodo::initTime(QWidget *parent,QBoxLayout *topLayout)
{
  kdDebug(5850) << k_funcinfo << endl;
  QBoxLayout *timeLayout = new QVBoxLayout(topLayout);

  QGroupBox *timeGroupBox = new QGroupBox(1,QGroupBox::Horizontal,
                                          i18n("Date && Time"),parent);
  timeLayout->addWidget(timeGroupBox);

  QFrame *timeBoxFrame = new QFrame(timeGroupBox);
  QWhatsThis::add( timeBoxFrame,
                   i18n("Sets options for due and start dates and times "
                        "for this to-do.") );

  QGridLayout *layoutTimeBox = new QGridLayout(timeBoxFrame,1,1);
  layoutTimeBox->setSpacing(topLayout->spacing());


  QString whatsThis = i18n("Sets the start date for this to-do");
  mStartCheck = new QCheckBox(i18n("Sta&rt:"),timeBoxFrame);
  QWhatsThis::add( mStartCheck, whatsThis );
  layoutTimeBox->addWidget(mStartCheck,0,0);
  connect(mStartCheck,SIGNAL(toggled(bool)),SLOT(enableStartEdit(bool)));
  connect(mStartCheck,SIGNAL(toggled(bool)),SLOT(startDateModified()));

  mStartDateEdit = new KDateEdit(timeBoxFrame);
  QWhatsThis::add( mStartDateEdit, whatsThis );
  layoutTimeBox->addWidget(mStartDateEdit,0,1);
  connect(mStartDateEdit,SIGNAL(dateChanged(const QDate&)),SLOT(startDateModified()));

  mStartTimeEdit = new KTimeEdit(timeBoxFrame);
  QWhatsThis::add( mStartTimeEdit,
                   i18n("Sets the start time for this to-do.") );
  layoutTimeBox->addWidget(mStartTimeEdit,0,2);
  connect(mStartTimeEdit,SIGNAL(timeChanged(QTime)),SLOT(startDateModified()));

  whatsThis = i18n("Sets the due date for this to-do.");
  mDueCheck = new QCheckBox(i18n("&Due:"),timeBoxFrame);
  QWhatsThis::add( mDueCheck, whatsThis );
  layoutTimeBox->addWidget(mDueCheck,1,0);
  connect(mDueCheck,SIGNAL(toggled(bool)),SLOT(enableDueEdit(bool)));
  connect(mDueCheck,SIGNAL(toggled(bool)),SIGNAL(dueDateEditToggle(bool)));
  connect(mDueCheck,SIGNAL(toggled(bool)),SLOT(dateChanged()));

  mDueDateEdit = new KDateEdit(timeBoxFrame);
  QWhatsThis::add( mDueDateEdit, whatsThis );
  layoutTimeBox->addWidget(mDueDateEdit,1,1);
  connect(mDueDateEdit,SIGNAL(dateChanged(const QDate&)),SLOT(dateChanged()));

  mDueTimeEdit = new KTimeEdit(timeBoxFrame);
  QWhatsThis::add( mDueTimeEdit,
                   i18n("Sets the due time for this to-do.") );
  layoutTimeBox->addWidget(mDueTimeEdit,1,2);
  connect(mDueTimeEdit,SIGNAL(timeChanged( QTime )),SLOT(dateChanged()));

  mTimeButton = new QCheckBox(i18n("Ti&me associated"),timeBoxFrame);
  QWhatsThis::add( mTimeButton,
                   i18n("Sets whether or not this to-do's start and due dates "
                        "have times associated with them.") );
  layoutTimeBox->addWidget( mTimeButton, 0, 3 );
  connect(mTimeButton,SIGNAL(toggled(bool)),SLOT(enableTimeEdits(bool)));
  connect(mTimeButton,SIGNAL(toggled(bool)),SLOT(dateChanged()));

  QLabel *label = new QLabel( i18n( "Recurrence:" ), timeBoxFrame );
  layoutTimeBox->addWidget( label, 3, 0 );
  QBoxLayout *recLayout = new QHBoxLayout();
  layoutTimeBox->addMultiCellLayout( recLayout, 3, 3, 1, 4 );
  mRecEditButton = new QPushButton( timeBoxFrame );
  mRecEditButton->setIconSet( KOGlobals::self()->smallIconSet( "recur", 16 ) );
  recLayout->addWidget( mRecEditButton );
  connect( mRecEditButton, SIGNAL(clicked()), SIGNAL(editRecurrence()) );
  mRecEditLabel = new QLabel( QString(), timeBoxFrame );
  recLayout->addWidget( mRecEditLabel );
  recLayout->addStretch( 1 );

  label = new QLabel( i18n("Reminder:"), timeBoxFrame );
  layoutTimeBox->addWidget( label, 4, 0 );
  QBoxLayout *alarmLineLayout = new QHBoxLayout();
  layoutTimeBox->addMultiCellLayout( alarmLineLayout, 4, 4, 1, 4 );
  initAlarm( timeBoxFrame, alarmLineLayout );
  alarmLineLayout->addStretch( 1 );

  // some more layouting
  layoutTimeBox->setColStretch( 3, 1 );

  QBoxLayout *secLayout = new QHBoxLayout();
  layoutTimeBox->addLayout( secLayout, 0, 4 );
  initSecrecy( timeBoxFrame, secLayout );
}


void KOEditorGeneralTodo::initCompletion( QWidget *parent, QBoxLayout *topLayout )
{
  QHBoxLayout *completionLayout = new QHBoxLayout( topLayout );

  QLabel *label = new QLabel( i18n( "&Completed:" ), parent );
  completionLayout->addWidget( label );

  mCompletedToggle = new QCheckBox( parent );
  QToolTip::add( mCompletedToggle,
                 i18n( "Toggle between 0% and 100% complete" ) );
  QWhatsThis::add( mCompletedToggle,
                   i18n( "Click this checkbox to toggle the completed percentage of the to-do "
                         "between 0% or 100%" ) );
  connect( mCompletedToggle, SIGNAL(clicked()), SLOT(completedChanged()) );
  completionLayout->addWidget( mCompletedToggle );
  label->setBuddy( mCompletedToggle );

  mCompletedCombo = new QComboBox( parent );
  QToolTip::add( mCompletedCombo,
                 i18n( "Select the completed percentage" ) );
  QWhatsThis::add( mCompletedCombo,
                   i18n( "Use this combobox to set the completion percentage of the to-do." ) );
  for ( int i = 0; i <= 100; i+=10 ) {
    // xgettext:no-c-format
    QString label = i18n( "Percent complete", "%1 %" ).arg( i );
    mCompletedCombo->insertItem( label );
  }
  connect( mCompletedCombo, SIGNAL(activated(int)), SLOT(completedChanged(int)) );
  completionLayout->addWidget( mCompletedCombo );

  mCompletedLabel = new QLabel( i18n( "completed on", "on" ), parent );
  mCompletedLabel->hide();
  completionLayout->addWidget( mCompletedLabel );

  mCompletionDateEdit = new KDateEdit( parent );
  mCompletionDateEdit->hide();
  completionLayout->addWidget( mCompletionDateEdit );

  mCompletionTimeEdit = new KTimeEdit( parent, QTime() );
  mCompletionTimeEdit->hide();
  completionLayout->addWidget( mCompletionTimeEdit );
}

void KOEditorGeneralTodo::initPriority(QWidget *parent, QBoxLayout *topLayout)
{
  QLabel *priorityLabel = new QLabel( i18n( "&Priority:" ), parent );
  topLayout->addWidget( priorityLabel );

  mPriorityCombo = new QComboBox( parent );
  QToolTip::add( mPriorityCombo,
                 i18n( "Set the priority of the to-do" ) );
  QWhatsThis::add( mPriorityCombo,
                   i18n( "Sets the priority of this to-do on a scale from one to nine, "
                         "with one being the highest priority, five being a medium priority, "
                         "and nine being the lowest. In programs that have a different scale, "
                         "the numbers will be adjusted to match the appropriate scale." ) );
  mPriorityCombo->insertItem( i18n( "unspecified" ) );
  mPriorityCombo->insertItem( i18n( "1 (highest)" ) );
  mPriorityCombo->insertItem( i18n( "2" ) );
  mPriorityCombo->insertItem( i18n( "3" ) );
  mPriorityCombo->insertItem( i18n( "4" ) );
  mPriorityCombo->insertItem( i18n( "5 (medium)" ) );
  mPriorityCombo->insertItem( i18n( "6" ) );
  mPriorityCombo->insertItem( i18n( "7" ) );
  mPriorityCombo->insertItem( i18n( "8" ) );
  mPriorityCombo->insertItem( i18n( "9 (lowest)" ) );
  topLayout->addWidget( mPriorityCombo );
  priorityLabel->setBuddy( mPriorityCombo );
}

void KOEditorGeneralTodo::initStatus(QWidget *parent,QBoxLayout *topLayout)
{
  QBoxLayout *statusLayout = new QHBoxLayout(topLayout);

  initCompletion( parent, statusLayout );

  statusLayout->addStretch( 1 );

  initPriority( parent, statusLayout );
}

void KOEditorGeneralTodo::setDefaults( const QDateTime &due, bool allDay )
{
  kdDebug(5850) << k_funcinfo << due <<endl;
  KOEditorGeneral::setDefaults(allDay);

  mTimeButton->setChecked( !allDay );
  mTimeButton->setEnabled( mTimeButton->isChecked() /* i.e. !allDay */ );

  enableTimeEdits( !allDay );

  mDueCheck->setChecked( due.isValid() );
  enableDueEdit( due.isValid() );

  mStartCheck->setChecked(false);
  enableStartEdit(false);

  if ( due.isValid() ) {
    mDueDateEdit->setDate( due.date() );
    mDueTimeEdit->setTime( due.time() );
  } else {
    // Make it due tomorrow.
    mDueDateEdit->setDate( QDate::currentDate().addDays(1) );
    mDueTimeEdit->setTime( QTime::currentTime() );
  }

  if ( !due.isValid() || (QDateTime::currentDateTime() < due) ) {
    mStartDateEdit->setDate( QDate::currentDate() );
    mStartTimeEdit->setTime( QTime::currentTime() );
  } else {
    mStartDateEdit->setDate( due.date().addDays( -1 ) );
    mStartTimeEdit->setTime( due.time() );
  }
  mStartDateModified = false;

  mPriorityCombo->setCurrentItem( 5 );

  mCompletedToggle->setChecked( false );
  mCompletedCombo->setCurrentItem( 0 );
}

void KOEditorGeneralTodo::readTodo(Todo *todo, Calendar *calendar, const QDate &date )
{
  KOEditorGeneral::readIncidence(todo, calendar);

  QDateTime dueDT;

  if (todo->hasDueDate()) {
    dueDT = todo->dtDue();
    if ( todo->doesRecur() && date.isValid() ) {
      QDateTime dt( date, QTime( 0, 0, 0 ) );
      dt = dt.addSecs( -1 );
      dueDT.setDate( todo->recurrence()->getNextDateTime( dt ).date() );
    }
    mDueDateEdit->setDate(dueDT.date());
    mDueTimeEdit->setTime(dueDT.time());
    mDueCheck->setChecked(true);
  } else {
    mDueDateEdit->setEnabled(false);
    mDueTimeEdit->setEnabled(false);
    mDueDateEdit->setDate(QDate::currentDate());
    mDueTimeEdit->setTime(QTime::currentTime());
    mDueCheck->setChecked(false);
  }

  if (todo->hasStartDate()) {
    QDateTime startDT = todo->dtStart();
    if ( todo->doesRecur() && date.isValid() && todo->hasDueDate() ) {
      int days = todo->dtStart( true ).daysTo( todo->dtDue( true ) );
      startDT.setDate( date.addDays( -days ) );
    }
    mStartDateEdit->setDate(startDT.date());
    mStartTimeEdit->setTime(startDT.time());
    mStartCheck->setChecked(true);
  } else {
    mStartDateEdit->setEnabled(false);
    mStartTimeEdit->setEnabled(false);
    mStartDateEdit->setDate(QDate::currentDate());
    mStartTimeEdit->setTime(QTime::currentTime());
    mStartCheck->setChecked(false);
  }

  mTimeButton->setChecked( !todo->doesFloat() );

  updateRecurrenceSummary( todo );

  mAlreadyComplete = false;
  mCompletedCombo->setCurrentItem( todo->percentComplete() / 10 );
  if ( todo->isCompleted() && todo->hasCompletedDate() ) {
    mCompletedDateTime = todo->completed();
    mCompletedToggle->setChecked( true );
    mAlreadyComplete = true;
  }
  setCompletedDate();

  mPriorityCombo->setCurrentItem( todo->priority() );
  mStartDateModified = false;
}

void KOEditorGeneralTodo::writeTodo(Todo *todo)
{
  KOEditorGeneral::writeIncidence(todo);

  // temp. until something better happens.
  QString tmpStr;

  todo->setHasDueDate(mDueCheck->isChecked());
  todo->setHasStartDate(mStartCheck->isChecked());

  QDate tmpSDate, tmpDDate;
  QTime tmpSTime, tmpDTime;
  QDateTime tmpStartDT, tmpDueDT;
  if ( mTimeButton->isChecked() ) {
    todo->setFloats(false);

    // set due date/time
    tmpDDate = mDueDateEdit->date();
    tmpDTime = mDueTimeEdit->getTime();
    tmpDueDT.setDate(tmpDDate);
    tmpDueDT.setTime(tmpDTime);

    // set start date/time
    if ( mStartCheck->isChecked() ) {
      tmpSDate = mStartDateEdit->date();
      tmpSTime = mStartTimeEdit->getTime();
      tmpStartDT.setDate(tmpSDate);
      tmpStartDT.setTime(tmpSTime);
    } else {
      tmpStartDT = tmpDueDT;
    }
  } else {
    todo->setFloats(true);

    // need to change this.
    tmpDDate = mDueDateEdit->date();
    tmpDTime.setHMS(0,0,0);
    tmpDueDT.setDate(tmpDDate);
    tmpDueDT.setTime(tmpDTime);

    if ( mStartCheck->isChecked() ) {
      tmpSDate = mStartDateEdit->date();
      tmpSTime.setHMS(0,0,0);
      tmpStartDT.setDate(tmpSDate);
      tmpStartDT.setTime(tmpSTime);
    } else {
      tmpStartDT = tmpDueDT;
    }
  }

  // TODO: Don't use the due date for the recurrence, but the start date (cf. rfc 2445)
  if ( todo->doesRecur() && !mStartDateModified ) {
    todo->setDtDue( tmpDueDT );
  } else {
    todo->setDtDue( tmpDueDT, true );
    todo->setDtStart( tmpStartDT );
    todo->setDtRecurrence( tmpDueDT );
  }

  todo->setPriority( mPriorityCombo->currentItem() );

  // set completion state
  todo->setPercentComplete( mCompletedCombo->currentItem() * 10 );

  if (mCompletedCombo->currentItem() == 10 && mCompletedDateTime.isValid()) {
    QDateTime completed( mCompletionDateEdit->date(),
                         mCompletionTimeEdit->getTime() );
    int difference = mCompletedDateTime.secsTo( completed );
    if ( (difference < 60) && (difference > -60) &&
         (completed.time().minute() == mCompletedDateTime.time().minute() ) ) {
      // completion time wasn't changed substantially (only the seconds
      // truncated, but that's an effect done by KTimeEdit automatically).
      completed = mCompletedDateTime;
    }
    todo->setCompleted( completed );
  }
}

void KOEditorGeneralTodo::enableDueEdit(bool enable)
{
  mDueDateEdit->setEnabled( enable );

  if(mDueCheck->isChecked() || mStartCheck->isChecked()) {
    mTimeButton->setEnabled(true);
  } else {
    mTimeButton->setEnabled(false);
  }

  if (enable) {
    mDueTimeEdit->setEnabled( mTimeButton->isChecked() );
  } else {
    mDueTimeEdit->setEnabled( false );
  }
}

void KOEditorGeneralTodo::enableStartEdit( bool enable )
{
  mStartDateEdit->setEnabled( enable );

  if(mDueCheck->isChecked() || mStartCheck->isChecked()) {
    mTimeButton->setEnabled(true);
  }
  else {
    mTimeButton->setEnabled(false);
    mTimeButton->setChecked(false);
  }

  if (enable) {
    mStartTimeEdit->setEnabled( mTimeButton->isChecked() );
  } else {
    mStartTimeEdit->setEnabled( false );
  }
}

void KOEditorGeneralTodo::enableTimeEdits(bool enable)
{
  if(mStartCheck->isChecked()) {
    mStartTimeEdit->setEnabled( enable );
  }
  if(mDueCheck->isChecked()) {
    mDueTimeEdit->setEnabled( enable );
  }
}

bool KOEditorGeneralTodo::validateInput()
{
  if (mDueCheck->isChecked()) {
    if (!mDueDateEdit->date().isValid()) {
      KMessageBox::sorry(0,i18n("Please specify a valid due date."));
      return false;
    }
    if (mTimeButton->isChecked()) {
      if (!mDueTimeEdit->inputIsValid()) {
        KMessageBox::sorry(0,i18n("Please specify a valid due time."));
        return false;
      }
    }
  }

  if (mStartCheck->isChecked()) {
    if (!mStartDateEdit->date().isValid()) {
      KMessageBox::sorry(0,i18n("Please specify a valid start date."));
      return false;
    }
    if (mTimeButton->isChecked()) {
      if (!mStartTimeEdit->inputIsValid()) {
        KMessageBox::sorry(0,i18n("Please specify a valid start time."));
        return false;
      }
    }
  }

  if (mStartCheck->isChecked() && mDueCheck->isChecked()) {
    QDateTime startDate;
    QDateTime dueDate;
    startDate.setDate(mStartDateEdit->date());
    dueDate.setDate(mDueDateEdit->date());
    if (mTimeButton->isChecked()) {
      startDate.setTime(mStartTimeEdit->getTime());
      dueDate.setTime(mDueTimeEdit->getTime());
    }
    if (startDate > dueDate) {
      KMessageBox::sorry(0,
                         i18n("The start date cannot be after the due date."));
      return false;
    }
  }

  return KOEditorGeneral::validateInput();
}

void KOEditorGeneralTodo::updateRecurrenceSummary( Todo *todo )
{
  if ( todo->doesRecur() ) {
    mRecEditLabel->setText( IncidenceFormatter::recurrenceString( todo ) );
  } else {
    mRecEditLabel->setText( QString() );
  }
}

void KOEditorGeneralTodo::completedChanged( int index )
{
  if ( index == 10 ) {
    mCompletedToggle->setChecked( true );
    mCompletedDateTime = QDateTime::currentDateTime();
  } else {
    mCompletedToggle->setChecked( false );
  }
  setCompletedDate();
}

void KOEditorGeneralTodo::completedChanged()
{
  if ( mCompletedToggle->isChecked() ) {
    mCompletedCombo->setCurrentItem( 10 );
    mCompletedDateTime = QDateTime::currentDateTime();
  } else {
    mCompletedCombo->setCurrentItem( 0 );
  }
  setCompletedDate();
}

void KOEditorGeneralTodo::dateChanged()
{
  KLocale *l = KGlobal::locale();
  QString dateTimeStr = "";

  if ( mStartCheck->isChecked() ) {
    dateTimeStr += i18n("Start: %1").arg(
                                     l->formatDate( mStartDateEdit->date() ) );
    if ( mTimeButton->isChecked() )
      dateTimeStr += QString(" %1").arg(
                                   l->formatTime( mStartTimeEdit->getTime() ) );
  }

  if ( mDueCheck->isChecked() ) {
    dateTimeStr += i18n("   Due: %1").arg(
                                      l->formatDate( mDueDateEdit->date() ) );
    if ( mTimeButton->isChecked() )
      dateTimeStr += QString(" %1").arg(
                                    l->formatTime( mDueTimeEdit->getTime() ) );
  }

  emit dateTimeStrChanged( dateTimeStr );
  QDateTime endDt( mDueDateEdit->date(), mDueTimeEdit->getTime() );
  emit signalDateTimeChanged( endDt, endDt );
}

void KOEditorGeneralTodo::startDateModified()
{
  mStartDateModified = true;
  dateChanged();
}

void KOEditorGeneralTodo::setCompletedDate()
{
  if ( mCompletedCombo->currentItem() == 10 && mCompletedDateTime.isValid() ) {
    mCompletedLabel->show();
    mCompletionDateEdit->show();
    mCompletionTimeEdit->show();
    mCompletionDateEdit->setDate( mCompletedDateTime.date() );
    mCompletionTimeEdit->setTime( mCompletedDateTime.time() );
  } else {
    mCompletedLabel->hide();
    mCompletionDateEdit->hide();
    mCompletionTimeEdit->hide();
  }
}

void KOEditorGeneralTodo::modified (Todo* todo, KOGlobals::HowChanged modification)
{
  switch (modification) {
  case KOGlobals::PRIORITY_MODIFIED:
    mPriorityCombo->setCurrentItem( todo->priority() );
    break;
  case KOGlobals::COMPLETION_MODIFIED:
    mCompletedCombo->setCurrentItem(todo->percentComplete() / 10);
    if (todo->isCompleted() && todo->hasCompletedDate()) {
      mCompletedDateTime = todo->completed();
      mCompletedToggle->setChecked( true );
    }
    setCompletedDate();
    break;
  case KOGlobals::CATEGORY_MODIFIED:
    setCategories( todo->categories() );
    break;
  case KOGlobals::UNKNOWN_MODIFIED: // fall through
  default:
    readTodo( todo, 0, QDate() );
    break;
  }
}