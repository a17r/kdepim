/*
    This file is part of the IMAP resources.
    Copyright (c) 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef RESOURCEIMAP_H
#define RESOURCEIMAP_H

#include <resourceimapshared.h>
#include <resourcenotes.h>
#include <libkcal/incidencebase.h>
#include <libkcal/calendarlocal.h>


namespace KNotesIMAP {

/**
 * This class implements a KNotes resource that keeps its
 * addresses in an IMAP folder in KMail (or other conforming email
 * clients).
 */
  class ResourceIMAP : public ResourceNotes,
                       public KCal::IncidenceBase::Observer,
                       public ResourceIMAPBase::ResourceIMAPShared
{
  Q_OBJECT

public:
    ResourceIMAP( const KConfig* );
    virtual ~ResourceIMAP();

    /**
     * Load resource data.
     */
    virtual bool load();

    /**
     * Save resource data.
     */
    virtual bool save();

    virtual bool addNote( KCal::Journal* );

    virtual bool deleteNote( KCal::Journal* );

    virtual void incidenceUpdated( KCal::IncidenceBase* );

    // The IMAPBase methods called by KMail
    virtual bool addIncidence( const QString& type, const QString& notes );
    virtual void deleteIncidence( const QString& type, const QString& uid );
    virtual void slotRefresh( const QString& type );

private:
    // Parse a journal from a string
    KCal::Journal* parseJournal( const QString& str );
    KCal::CalendarLocal mCalendar;

    bool mSilent;
};

}

#endif
