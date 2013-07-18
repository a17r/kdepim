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

#include "folderarchiveagentjob.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchivemanager.h"

#include <Akonadi/ItemMoveJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemMoveJob>

#include <KLocale>

FolderArchiveAgentJob::FolderArchiveAgentJob(FolderArchiveManager *manager, FolderArchiveAccountInfo *info, const QList<qint64> &lstItem, QObject *parent)
    : QObject(parent),
      mLstItem(lstItem),
      mManager(manager),
      mInfo(info)
{
}

FolderArchiveAgentJob::~FolderArchiveAgentJob()
{
}

void FolderArchiveAgentJob::start()
{
    Akonadi::CollectionFetchJob *fetchCollection = new Akonadi::CollectionFetchJob( Akonadi::Collection(mInfo->archiveTopLevel()), Akonadi::CollectionFetchJob::Base );
    connect( fetchCollection, SIGNAL(result(KJob*)), this, SLOT(slotFetchCollection(KJob*)));
}

void FolderArchiveAgentJob::slotFetchCollection(KJob *job)
{
    if ( job->error() ) {
        sendError(i18n("Can not fetch collection. %1", job->errorString() ));
        return;
    }
    Akonadi::CollectionFetchJob *fetchCollectionJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    Akonadi::Collection::List collections = fetchCollectionJob->collections();
    if (collections.isEmpty()) {
        sendError(i18n("List of collection is empty. %1", job->errorString() ));
        return;
    }
    Akonadi::Item::List lst;
    Q_FOREACH (qint64 i, mLstItem) {
        lst.append(Akonadi::Item(i));
    }

    Akonadi::ItemMoveJob *moveJob = new Akonadi::ItemMoveJob(lst, collections.first());
    connect( moveJob, SIGNAL(result(KJob*)), this, SLOT(slotMoveMessages(KJob*)));
}

void FolderArchiveAgentJob::slotMoveMessages(KJob *job)
{
    if ( job->error() ) {
        sendError(i18n("Can not move messages. %1", job->errorString() ));
        return;
    }
    //TODO
    mManager->moveDone(QString());
}

void FolderArchiveAgentJob::sendError(const QString &error)
{
    mManager->moveFailed(error);
}

#include "folderarchiveagentjob.moc"
