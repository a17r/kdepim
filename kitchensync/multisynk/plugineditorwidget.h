/*
    This file is part of KitchenSync.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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
*/

#ifndef PLUGINEDITORWIDGET_H
#define PLUGINEDITORWIDGET_H

#include <qwidget.h>

class KComboBox;

class QLabel;
class QPushButton;

class PluginEditorWidget : public QWidget
{
  Q_OBJECT

  public:
    PluginEditorWidget( QWidget *parent = 0, const char *name = 0 );
    ~PluginEditorWidget();

    void setLabel( const QString &label );

    void set( KonnectorPair *pair, KSync::Konnector *konnector );
    void get( KonnectorPair *pair );

  private slots:
    void typeChanged( int );
    void changeOptions();

  private:
    void initGUI();
    void fillTypeBox();
    QString currentType() const;

    KComboBox *mTypeBox;

    QLabel *mInfoLabel;
    QLabel *mLabel;
    QPushButton *mOptionButton;

    KonnectorPair *mPair;
    KSync::Konnector *mKonnector;
    KSync::Konnector *mOldKonnector;
};

#endif
