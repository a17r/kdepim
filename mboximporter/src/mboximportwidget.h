/*
   Copyright (C) 2012-2016 Montel Laurent <montel@kde.org>

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

#ifndef IMPORTMAILPAGE_H
#define IMPORTMAILPAGE_H

#include <QWidget>

namespace Ui
{
class MBoxImportWidget;
}

namespace Akonadi
{
class Collection;
}

namespace MailImporter
{
class ImportMailsWidget;
}

class MBoxImportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MBoxImportWidget(QWidget *parent = Q_NULLPTR);
    ~MBoxImportWidget();
    MailImporter::ImportMailsWidget *mailWidget();
    Akonadi::Collection selectedCollection() const;
    void setImportButtonEnabled(bool enabled);

private Q_SLOTS:
    void collectionChanged(const Akonadi::Collection &collection);

Q_SIGNALS:
    void importMailsClicked();

private:
    Ui::MBoxImportWidget *ui;
};

#endif // IMPORTMAILPAGE_H
