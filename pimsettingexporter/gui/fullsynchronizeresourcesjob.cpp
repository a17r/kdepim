/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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


#include "fullsynchronizeresourcesjob.h"

#include <synchronizeresourcejob.h>

FullSynchronizeResourcesJob::FullSynchronizeResourcesJob(QObject *parent)
    : QObject(parent),
      mWindowParent(Q_NULLPTR)
{

}

FullSynchronizeResourcesJob::~FullSynchronizeResourcesJob()
{

}

void FullSynchronizeResourcesJob::setResources(const QStringList &lst)
{
    if (lst.isEmpty()) {
        Q_EMIT synchronizeFinished();
        deleteLater();
    } else {
        mResources = lst;
    }
}

void FullSynchronizeResourcesJob::setWindowParent(QWidget *parent)
{
    mWindowParent = parent;
}

void FullSynchronizeResourcesJob::start()
{
    SynchronizeResourceJob *job = new SynchronizeResourceJob(this);
    //Full synch
    job->setSynchronizeOnlyCollection(false);
    job->setListResources(mResources);
    connect(job, &SynchronizeResourceJob::synchronizationFinished, this, &FullSynchronizeResourcesJob::synchronizeFinished);
    connect(job, &SynchronizeResourceJob::synchronizationInstanceDone, this, &FullSynchronizeResourcesJob::slotSynchronizeInstanceDone);
    connect(job, &SynchronizeResourceJob::synchronizationInstanceFailed, this, &FullSynchronizeResourcesJob::slotSynchronizeInstanceFailed);
    job->start();
}

void FullSynchronizeResourcesJob::slotSynchronizeInstanceDone(const QString &identifier)
{
    Q_EMIT synchronizeInstanceDone(identifier);
    //TODO increase progress indicator
}

void FullSynchronizeResourcesJob::slotSynchronizeInstanceFailed(const QString &identifier)
{
    Q_EMIT synchronizeInstanceFailed(identifier);
    //TODO increase progress indicator
}