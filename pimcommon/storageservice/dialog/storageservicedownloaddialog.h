/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef STORAGESERVICEDOWNLOADDIALOG_H
#define STORAGESERVICEDOWNLOADDIALOG_H

#include <KDialog>
#include "pimcommon_export.h"

class QTreeWidgetItem;
namespace PimCommon {
class StorageServiceAbstract;
class StorageServiceTreeWidget;
class StorageServiceProgressWidget;
class PIMCOMMON_EXPORT StorageServiceDownloadDialog : public KDialog
{
    Q_OBJECT
public:
    explicit StorageServiceDownloadDialog(PimCommon::StorageServiceAbstract *storage, const QString &destination, QWidget *parent=0);
    ~StorageServiceDownloadDialog();

private slots:
    void slotItemActivated(QTreeWidgetItem *item, int column);
    void slotDownloadFile();

    void slotDownfileDone(const QString &serviceName, const QString &filename);

    void slotDownfileFailed(const QString &serviceName, const QString &filename);
private:
    void readConfig();
    void writeConfig();
    QString mDestination;
    StorageServiceTreeWidget *mTreeWidget;
    PimCommon::StorageServiceAbstract *mStorage;
    PimCommon::StorageServiceProgressWidget *mProgressWidget;
};
}

#endif // STORAGESERVICEDOWNLOADDIALOG_H
