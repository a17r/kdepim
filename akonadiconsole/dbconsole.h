/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADICONSOLE_DBCONSOLE_H
#define AKONADICONSOLE_DBCONSOLE_H

#include "ui_dbconsole.h"

class QSqlQueryModel;

class DbConsole : public QWidget
{
    Q_OBJECT
public:
    explicit DbConsole(QWidget *parent = Q_NULLPTR);

private Q_SLOTS:
    void execClicked();
    void copyCell();

private:
    Ui::DbConsole ui;
    QSqlQueryModel *mQueryModel;
};

#endif
