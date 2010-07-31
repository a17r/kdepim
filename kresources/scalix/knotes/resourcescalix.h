/*
    This file is part of the scalix resource - based on the kolab resource.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

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

#ifndef KNOTES_RESOURCESCALIX_H
#define KNOTES_RESOURCESCALIX_H

#include <resourcenotes.h>
#include <libkcal/incidencebase.h>
#include <libkcal/calendarlocal.h>
#include "../shared/resourcescalixbase.h"
#include "../shared/subresource.h"
#include <kdepimmacros.h>


namespace Scalix {

/**
 * This class implements a KNotes resource that keeps its
 * addresses in an IMAP folder in KMail (or other conforming email
 * clients).
 */
class KDE_EXPORT ResourceScalix : public ResourceNotes,
                      public KCal::IncidenceBase::Observer,
                      public ResourceScalixBase
{
  Q_OBJECT

public:
  ResourceScalix( const KConfig* );
  virtual ~ResourceScalix();

  /// Load resource data.
  bool load();

  /// Save resource data.
  bool save();

  /// Open the notes resource.
  bool doOpen();
  /// Close the notes resource.
  void doClose();

  bool addNote( KCal::Journal* );

  bool deleteNote( KCal::Journal* );

  KCal::Alarm::List alarms( const TQDateTime& from, const TQDateTime& to );

  /// Reimplemented from IncidenceBase::Observer to know when a note was changed
  void incidenceUpdated( KCal::IncidenceBase* );

  /// The ResourceScalixBase methods called by KMail
  bool fromKMailAddIncidence( const TQString& type, const TQString& resource,
                              Q_UINT32 sernum, int format, const TQString& note );
  void fromKMailDelIncidence( const TQString& type, const TQString& resource,
                              const TQString& uid );
  void fromKMailRefresh( const TQString& type, const TQString& resource );

  /// Listen to KMail changes in the amount of sub resources
  void fromKMailAddSubresource( const TQString& type, const TQString& resource,
                                const TQString& label, bool writable );
  void fromKMailDelSubresource( const TQString& type, const TQString& resource );

  void fromKMailAsyncLoadResult( const TQMap<Q_UINT32, TQString>& map,
                                 const TQString& type,
                                 const TQString& folder );

  /** Return the list of subresources. */
  TQStringList subresources() const;

  /** Is this subresource active? */
  bool subresourceActive( const TQString& ) const;

signals:
  void signalSubresourceAdded( Resource*, const TQString&, const TQString& );
  void signalSubresourceRemoved( Resource*, const TQString&, const TQString& );

private:
  bool addNote( KCal::Journal* journal, const TQString& resource,
                Q_UINT32 sernum );
  KCal::Journal* addNote( const TQString& data, const TQString& subresource,
                          Q_UINT32 sernum, const TQString &mimetype );

  bool loadSubResource( const TQString& resource, const TQString& mimetype );

  TQString configFile() const {
    return ResourceScalixBase::configFile( "knotes" );
  }

  KCal::CalendarLocal mCalendar;

  // The list of subresources
  Scalix::ResourceMap mSubResources;
};

}

#endif // KNOTES_RESOURCESCALIX_H
