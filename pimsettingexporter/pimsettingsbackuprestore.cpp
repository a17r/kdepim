/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "pimsettingsbackuprestore.h"
#include "archivestorage.h"
#include "importexportprogressindicatorgui.h"

#include "mail/exportmailjob.h"
#include "mail/importmailjob.h"

#include "addressbook/exportaddressbookjob.h"
#include "addressbook/importaddressbookjob.h"

#include "calendar/exportcalendarjob.h"
#include "calendar/importcalendarjob.h"

#include "alarm/exportalarmjob.h"
#include "alarm/importalarmjob.h"

#include "notes/exportnotesjob.h"
#include "notes/importnotesjob.h"

#include "akregator/exportakregatorjob.h"
#include "akregator/importakregatorjob.h"

#include "blogilo/exportblogilojob.h"
#include "blogilo/importblogilojob.h"

#include <KLocalizedString>
#include <KMessageBox>

#include "pimsettingexport_debug.h"
#include <QDateTime>
#include <QLocale>

PimSettingsBackupRestore::PimSettingsBackupRestore(QWidget *parentWidget, QObject *parent)
    : QObject(parent),
      mParentWidget(parentWidget),
      mImportExportData(Q_NULLPTR),
      mArchiveStorage(Q_NULLPTR)
{

}

PimSettingsBackupRestore::~PimSettingsBackupRestore()
{
    delete mImportExportData;
}

void PimSettingsBackupRestore::setStoredParameters(const QHash<Utils::AppsType, Utils::importExportParameters> &stored)
{
    mStored = stored;
}

bool PimSettingsBackupRestore::openArchive(const QString &filename, bool readWrite)
{
    mArchiveStorage = new ArchiveStorage(filename, this);
    if (!mArchiveStorage->openArchive(readWrite)) {
        delete mArchiveStorage;
        mArchiveStorage = Q_NULLPTR;
        return false;
    }
    return true;
}

void PimSettingsBackupRestore::backupStart(const QString &filename)
{
    if (mStored.isEmpty()) {
        Q_EMIT jobFailed();
        deleteLater();
        return;
    }
    if (!openArchive(filename, true)) {
        Q_EMIT jobFailed();
        deleteLater();
        return;
    }
    Q_EMIT updateActions(true);
    mAction = Backup;
    mStoreIterator = mStored.constBegin();
    const QDateTime now = QDateTime::currentDateTime();
    Q_EMIT addInfo(QLatin1Char('[') + QLocale().toString((now), QLocale::ShortFormat) + QLatin1Char(']'));
    Q_EMIT addInfo(i18n("Start to backup data in \'%1\'", mArchiveStorage->filename()));
    Q_EMIT addEndLine();
    //Add version
    Utils::addVersion(mArchiveStorage->archive());
    backupNextStep();
}

void PimSettingsBackupRestore::backupNextStep()
{
    if (mStoreIterator != mStored.constEnd()) {
        switch (mStoreIterator.key()) {
        case Utils::KMail:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportMailJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KAddressBook:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportAddressbookJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KAlarm:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportAlarmJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KOrganizer:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportCalendarJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KNotes:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportNotesJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::Akregator:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportAkregatorJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::Blogilo:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ExportBlogiloJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::Unknown:
            break;
        }
    } else {
        backupFinished();
    }
}

void PimSettingsBackupRestore::backupFinished()
{
    Q_EMIT addInfo(i18n("Backup in \'%1\' done.", mArchiveStorage->filename()));
    //At the end
    mArchiveStorage->closeArchive();
    delete mArchiveStorage;
    mArchiveStorage = Q_NULLPTR;
    delete mImportExportData;
    mImportExportData = Q_NULLPTR;
    Q_EMIT backupDone();
    Q_EMIT showBackupFinishDialogInformation();
    Q_EMIT updateActions(false);
    deleteLater();
}

void PimSettingsBackupRestore::restoreNextStep()
{
    if (mStoreIterator != mStored.constEnd()) {
        switch (mStoreIterator.key()) {
        case Utils::KMail:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportMailJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KAddressBook:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportAddressbookJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KAlarm:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportAlarmJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KOrganizer:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportCalendarJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::KNotes:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportNotesJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::Akregator:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportAkregatorJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::Blogilo:
            if (mStoreIterator.value().numberSteps != 0) {
                mImportExportData = new ImportBlogiloJob(mParentWidget, mStoreIterator.value().types, mArchiveStorage, mStoreIterator.value().numberSteps);
                executeJob();
            }
            break;
        case Utils::Unknown:
            break;
        }
    } else {
        restoreFinished();
    }
}

bool PimSettingsBackupRestore::continueToRestore()
{
    return true;
}

bool PimSettingsBackupRestore::restoreStart(const QString &filename)
{
    if (mStored.isEmpty()) {
        Q_EMIT jobFailed();
        deleteLater();
        return false;
    }
    if (!openArchive(filename, false)) {
        Q_EMIT jobFailed();
        deleteLater();
        return false;
    }
    Q_EMIT updateActions(true);
    mAction = Restore;
    mStoreIterator = mStored.constBegin();
    const int version = Utils::archiveVersion(mArchiveStorage->archive());
    if (version > Utils::currentArchiveVersion()) {
        if (!continueToRestore())
            return false;
    }
    qCDebug(PIMSETTINGEXPORTER_LOG) << " version " << version;
    AbstractImportExportJob::setArchiveVersion(version);

    const QDateTime now = QDateTime::currentDateTime();
    Q_EMIT addInfo(QLatin1Char('[') + QLocale().toString((now), QLocale::ShortFormat) + QLatin1Char(']'));

    Q_EMIT addInfo(i18n("Start to restore data from \'%1\'", mArchiveStorage->filename()));
    Q_EMIT addEndLine();
    restoreNextStep();
    return true;
}

void PimSettingsBackupRestore::restoreFinished()
{
    Q_EMIT addInfo(i18n("Restoring data from \'%1\' done.", mArchiveStorage->filename()));
    //At the end
    mArchiveStorage->closeArchive();
    delete mArchiveStorage;
    mArchiveStorage = Q_NULLPTR;
    delete mImportExportData;
    mImportExportData = Q_NULLPTR;
    Q_EMIT updateActions(false);
    deleteLater();
}

void PimSettingsBackupRestore::executeJob()
{
    mImportExportData->setImportExportProgressIndicator(new ImportExportProgressIndicatorGui(mParentWidget, this));
    connect(mImportExportData, &AbstractImportExportJob::info, this, &PimSettingsBackupRestore::addInfo);
    connect(mImportExportData, &AbstractImportExportJob::error, this, &PimSettingsBackupRestore::addError);
    connect(mImportExportData, &AbstractImportExportJob::title, this, &PimSettingsBackupRestore::addTitle);
    connect(mImportExportData, &AbstractImportExportJob::endLine, this, &PimSettingsBackupRestore::addEndLine);
    connect(mImportExportData, &AbstractImportExportJob::jobFinished, this, &PimSettingsBackupRestore::jobFinished);
    mImportExportData->start();
}

void PimSettingsBackupRestore::slotJobFinished()
{
    ++mStoreIterator;
    Q_EMIT addEndLine();
    delete mImportExportData;
    mImportExportData = Q_NULLPTR;
    switch (mAction) {
    case Backup:
        backupNextStep();
        break;
    case Restore:
        restoreNextStep();
        break;
    }
}
