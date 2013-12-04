/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#ifndef STORAGESERVICEABSTRACT_H
#define STORAGESERVICEABSTRACT_H

#include <QObject>
#include <QUrl>

namespace PimCommon {

struct AccountInfo {
    AccountInfo()
        : accountSize(-1),
          quota(-1),
          shared(-1)
    {

    }
    qint64 accountSize;
    qint64 quota;
    qint64 shared;
    QString displayName;
};

class StorageServiceAbstract : public QObject
{
    Q_OBJECT
public:
    explicit StorageServiceAbstract(QObject *parent=0);
    ~StorageServiceAbstract();

    virtual void uploadFile(const QString &filename) = 0;
    virtual void accountInfo() = 0;
    virtual void createFolder(const QString &folder) = 0;
    virtual void listFolder() = 0;
    virtual void removeConfig() = 0;
    virtual void authentification() = 0;
    virtual void shareLink(const QString &root, const QString &path) = 0;

Q_SIGNALS:
    void actionFailed(const QString &serviceName, const QString &error);
    void accountInfoDone(const QString &serviceName, const PimCommon::AccountInfo &);
    void uploadFileProgress(const QString &serviceName, qint64 done,qint64 total);
    void shareLinkDone(const QString &serviceName, const QString &link);
    void downloadDone();
    void downloadFailed();
};
}

#endif // STORAGESERVICEABSTRACT_H
