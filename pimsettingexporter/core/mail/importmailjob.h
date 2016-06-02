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

#ifndef ImportMailJob_H
#define ImportMailJob_H

#include "abstractimportexportjob.h"
#include <AkonadiCore/Collection>
#include <QStringList>
#include <QHash>
#include <KArchive>
class KArchiveDirectory;
class KArchiveFile;
class ArchiveStorage;

class ImportMailJob : public AbstractImportExportJob
{
    Q_OBJECT
public:
    explicit ImportMailJob(QObject *widget, Utils::StoredTypes typeSelected, ArchiveStorage *archiveStorage, int numberOfStep);
    ~ImportMailJob();

    void start() Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void slotNextStep() Q_DECL_OVERRIDE;

private:
    void restoreTransports();
    void restoreResources();
    void restoreMails();
    void restoreConfig();
    void restoreIdentity();
    void restoreAkonadiDb();

    void importTemplatesConfig(const KArchiveFile *templatesconfiguration, const QString &templatesconfigurationrc, const QString &filename, const QString &prefix);
    void importKmailConfig(const KArchiveFile *kmailsnippet, const QString &kmail2rc, const QString &filename, const QString &prefix);
    void importArchiveConfig(const KArchiveFile *archiveconfiguration, const QString &archiveconfigurationrc, const QString &filename, const QString &prefix);
    void importFolderArchiveConfig(const KArchiveFile *archiveconfiguration, const QString &archiveconfigurationrc, const QString &filename, const QString &prefix);
    void searchAllMailsFiles(const KArchiveDirectory *dir, const QString &prefix);
    void storeMailArchiveResource(const KArchiveDirectory *dir, const QString &prefix);

    void mergeLdapConfig(const KArchiveFile *archivefile, const QString &filename, const QString &prefix);
    void mergeKmailSnippetConfig(const KArchiveFile *archivefile, const QString &filename, const QString &prefix);
    void mergeArchiveMailAgentConfig(const KArchiveFile *archivefile, const QString &filename, const QString &prefix);
    void copyArchiveMailAgentConfigGroup(KSharedConfig::Ptr archiveConfigOrigin, KSharedConfig::Ptr archiveConfigDestination);
    void mergeSieveTemplate(const KArchiveFile *archivefile, const QString &filename, const QString &prefix);
    void importSimpleFilesInDirectory(const QString &relativePath);
    QString uniqueIdentityName(const QString &name);

    QHash<uint, uint> mHashIdentity;
    QHash<int, int> mHashTransport;
    QHash<QString, QString> mHashResources;
    QStringList mFileList;
};

#endif // ImportMailJob_H
