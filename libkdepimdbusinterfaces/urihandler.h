/*
  This file is part of libkdepim.

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
#ifndef URIHANDLER_H
#define URIHANDLER_H

#include "kdepimdbusinterfaces_export.h"
#include <akonadi/item.h>
class QString;

class KDEPIMDBUSINTERFACES_EXPORT UriHandler
{
public:
    /**
      Process URI (e.g. open mailer, open browser, open incidence viewer etc.).
        @return true if handler handled the URI, otherwise false.
        @param uri The URI of the link that should be handled.
    */
    static bool process( const QString &uri, const Akonadi::Item & item = Akonadi::Item() );
};

#endif
