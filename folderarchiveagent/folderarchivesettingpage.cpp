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

#include "folderarchivesettingpage.h"
#include "mailcommon/folder/folderrequester.h"

#include <KLocale>
#include <KGlobal>
#include <KSharedConfig>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

FolderArchiveSettingPage::FolderArchiveSettingPage(const QString &instanceName, QWidget *parent)
    : QWidget(parent),
      mInstanceName(instanceName)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mEnabled = new QCheckBox(i18n("Enable"));
    lay->addWidget(mEnabled);

    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *lab = new QLabel(i18n("Archive folder:"));
    hbox->addWidget(lab);
    mArchiveFolder = new MailCommon::FolderRequester;
    hbox->addWidget(mArchiveFolder);
    lay->addLayout(hbox);

    setLayout(lay);
}

FolderArchiveSettingPage::~FolderArchiveSettingPage()
{

}

void FolderArchiveSettingPage::loadSettings()
{

    //TODO
}

void FolderArchiveSettingPage::writeSettings()
{
    //TODO
}

#include "folderarchivesettingpage.moc"
