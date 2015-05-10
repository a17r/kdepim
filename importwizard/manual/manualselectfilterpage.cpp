/***************************************************************************
                          kselfilterpage.cpp  -  description
                             -------------------
    begin                : Fri Jan 17 2003
    copyright            : (C) 2003 by Laurence Anderson
    email                : l.d.anderson@warwick.ac.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Local includes
#include "manualselectfilterpage.h"

// Filter includes
#include <filter_mbox.h>
#include <filter_oe.h>
#include <filter_pmail.h>
#include <filter_plain.h>
#include <filter_evolution.h>
#include <filter_mailapp.h>
#include <filter_evolution_v2.h>
#include <filter_opera.h>
#include <filter_thunderbird.h>
#include <filter_kmail_maildir.h>
#include <filter_kmail_archive.h>
#include <filter_sylpheed.h>
#include <filter_thebat.h>
#include <filter_lnotes.h>
#include <filter_mailmangzip.h>
#include <filtericedove.h>

#include <filters.h>

#include <filter_evolution_v3.h>

// KDE includes

#include <KLocalizedString>

// Qt includes
#include <QCheckBox>

#include <QStandardPaths>

using namespace MailImporter;

ManualSelectFilterPage::ManualSelectFilterPage(QWidget *parent)
    : QWidget(parent)
{
    mWidget = new Ui::ManualSelectFilterPage;
    mWidget->setupUi(this);
    mWidget->mIntroSidebar->setPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("importwizard/pics/step1.png")));
    connect(mWidget->mFilterCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &ManualSelectFilterPage::filterSelected);

    // Add new filters below. If this annoys you, please rewrite the stuff to use a factory.
    // The former approach was overengineered and only worked around problems in the design
    // For now, we have to live without the warm and fuzzy feeling a refactoring might give.
    // Patches appreciated. (danimo)

    addFilter(new MailImporter::FilterKMailArchive);
    addFilter(new MailImporter::FilterMBox);
    addFilter(new MailImporter::FilterEvolution);
    addFilter(new MailImporter::FilterEvolution_v2);
    addFilter(new MailImporter::FilterEvolution_v3);
    addFilter(new MailImporter::FilterKMail_maildir);
    addFilter(new MailImporter::FilterMailApp);
    addFilter(new MailImporter::FilterOpera);
    addFilter(new MailImporter::FilterSylpheed);
    addFilter(new MailImporter::FilterThunderbird);
    addFilter(new MailImporter::FilterIcedove);
    addFilter(new MailImporter::FilterTheBat);
    addFilter(new MailImporter::FilterOE);
    addFilter(new MailImporter::FilterPMail);
    addFilter(new MailImporter::FilterLNotes);
    addFilter(new MailImporter::FilterPlain);
    addFilter(new MailImporter::FilterMailmanGzip);

    // Ensure we return the correct type of Akonadi collection.
    mWidget->mCollectionRequestor->setMustBeReadWrite(true);
}

ManualSelectFilterPage::~ManualSelectFilterPage()
{
    qDeleteAll(mFilterList);
    mFilterList.clear();
    delete mWidget;
}

void ManualSelectFilterPage::filterSelected(int i)
{
    QString info = mFilterList.at(i)->info();
    const QString author = mFilterList.at(i)->author();
    if (!author.isEmpty()) {
        info += i18n("<p><i>Written by %1.</i></p>", author);
    }
    mWidget->mDesc->setText(info);
}

void ManualSelectFilterPage::addFilter(Filter *f)
{
    mFilterList.append(f);
    mWidget->mFilterCombo->addItem(f->name());
    if (mWidget->mFilterCombo->count() == 1) {
        filterSelected(0);    // Setup description box with fist filter selected
    }
}

bool ManualSelectFilterPage::removeDupMsg_checked() const
{
    return mWidget->remDupMsg->isChecked();
}

Filter *ManualSelectFilterPage::getSelectedFilter() const
{
    return mFilterList.at(mWidget->mFilterCombo->currentIndex());
}

Ui::ManualSelectFilterPage *ManualSelectFilterPage::widget() const
{
    return mWidget;
}
