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

#include "folderarchivemanager.h"
#include "folderarchiveagentjob.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchivekernel.h"

#include <mailcommon/kernel/mailkernel.h>

#include <Akonadi/AgentManager>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/CollectionFetchJob>

#include <KSharedConfig>
#include <KGlobal>
#include <KNotification>
#include <KIcon>
#include <KLocale>
#include <KIconLoader>

FolderArchiveManager::FolderArchiveManager(QObject *parent)
    : QObject(parent),
      mCurrentJob(0)
{
    mFolderArchivelKernel = new FolderArchiveKernel( this );
    CommonKernel->registerKernelIf( mFolderArchivelKernel ); //register KernelIf early, it is used by the Filter classes
    CommonKernel->registerSettingsIf( mFolderArchivelKernel ); //SettingsIf is used in FolderTreeWidget

    connect( Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
             this, SLOT(slotInstanceRemoved(Akonadi::AgentInstance)) );

}

FolderArchiveManager::~FolderArchiveManager()
{
    qDeleteAll(mListAccountInfo);
    mListAccountInfo.clear();
    qDeleteAll(mJobQueue);
    delete mCurrentJob;
}

void FolderArchiveManager::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_FOREACH (FolderArchiveAccountInfo *info, mListAccountInfo) {
        if (info->archiveTopLevel() == collection.id()) {
            info->setArchiveTopLevel(-1);
            KConfigGroup group = KGlobal::config()->group(QLatin1String("FolderArchiveAccount ") + info->instanceName());
            info->writeConfig(group);
        }
    }
    load();
}

FolderArchiveAccountInfo *FolderArchiveManager::infoFromInstanceName(const QString &instanceName) const
{
    Q_FOREACH (FolderArchiveAccountInfo *info, mListAccountInfo) {
        if (info->instanceName() == instanceName) {
            return info;
        }
    }
    return 0;
}

void FolderArchiveManager::setArchiveItem(qlonglong itemId)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( Akonadi::Item(itemId), this );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().setFetchRemoteIdentification(true);
    connect( job, SIGNAL(result(KJob*)), SLOT(slotFetchParentCollection(KJob*)) );
}

void FolderArchiveManager::slotFetchParentCollection(KJob *job)
{
    if ( job->error() ) {
        moveFailed(i18n("Unable to fetch folder. Error reported:%1",job->errorString()));
        kDebug()<<"Unable to fetch folder:"<<job->errorString();
        return;
    }
    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
    const Akonadi::Item::List items = fetchJob->items();
    if (items.isEmpty()) {
        moveFailed(i18n("No folder returned."));
        kDebug()<<"Fetch list is empty";
    } else {
        Akonadi::CollectionFetchJob* jobCol = new Akonadi::CollectionFetchJob( Akonadi::Collection(items.first().parentCollection().id()), Akonadi::CollectionFetchJob::Base, this );
        jobCol->setProperty("itemId", items.first().id());
        connect( jobCol, SIGNAL(result(KJob*)), SLOT(slotFetchCollection(KJob*)) );
    }
}

void FolderArchiveManager::slotFetchCollection(KJob *job)
{
    if ( job->error() ) {
        moveFailed(i18n("Unable to fetch parent folder. Error reported: %1", job->errorString()));
        kDebug()<<"can not fetch collection "<<job->errorString();
        return;
    }
    Akonadi::CollectionFetchJob* jobCol = qobject_cast<Akonadi::CollectionFetchJob*>(job);
    if (jobCol->collections().isEmpty()) {
        moveFailed(i18n("Unable to return list of folders."));
        kDebug()<<"List of folder is empty";
        return;
    }

    QList<qlonglong> itemIds;
    itemIds << jobCol->property("itemId").toLongLong();
    setArchiveItems(itemIds, jobCol->collections().first().resource());
}

void FolderArchiveManager::setArchiveItems(const QList<qlonglong> &itemIds, const QString &instanceName)
{
    FolderArchiveAccountInfo *info = infoFromInstanceName(instanceName);
    if (info) {
        FolderArchiveAgentJob *job = new FolderArchiveAgentJob(this, info, itemIds);
        if (mCurrentJob) {
            mJobQueue.enqueue(job);
        } else {
            mCurrentJob = job;
            job->start();
        }
    }
}

void FolderArchiveManager::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    const QString instanceName = instance.name();
    Q_FOREACH (FolderArchiveAccountInfo *info, mListAccountInfo) {
        if (info->instanceName() == instanceName) {
            mListAccountInfo.removeAll(info);
            removeInfo(instanceName);
            break;
        }
    }
}

void FolderArchiveManager::removeInfo(const QString &instanceName)
{
    KConfigGroup group = KGlobal::config()->group(QLatin1String("FolderArchiveAccount ") + instanceName);
    group.deleteGroup();
    KGlobal::config()->sync();
}

void FolderArchiveManager::load()
{
    qDeleteAll(mListAccountInfo);
    mListAccountInfo.clear();

    const QStringList accountList = KGlobal::config()->groupList().filter( QRegExp( QLatin1String("FolderArchiveAccount ") ) );
    Q_FOREACH (const QString &account, accountList) {
        KConfigGroup group = KGlobal::config()->group(account);
        FolderArchiveAccountInfo *info = new FolderArchiveAccountInfo(group);
        if (info->enabled()) {
            mListAccountInfo.append(info);
        } else {
            delete info;
        }
    }
}

void FolderArchiveManager::moveDone()
{
    const QPixmap pixmap = KIcon( QLatin1String("kmail") ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );

    KNotification::event( QLatin1String("folderarchivedone"),
                          i18n("Messages archived"),
                          pixmap,
                          0,
                          KNotification::CloseOnTimeout,
                          KGlobal::mainComponent());
    nextJob();
}

void FolderArchiveManager::moveFailed(const QString &msg)
{
    const QPixmap pixmap = KIcon( QLatin1String("kmail") ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );

    KNotification::event( QLatin1String("folderarchiveerror"),
                          msg,
                          pixmap,
                          0,
                          KNotification::CloseOnTimeout,
                          KGlobal::mainComponent());
    nextJob();
}

void FolderArchiveManager::nextJob()
{
    mCurrentJob->deleteLater();
    if (mJobQueue.isEmpty()) {
        mCurrentJob = 0;
    } else {
        mCurrentJob = mJobQueue.dequeue();
        mCurrentJob->start();
    }
}

#include "folderarchivemanager.moc"
