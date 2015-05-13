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

#include "gravatarconfigwidget.h"

#include <QVBoxLayout>
#include <KLocalizedString>
#include <QCheckBox>
#include "settings/globalsettings.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"

using namespace MessageViewer;
using namespace PimCommon::ConfigureImmutableWidgetUtils;

GravatarConfigWidget::GravatarConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->setMargin(0);

    mEnableGravatarSupport = new QCheckBox(i18n("Enable Gravatar Support"));
    mEnableGravatarSupport->setObjectName(QStringLiteral("gravatarcheckbox"));
    mainLayout->addWidget(mEnableGravatarSupport);

    mUseDefaultPixmap = new QCheckBox(i18n("Use Default Image"));
    mUseDefaultPixmap->setObjectName(QStringLiteral("usedefaultimage"));
    mainLayout->addWidget(mUseDefaultPixmap);

    connect(mUseDefaultPixmap, &QAbstractButton::clicked, this, &GravatarConfigWidget::configChanged);
    connect(mEnableGravatarSupport, &QAbstractButton::clicked, this, &GravatarConfigWidget::configChanged);
}

GravatarConfigWidget::~GravatarConfigWidget()
{

}

void GravatarConfigWidget::save()
{
    saveCheckBox(mEnableGravatarSupport, MessageViewer::GlobalSettings::self()->gravatarSupportEnabledItem());
    saveCheckBox(mUseDefaultPixmap, MessageViewer::GlobalSettings::self()->gravatarUseDefaultImageItem());
}

void GravatarConfigWidget::doLoadFromGlobalSettings()
{
    loadWidget(mEnableGravatarSupport, MessageViewer::GlobalSettings::self()->gravatarSupportEnabledItem());
    loadWidget(mUseDefaultPixmap, MessageViewer::GlobalSettings::self()->gravatarUseDefaultImageItem());
}

void GravatarConfigWidget::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageViewer::GlobalSettings::self()->useDefaults(true);
    doLoadFromGlobalSettings();
    GlobalSettings::self()->useDefaults(bUseDefaults);
}

