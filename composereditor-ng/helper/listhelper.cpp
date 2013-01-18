/*
  Copyright (c) 2012-2013 Montel Laurent <montel.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "listhelper_p.h"

namespace ComposerEditorNG {
static QString OL = QLatin1String("ol");
static QString UL = QLatin1String("ul");

QWebElement ListHelper::ulElement(const QWebElement& element)
{
    if (element.isNull())
        return element;
    const QString tagName = element.tagName();
    if (tagName == UL) {
        return element;
    } else {
        QWebElement e = element;
        do {
            e = e.parent();
        } while( (e.tagName().toLower() != UL) && !e.isNull() );
        return e;
    }
    return element;
}

QWebElement ListHelper::olElement(const QWebElement& element)
{
    if (element.isNull())
        return element;
    const QString tagName = element.tagName();
    if (tagName == OL) {
        return element;
    } else {
        QWebElement e = element;
        do {
            e = e.parent();
        } while( (e.tagName().toLower() != OL) && !e.isNull() );
        return e;
    }
    return element;
}

QWebElement ListHelper::listElement(const QWebElement& element)
{
    if (element.isNull())
        return element;
    const QString tagName = element.tagName();
    if ((tagName == OL) || (tagName == UL)) {
        return element;
    } else {
        QWebElement e = element;
        do {
            e = e.parent();
        } while( ((e.tagName().toLower() != OL) || (e.tagName().toLower() != UL)) && !e.isNull() );
        return e;
    }
    return element;
}

}
