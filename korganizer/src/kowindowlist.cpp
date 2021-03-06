/*
  This file is part of KOrganizer.

  Copyright (c) 2000,2003 Cornelius Schumacher <schumacher@kde.org>

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

#include "kowindowlist.h"
#include "mainwindow.h"

#include <QUrl>

KOWindowList::KOWindowList()
    : QObject(Q_NULLPTR), mDefaultWindow(Q_NULLPTR)
{
}

KOWindowList::~KOWindowList()
{
}

void KOWindowList::addWindow(KOrg::MainWindow *korg)
{
    if (!korg->hasDocument()) {
        mDefaultWindow = korg;
    } else {
        mWindowList.append(korg);
    }
}

void KOWindowList::removeWindow(KOrg::MainWindow *korg)
{
    if (korg == mDefaultWindow) {
        mDefaultWindow = Q_NULLPTR;
    } else {
        mWindowList.removeAll(korg);
    }
}

bool KOWindowList::lastInstance()
{
    if (mWindowList.count() == 1 && !mDefaultWindow) {
        return true;
    }

    if (mWindowList.count() == 0 && mDefaultWindow) {
        return true;
    } else {
        return false;
    }
}

KOrg::MainWindow *KOWindowList::findInstance(const QUrl &url)
{
    foreach (KOrg::MainWindow *inst, mWindowList) {
        if (inst && inst->getCurrentURL() == url) {
            return inst;
        }
    }
    return Q_NULLPTR;
}

KOrg::MainWindow *KOWindowList::defaultInstance()
{
    return mDefaultWindow;
}

