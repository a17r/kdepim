/*
    This file is part of KitchenSync.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/
#ifndef CONFIGGUISYNCML_H
#define CONFIGGUISYNCML_H

#include <tqdom.h>

#include "configgui.h"

class QCheckBox;
class QComboBox;
class QGridLayout;
class QSpinBox;
class KComboBox;
class KLineEdit;

class ConfigGuiSyncmlHttp : public ConfigGui
{

  Q_OBJECT

  public:
    ConfigGuiSyncmlHttp( const QSync::Member &, TQWidget *parent );

    void load( const TQString &xml );
    TQString save() const;

  private:
    TQGridLayout *mGridLayout;

    KLineEdit *mUsername;
    KLineEdit *mPassword;
    KLineEdit *mUrl;
    TQSpinBox *mPort;
    TQCheckBox *mUseStringTable;
    TQCheckBox *mOnlyReplace;

    TQSpinBox *mRecvLimit;
    TQSpinBox *mMaxObjSize;

    KComboBox *mContactDb;
    KComboBox *mCalendarDb;
    KComboBox *mNoteDb;

  protected slots:
    void addLineEdit( TQWidget *parent, const TQString &text, KComboBox **edit, int row );
};

#endif
