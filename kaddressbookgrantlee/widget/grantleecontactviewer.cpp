/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#include "grantleecontactviewer.h"
#include "formatter/grantleecontactformatter.h"
#include <grantleetheme/globalsettings_base.h>
#include <KSharedConfig>
#include <grantleetheme/grantleethememanager.h>

using namespace KAddressBookGrantlee;

GrantleeContactViewer::GrantleeContactViewer(QWidget *parent)
    : Akonadi::ContactViewer(parent)
{
    mFormatter = new KAddressBookGrantlee::GrantleeContactFormatter;
    setContactFormatter(mFormatter);
    mFormatter->setAbsoluteThemePath(kaddressBookAbsoluteThemePath());
}

GrantleeContactViewer::~GrantleeContactViewer()
{
}

QString GrantleeContactViewer::kaddressBookAbsoluteThemePath()
{
    QString themeName = GrantleeTheme::GrantleeSettings::self()->grantleeAddressBookThemeName();
    if (themeName.isEmpty()) {
        themeName = QLatin1String("default");
    }
    const QString absolutePath = GrantleeTheme::GrantleeThemeManager::pathFromThemes(QLatin1String("kaddressbook/viewertemplates/"), themeName, QLatin1String( "theme.desktop" ));
    return absolutePath;
}

void GrantleeContactViewer::setForceDisableQRCode(bool b)
{
    mFormatter->setForceDisableQRCode(b);
}
