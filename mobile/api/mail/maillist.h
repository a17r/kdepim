/*
 Copyright 2014  Michael Bohlender michael.bohlender@kdemail.net

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; either version 2 of
 the License or (at your option) version 3 or any later version
 accepted by the membership of KDE e.V. (or its successor approved
 by the membership of KDE e.V.), which shall act as a proxy
 defined in Section 14 of version 3 of the license.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef MAILLIST_H
#define MAILLIST_H

#include "maillistmodel.h"

#include <AkonadiCore/Item>

#include <QObject>
#include <QScopedPointer>
#include <QUrl>
#include <QStringList>

class MailList : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QAbstractItemModel* model READ model CONSTANT )

public:
    explicit MailList( QObject *parent = Q_NULLPTR );

    MailListModel *model() const;

public Q_SLOTS:
    void loadCollection( const QUrl &akonadiUrl );

private Q_SLOTS:
    void slotItemsReceived( const Akonadi::Item::List &itemList );

private:
    QScopedPointer<MailListModel> m_model;
};

#endif

