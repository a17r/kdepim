/***************************************************************************
        konsolekalendarexports.cpp  -  description
           -------------------
    begin                : Sun May 25 2003
    copyright            : (C) 2003 by Tuukka Pasanen
    copyright            : (C) 2003 by Allen Winter
    email                : illuusio@mailcity.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <iostream>

#include <qdatetime.h>

#include <kdebug.h>
#include <klocale.h>

#include <libkcal/calendarlocal.h>
#include <libkcal/calendar.h>
#include <libkcal/event.h>
#include <libkcal/htmlexport.h>


#include "konsolekalendarexports.h"

using namespace KCal;
using namespace std;

KonsoleKalendarExports::KonsoleKalendarExports( KonsoleKalendarVariables *variables )
{
  m_variables = variables;
  m_firstEntry = true;
}


KonsoleKalendarExports::~KonsoleKalendarExports()
{
}

void KonsoleKalendarExports::exportAsTxt( Event *event ){

  if( m_firstEntry == true || 
      m_lastDate.day() != event->dtStart().date().day() ||
      m_lastDate.month() != event->dtStart().date().month() ||
      m_lastDate.year() != event->dtStart().date().year() ){
	  
	  
    m_firstEntry=false;	  
    int len = event->dtStartStr().length();
    QString date = event->dtStartStr();
    date.truncate( len - 5 );
    cout << I18N_NOOP("Date:") << "\t" <<  date.local8Bit() << endl;
    m_lastDate = event->dtStart().date();
	  
  }

  if ( !event->doesFloat() ) {
    cout << "\t";
    cout <<  event->dtStartStr().remove(0, (event->dtStartStr().find(' ', 0, false) + 1) ).local8Bit();
    cout << " - ";
    cout << event->dtEndStr().remove(0, (event->dtEndStr().find(' ', 0, false) + 1) ).local8Bit();
  }


  cout << endl << I18N_NOOP("Summary:") << endl;
  cout << "\t" << event->summary().local8Bit() << endl;
  cout << I18N_NOOP("Description:") << endl;  
  cout << "\t" << event->description().local8Bit() << endl;
  cout << "-----------------" << endl;
}

void KonsoleKalendarExports::exportAsTxtKorganizer( Event *event ){
 event = event;
}

void KonsoleKalendarExports::exportAsCSV( Event *event ){
  if ( !event->doesFloat() ) {
    cout <<  event->dtStartStr().remove(0, (event->dtStartStr().find(' ', 0, false) + 1) ).local8Bit();
    cout << "\t";
    cout << event->dtEndStr().remove(0, (event->dtEndStr().find(' ', 0, false) + 1) ).local8Bit();
  }


  cout << "\t" << I18N_NOOP("Summary:") << "\t";
  cout << "\t\"" << event->summary().local8Bit() << "\"" << "\t";
  cout << "\t" << I18N_NOOP("Description:") << "\t";
  cout << "\t\"" << event->description().local8Bit() << "\"" << endl;


}


