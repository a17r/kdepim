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

#ifndef STORAGESERVICEABSTRACTJOB_H
#define STORAGESERVICEABSTRACTJOB_H

#include <QObject>
#include <QNetworkReply>
#include "storageservice/storageserviceabstract.h"

class QNetworkAccessManager;
namespace PimCommon {
class AccountInfo;
class StorageServiceAbstractJob : public QObject
{
    Q_OBJECT
public:
    explicit StorageServiceAbstractJob(QObject *parent = 0);
    ~StorageServiceAbstractJob();

    virtual void requestTokenAccess() = 0;
    virtual void uploadFile(const QString &filename) = 0;
    virtual void listFolder(const QString &folder = QString()) = 0;
    virtual void accountInfo() = 0;
    virtual void createFolder(const QString &filename=QString()) = 0;
    virtual void shareLink(const QString &root, const QString &path) = 0;
    virtual void createServiceFolder() = 0;
    virtual void downloadFile(const QString &filename) = 0;
    virtual void deleteFile(const QString &filename) = 0;
    virtual void deleteFolder(const QString &foldername) = 0;
    virtual void renameFolder(const QString &source, const QString &destination) = 0;

protected Q_SLOTS:
    void slotError(QNetworkReply::NetworkError);

Q_SIGNALS:
    void actionFailed(const QString &data);
    void shareLinkDone(const QString &url);
    void accountInfoDone(const PimCommon::AccountInfo &data);
    void uploadFileProgress(qint64 done, qint64 total);
    void createFolderDone(const QString &folderName);
    void uploadFileDone(const QString &fileName);
    void listFolderDone(const QString &listFolder);
    void authorizationFailed(const QString &error);
    void downLoadFileDone(const QString &filename);
    void deleteFileDone(const QString &filename);
    void deleteFolderDone(const QString &filename);

protected:
    void errorMessage(PimCommon::StorageServiceAbstract::ActionType type, const QString &errorStr);

    QNetworkAccessManager *mNetworkAccessManager;
    PimCommon::StorageServiceAbstract::ActionType mActionType;
    bool mError;
};
}

#endif // STORAGESERVICEABSTRACTJOB_H
