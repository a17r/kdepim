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

#include "invalidfilterdialog.h"
#include "invalidfilterwidget.h"
#include "invalidfilterinfowidget.h"
#include <KConfigGroup>
#include <KSharedConfig>
#include <QVBoxLayout>
#include <KLocalizedString>

using namespace MailCommon;

InvalidFilterDialog::InvalidFilterDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18n("Invalid Filters"));
    setWindowIcon(QIcon::fromTheme(QLatin1String("kmail")));
    setButtons(Cancel | Ok);
    setDefaultButton(Ok);
    setModal(true);

    QWidget *w = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    w->setLayout(vbox);
    mInvalidFilterWidget = new InvalidFilterWidget(this);
    mInvalidFilterWidget->setObjectName(QLatin1String("invalid_filter_widget"));
    vbox->addWidget(mInvalidFilterWidget);

    mInvalidFilterInfoWidget = new InvalidFilterInfoWidget(this);
    mInvalidFilterInfoWidget->setObjectName(QLatin1String("invalid_filter_infowidget"));
    vbox->addWidget(mInvalidFilterInfoWidget);
    connect(mInvalidFilterWidget, SIGNAL(showDetails(QString)), mInvalidFilterInfoWidget, SLOT(slotShowDetails(QString)));
    connect(mInvalidFilterWidget, SIGNAL(hideInformationWidget()), mInvalidFilterInfoWidget, SLOT(animatedHide()));
    setMainWidget(w);
    readConfig();
}

InvalidFilterDialog::~InvalidFilterDialog()
{
    writeConfig();
}

void InvalidFilterDialog::setInvalidFilters(const QVector<InvalidFilterInfo> &lst)
{
    mInvalidFilterWidget->setInvalidFilters(lst);
}

void InvalidFilterDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "InvalidFilterDialog");
    group.writeEntry("Size", size());
}

void InvalidFilterDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "InvalidFilterDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(300, 350));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
}
