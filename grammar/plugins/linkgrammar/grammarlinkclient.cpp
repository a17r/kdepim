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


#include "grammarlinkclient.h"
#include "grammarlinkplugin.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kdebug.h>

K_PLUGIN_FACTORY( GrammarLinkClientFactory, registerPlugin<GrammarLinkClient>(); )
K_EXPORT_PLUGIN( GrammarLinkClientFactory( "grammar_link" ) )

GrammarLinkClient::GrammarLinkClient(QObject *parent, const QVariantList& /* args */)
    : Grammar::GrammarClient(parent)
{
}

GrammarLinkClient::~GrammarLinkClient()
{

}

Grammar::GrammarPlugin *GrammarLinkClient::createGrammarChecker(const QString &language)
{
    GrammarLinkPlugin *plugin = new GrammarLinkPlugin(language);
    return plugin;
}

QStringList GrammarLinkClient::languages() const
{
    //TODO
    return QStringList();
}

QString GrammarLinkClient::name() const
{
    return QLatin1String("grammarlink");
}


#include "grammarlinkclient.moc"
