/*
    This file is part of libkdepim.

    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

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
*/

#ifndef KVCARDDRAG_H
#define KVCARDDRAG_H

#include <tqdragobject.h>
#include <tqstring.h>

#include <kabc/addressee.h>
#include <kdepimmacros.h>

class KVCardDragPrivate;

/**
 * A drag-and-drop object for vcards. The according MIME type
 * is set to text/x-vcard.
 *
 * See the Qt drag'n'drop documentation.
 */
class KDE_EXPORT KVCardDrag : public QStoredDrag
{
  Q_OBJECT

  public:
    /**
     * Constructs an empty vcard drag.
     */
    KVCardDrag( TQWidget *dragsource = 0, const char *name = 0 );

    /**
     * Constructs a vcard drag with the @p addressee.
     */
    KVCardDrag( const TQString &content, TQWidget *dragsource = 0, const char *name = 0 );
    virtual ~KVCardDrag() {}

    /**
     * Sets the vcard of the drag to @p content.
     */
    void setVCard( const TQString &content );

    /**
     * Returns true if the MIME source @p e contains a vcard object.
     */
    static bool canDecode( TQMimeSource *e );

    /**
     * Decodes the MIME source @p e and puts the resulting vcard into @p content.
     */
    static bool decode( TQMimeSource *e, TQString &content );

    /**
     * Decodes the MIME source @p e and puts the resulting vcard into @p addresseess.
     */
    static bool decode( TQMimeSource *e, KABC::Addressee::List& addressees );

  protected:
     virtual void virtual_hook( int id, void* data );

  private:
     KVCardDragPrivate *d;
};

#endif // KVCARDDRAG_H
