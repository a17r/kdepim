/*
   Copyright (C) 2013-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef CONTACTPREVIEWWIDGET_H
#define CONTACTPREVIEWWIDGET_H

#include <QWidget>

#include <KContacts/Addressee>

class QTabWidget;

namespace Akonadi
{
class ContactViewer;
class ContactGroupViewer;
}

namespace KAddressBookGrantlee
{
class GrantleeContactFormatter;
class GrantleeContactGroupFormatter;
}

class ContactPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ContactPreviewWidget(const QString &projectDirectory, QWidget *parent = Q_NULLPTR);
    ~ContactPreviewWidget();

    void updateViewer();
    void createScreenShot(const QStringList &fileName);
    void setThemePath(const QString &projectDirectory);
    void loadConfig();

private:
    KContacts::Addressee mContact;
    Akonadi::ContactViewer *mContactViewer;
    Akonadi::ContactGroupViewer *mGroupViewer;

    KAddressBookGrantlee::GrantleeContactFormatter *mFormatter;
    KAddressBookGrantlee::GrantleeContactGroupFormatter *mGroupFormatter;

    QTabWidget *mTabWidget;
};

#endif // CONTACTPREVIEWWIDGET_H
