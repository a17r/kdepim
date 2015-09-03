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


#include "customtoolsplugin.h"
#include "customtoolspluginmanager.h"
#include <KPluginFactory>
#include <KPluginLoader>
#include <kpluginmetadata.h>
#include <QDebug>
#include <QFileInfo>

using namespace PimCommon;

class CustomToolsPluginInfo
{
public:
    CustomToolsPluginInfo()
        : plugin(Q_NULLPTR)
    {

    }
    QString saveName() const;

    KPluginMetaData metaData;
    PimCommon::CustomToolsPlugin *plugin;
};

QString CustomToolsPluginInfo::saveName() const
{
    return QFileInfo(metaData.fileName()).baseName();
}

class PimCommon::CustomToolsPluginManagerPrivate
{
public:
    CustomToolsPluginManagerPrivate(CustomToolsPluginManager *qq)
        : q(qq)
    {

    }
    void initializePluginList();
    void loadPlugin(CustomToolsPluginInfo *item);
    QVector<CustomToolsPluginInfo> mPluginList;
    CustomToolsPluginManager *q;
};

void CustomToolsPluginManagerPrivate::initializePluginList()
{
    const QVector<KPluginMetaData> plugins = KPluginLoader::findPlugins(QStringLiteral("pimcommon"), [](const KPluginMetaData & md) {
        return md.serviceTypes().contains(QStringLiteral("PimCommonCustomTools/Plugin"));
    });
    qDebug()<<" plugins.count() "<<plugins.count();


    QVectorIterator<KPluginMetaData> i(plugins);
    i.toBack();
    QSet<QString> unique;
    while (i.hasPrevious()) {
        CustomToolsPluginInfo info;
        info.metaData = i.previous();

        // only load plugins once, even if found multiple times!
        if (unique.contains(info.saveName()))
            continue;
        info.plugin = Q_NULLPTR;
        mPluginList.push_back(info);
        unique.insert(info.saveName());
    }
    qDebug()<<" mPluginList.count() "<<mPluginList.count();
    QVector<CustomToolsPluginInfo>::iterator end(mPluginList.end());
    for (QVector<CustomToolsPluginInfo>::iterator it = mPluginList.begin(); it != end; ++it) {
        loadPlugin(&(*it));
    }
}

void CustomToolsPluginManagerPrivate::loadPlugin(CustomToolsPluginInfo *item)
{
    item->plugin = KPluginLoader(item->metaData.fileName()).factory()->create<PimCommon::CustomToolsPlugin>(q, QVariantList() << item->saveName());
    qDebug()<<" item->plugin"<<item->plugin;
}


CustomToolsPluginManager::CustomToolsPluginManager(QObject *parent)
    : d(new PimCommon::CustomToolsPluginManagerPrivate(this))
{
    d->initializePluginList();
}

CustomToolsPluginManager::~CustomToolsPluginManager()
{
    delete d;
}



