/*
 *  emailidcombo.cpp  -  email identity combo box with read-only option
 *  Program:  kalarm
 *  Copyright © 2004 by David Jarvie <djarvie@kde.org>
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

#include "emailidcombo.h"

#include <QMouseEvent>
#include <QKeyEvent>


EmailIdCombo::EmailIdCombo(KIdentityManagement::IdentityManager* manager, QWidget* parent)
    : KIdentityManagement::IdentityCombo(manager, parent),
      mReadOnly(false)
{ }

void EmailIdCombo::mousePressEvent(QMouseEvent* e)
{
    if (mReadOnly)
    {
        // Swallow up the event if it's the left button
        if (e->button() == Qt::LeftButton)
            return;
    }
    KIdentityManagement::IdentityCombo::mousePressEvent(e);
}

void EmailIdCombo::mouseReleaseEvent(QMouseEvent* e)
{
    if (!mReadOnly)
        KIdentityManagement::IdentityCombo::mouseReleaseEvent(e);
}

void EmailIdCombo::mouseMoveEvent(QMouseEvent* e)
{
    if (!mReadOnly)
        KIdentityManagement::IdentityCombo::mouseMoveEvent(e);
}

void EmailIdCombo::keyPressEvent(QKeyEvent* e)
{
    if (!mReadOnly  ||  e->key() == Qt::Key_Escape)
        KIdentityManagement::IdentityCombo::keyPressEvent(e);
}

void EmailIdCombo::keyReleaseEvent(QKeyEvent* e)
{
    if (!mReadOnly)
        KIdentityManagement::IdentityCombo::keyReleaseEvent(e);
}

// vim: et sw=4:
