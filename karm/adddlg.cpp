/*
 *   karm
 *   This file only: Copyright (C) 1999  Espen Sand, espensa@online.no
 *   Modifications (see CVS log) Copyright (C) 2000 Klarälvdalens
 *   Datakonsult AB <kalle@dalheimer.de>, Jesper Pedersen <blackie@kde.org>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qhbox.h>
#include <kapplication.h>
#include <klocale.h>
#include <qcombobox.h>
#include <kdebug.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qpixmap.h>
#include <kiconloader.h>
#include <qwhatsthis.h>
#include "adddlg.h"
#include "ktimewidget.h"
#include <qpushbutton.h>
#include <qgroupbox.h>
#include "kwinmodule.h"


AddTaskDialog::AddTaskDialog(QString caption, bool editDlg, DesktopListType* desktopList)
  :KDialogBase(0, "AddTaskDialog", true, caption, Ok|Cancel, Ok, true ),
  origTotal( 0 ), origSession( 0 )
{
  QWidget *page = new QWidget( this ); 
  setMainWidget(page);

  QVBoxLayout *lay1 = new QVBoxLayout(page);
  
  QHBoxLayout *lay2 = new QHBoxLayout();
  lay1->addLayout(lay2);
  
  // The name of the widget
  QLabel *label = new QLabel( i18n("Task &name:"), page, "name" );
  lay2->addWidget( label );
  lay2->addSpacing(5);
  
  
  _name = new QLineEdit( page, "lineedit" );
  
  _name->setMinimumWidth(fontMetrics().maxWidth()*15);
  lay2->addWidget( _name );
  label->setBuddy( _name );


  // The "Edit Absolut" radio button
  lay1->addSpacing(10);lay1->addStretch(1); 
  _absoluteRB = new QRadioButton( i18n( "Edit &absolute" ), page, "_absoluteRB" );
  lay1->addWidget( _absoluteRB );
  connect( _absoluteRB, SIGNAL( clicked() ), this, SLOT(slotAbsolutePressed()) );
  

  // Absolute times
  QHBoxLayout *lay5 = new QHBoxLayout();
  lay1->addLayout(lay5);
  lay5->addSpacing(20);
  QGridLayout *lay3 = new QGridLayout( 2, 2, -1, "lay3" );
  lay5->addLayout(lay3);
  
  // Total Time
  _totalLA = new QLabel( i18n("&Total:"), page, "time" );
  lay3->addWidget( _totalLA, 0, 0 );

  _totalTW = new KTimeWidget( page, "_totalTW" );
  lay3->addWidget( _totalTW, 0, 1 );
  _totalLA->setBuddy( _totalTW );
  

  // Session
  _sessionLA = new QLabel( i18n("&Session time:"), page, "session time" );
  lay3->addWidget( _sessionLA, 1, 0 );

  _sessionTW = new KTimeWidget( page, "_sessionTW" );
  lay3->addWidget( _sessionTW, 1, 1 );
  _sessionLA->setBuddy( _sessionTW );


  // The "Edit relative" radio button
  lay1->addSpacing(10);lay1->addStretch(1);
  _relativeRB = new QRadioButton( i18n( "Edit &relative (Apply to both session and total)" ), page, "_relativeRB" );
  lay1->addWidget( _relativeRB );
  connect( _relativeRB, SIGNAL( clicked() ), this, SLOT(slotRelativePressed()) );
  
  // The relative times
  QHBoxLayout *lay4 = new QHBoxLayout();
  lay1->addLayout( lay4 );
  lay4->addSpacing(20);
  
  _operator = new QComboBox(page);
  _operator->insertItem( QString::fromLatin1( "+" ) );
  _operator->insertItem( QString::fromLatin1( "-" ) );
  lay4->addWidget( _operator );

  lay4->addSpacing(5);
  
  _diffTW = new KTimeWidget( page, "_sessionAddTW" );
  lay4->addWidget( _diffTW );

  {
    KWinModule k;
    desktopCount = k.numberOfDesktops();
  }

  // The "Choose Desktop" checkbox
  lay1->addSpacing(10);
  lay1->addStretch(1);
  _desktopCB = new QCheckBox(i18n("&Auto tracking"), page);
  _desktopCB->setEnabled(true);
  lay1->addWidget(_desktopCB);
  QGroupBox* groupBox;
  {
    int lines = (int)(desktopCount/2);
    if (lines*2 != desktopCount) lines++; 
      groupBox = new QButtonGroup(lines, QGroupBox::Horizontal, i18n("in desktop"), page, "_desktopsGB");
  }
  lay1->addWidget(groupBox);

  QHBoxLayout *lay6 = new QHBoxLayout();
	
  lay1->addLayout(lay6);
  for (int i=0; i<desktopCount; i++) {
    _deskBox.push_back(new QCheckBox(groupBox,QString::number(i)));
    _deskBox[i]->setText(QString::number(i+1));
    _deskBox[i]->setChecked(false);

    lay6->addWidget(_deskBox[i]);
  }

  // check specified Desktop Check Boxes
  bool enableDesktops = false;

  if (desktopList!=0 and desktopList->size()>0) {
    DesktopListType::iterator it = desktopList->begin();
    while (it != desktopList->end()) {
      _deskBox[*it]->setChecked(true);
      it++;
    }
    enableDesktops = true;
  }
  // if some desktops were specified, then enable the parent box

  _desktopCB->setChecked(enableDesktops);

  for (int i=0; i<desktopCount; i++)
    _deskBox[i]->setEnabled(enableDesktops);

  connect(_desktopCB, SIGNAL(clicked()), this, SLOT(slotAutoTrackingPressed()));

  KIconLoader loader;
  
  QPixmap whatsThisIM = loader.loadIcon(QString::fromLatin1("contexthelp"), KIcon::Toolbar);
  QPushButton* whatsThisBU = new QPushButton(page, "whatsThisLA");
  whatsThisBU->setFocusPolicy(NoFocus);

  connect(whatsThisBU, SIGNAL(clicked()), this, SLOT(enterWhatsThis()));
  whatsThisBU->setPixmap( whatsThisIM );
  lay4->addWidget(whatsThisBU);
  
  lay1->addStretch(1);


  if ( editDlg ) {
    // This is an edit dialog.
    _operator->setFocus();
  }
  else {
    // This is an initial dialog
    _name->setFocus();
  }

  slotRelativePressed();

  // Whats this help.
  QWhatsThis::add(_name, i18n("Enter the name of the task here. This name is for your eyes only."));
  QWhatsThis::add(_absoluteRB, i18n("If you select this radio button, you specify that you want "
                                    "to enter the time as absolute values. For example: "
                                    "total for this task is 20 hours and 15 minutes.\n\n"
                                    "The time is specified for the session time and for the total time "
                                    "separately."));
  QWhatsThis::add(_relativeRB, i18n("If you select this radio button, you specify that you want "
                                    "to add or subtract time for the task. For example: I've worked 2 hours "
                                    "and 20 minutes more on this task (without having the timer "
                                    "running.)\n\n"
                                    "This time will be added or subtracted for both the "
                                    "session time and the total time."));
  QWhatsThis::add(_totalTW, i18n("This is the overall time this task has been running."));
  QWhatsThis::add(_sessionTW, i18n("This is the time the task has been running this session."));
  QWhatsThis::add(_diffTW, i18n("Specify how much time to add or subtract to the overall and "
                                "session time"));
  
}

void AddTaskDialog::enterWhatsThis() 
{
  QWhatsThis::enterWhatsThisMode ();
}

  
void AddTaskDialog::slotAbsolutePressed()
{
  _relativeRB->setChecked( false );
  _absoluteRB->setChecked( true );

  _operator->setEnabled( false );
  _diffTW->setEnabled( false );

  _totalLA->setEnabled( true );
  _sessionLA->setEnabled( true );
  _totalTW->setEnabled( true );
  _sessionTW->setEnabled( true );
}

void AddTaskDialog::slotRelativePressed()
{
  _relativeRB->setChecked( true );
  _absoluteRB->setChecked( false );

  _operator->setEnabled( true );
  _diffTW->setEnabled( true );

  _totalLA->setEnabled( false );
  _sessionLA->setEnabled( false );
  _totalTW->setEnabled( false );
  _sessionTW->setEnabled( false );
}

void AddTaskDialog::slotAutoTrackingPressed()
{
  bool checked = _desktopCB->isChecked();
  for (unsigned int i=0; i<_deskBox.size(); i++)
    _deskBox[i]->setEnabled(checked);

  if (!checked)  // uncheck all desktop boxes
    for (int i=0; i<desktopCount; i++) 
      _deskBox[i]->setChecked(false);
}

void AddTaskDialog::setTask( const QString &name, long total, long session )
{
  _name->setText( name );
  
  _totalTW->setTime( total / 60, total % 60 );
  _sessionTW->setTime( session / 60, session % 60 );
  origTotal = total;
  origSession = session;
}


QString AddTaskDialog::taskName() const
{ 
  return( _name->text() ); 
}


void AddTaskDialog::status(long *total, long *totalDiff, long *session, 
			   long *sessionDiff, DesktopListType *desktopList) const
{ 
  if ( _absoluteRB->isChecked() ) {
    *total = _totalTW->time();
    *session = _sessionTW->time();
  }
  else {
    int diff = _diffTW->time();
    if ( _operator->currentItem() == 1) {
      diff = -diff;
    }
    *total = origTotal + diff;
    *session = origSession + diff;
  }

  *totalDiff = *total - origTotal;
  *sessionDiff = *session - origSession;

  for (unsigned int i=0; i<_deskBox.size(); i++) {
    if (_deskBox[i]->isChecked())
      desktopList->push_back(i);
  }
}

#include "adddlg.moc"
