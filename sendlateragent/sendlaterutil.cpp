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


#include "sendlaterutil.h"
#include "sendlaterinfo.h"
#include "sendlateragentsettings.h"

#include <KConfigGroup>

#include <QDBusInterface>
#include <QStringList>

bool SendLater::SendLaterUtil::compareSendLaterInfo(SendLater::SendLaterInfo *left, SendLater::SendLaterInfo *right)
{
    if (left->dateTime() == right->dateTime()) {
        //Set no recursive first.
        if (left->isRecurrence())  {
            return false;
        }
    }
    return left->dateTime() < right->dateTime();
}

KSharedConfig::Ptr SendLater::SendLaterUtil::defaultConfig()
{
    return KSharedConfig::openConfig( QLatin1String("akonadi_sendlater_agentrc") );
}

void SendLater::SendLaterUtil::writeSendLaterInfo(SendLater::SendLaterInfo *info)
{
    if (!info)
        return;

    KSharedConfig::Ptr config = SendLaterUtil::defaultConfig();

    const QString groupName = QString::fromLatin1("SendLaterItem %1").arg(info->itemId());
    // first, delete all filter groups:
    const QStringList filterGroups =config->groupList().filter( groupName );
    foreach ( const QString &group, filterGroups ) {
        config->deleteGroup( group );
    }
    KConfigGroup group = config->group(groupName);
    info->writeConfig(group);
    config->sync();
    config->reparseConfiguration();
    reload();
}

bool SendLater::SendLaterUtil::sentLaterAgentWasRegistered()
{
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_sendlater_agent"), QLatin1String("/SendLaterAgent") );
    return interface.isValid();
}

bool SendLater::SendLaterUtil::sentLaterAgentEnabled()
{
    return SendLaterAgentSettings::self()->enabled();
}

void SendLater::SendLaterUtil::reload()
{
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_sendlater_agent"), QLatin1String("/SendLaterAgent") );
    if (interface.isValid()) {
        interface.call(QLatin1String("reload"));
    }
}

