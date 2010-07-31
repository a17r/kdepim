/*
 *   This file is part of ScalixAdmin.
 *
 *   Copyright (C) 2007 Trolltech ASA. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PASSWORDPAGE_H
#define PASSWORDPAGE_H

#include <tqwidget.h>

class QLineEdit;
class QPushButton;

namespace KIO {
class Job;
}

class PasswordPage : public QWidget
{
  Q_OBJECT

  public:
    PasswordPage( TQWidget *parent = 0 );

  private slots:
    void buttonClicked();
    void finished( KIO::Job* );
    void textChanged();

  private:
    void updateState( bool );

    TQLineEdit *mPassword;
    TQLineEdit *mPasswordRetype;
    TQPushButton *mButton;

    KIO::Job *mJob;
};

#endif
