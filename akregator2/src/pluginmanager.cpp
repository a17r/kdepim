/***************************************************************************
begin                : 2004/03/12
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "pluginmanager.h"
#include "plugin.h"

#include <vector>
#include <QFile>
#include <QString>

#include <klibloader.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

using std::vector;
using Akregator2::Plugin;

namespace Akregator2 {

vector<PluginManager::StoreItem>
PluginManager::m_store;


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC INTERFACE
/////////////////////////////////////////////////////////////////////////////////////

KService::List
PluginManager::query( const QString& constraint )
{
    // Add versioning constraint
    QString
    str  = "[X-KDE-akregator2-framework-version] == ";
    str += QString::number( AKREGATOR2_PLUGIN_INTERFACE_VERSION );
    str += " and ";
    if (!constraint.trimmed().isEmpty())
        str += constraint + " and ";
    str += "[X-KDE-akregator2-rank] > 0";

    kDebug() <<"Plugin trader constraint:" << str;

    return KServiceTypeTrader::self()->query( "Akregator2/Plugin", str );
}


Plugin*
PluginManager::createFromQuery( const QString &constraint )
{
    KService::List offers = query( constraint );

    if ( offers.isEmpty() ) {
        kWarning() <<"No matching plugin found.";
        return 0;
    }

    // Select plugin with highest rank
    int rank = 0;
    uint current = 0;
    for ( int i = 0; i < offers.count(); i++ ) {
        if ( offers[i]->property( "X-KDE-akregator2-rank" ).toInt() > rank )
            current = i;
    }

    return createFromService( offers[current] );
}


Plugin*
PluginManager::createFromService(const KService::Ptr service , QObject *parent)
{
    kDebug() <<"Trying to load:" << service->library();

    KPluginLoader loader( *service );
    KPluginFactory* factory = loader.factory();
    if ( !factory ) {
        kWarning() << QString( " Could not create plugin factory for: %1\n"
                                " Error message: %2" ).arg( service->library(), loader.errorString() );
        return 0;
    }
    Plugin* const plugin = factory->create<Plugin>( parent );

    //put plugin into store
    StoreItem item;
    item.plugin = plugin;
    item.service = service;
    m_store.push_back( item );

    dump( service );
    return plugin;
}


void
PluginManager::unload( Plugin* plugin )
{
#ifdef TEMPORARILY_REMOVED
    vector<StoreItem>::iterator iter = lookupPlugin( plugin );

    if ( iter != m_store.end() ) {
        delete (*iter).plugin;
        kDebug() <<"Unloading library:"<< (*iter).service->library();
        //PENDING(kdab,frank) Review
        (*iter).library->unload();


        m_store.erase( iter );
    }
    else
        kWarning() <<"Could not unload plugin (not found in store).";
#else //TEMPORARILY_REMOVED
    Q_UNUSED( plugin )
    kWarning() <<"PluginManager::unload temporarily disabled";
#endif //TEMPORARILY_REMOVED

}


KService::Ptr
PluginManager::getService( const Plugin* plugin )
{
    if ( !plugin ) {
        kWarning() <<"pointer == NULL";
        return KService::Ptr( 0 );
    }

    //search plugin in store
    vector<StoreItem>::const_iterator iter = lookupPlugin( plugin );

    if ( iter == m_store.end() ) {
        kWarning() <<"Plugin not found in store.";
        return KService::Ptr( 0 );
    }

    return (*iter).service;
}


void
PluginManager::showAbout( const QString &constraint )
{
    KService::List offers = query( constraint );

    if ( offers.isEmpty() )
        return;

    KService::Ptr s = offers.front();

    const QString body = "<tr><td>%1</td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"1\">";

    str += body.arg( i18nc( "Name of the plugin", "Name" ),                             s->name() );
    str += body.arg( i18nc( "Library name", "Library" ),                                s->library() );
    str += body.arg( i18nc( "Plugin authors", "Authors" ),                              s->property( "X-KDE-akregator2-authors" ).toStringList().join( "\n" ) );
    str += body.arg( i18nc( "Plugin authors' emaila addresses", "Email" ),              s->property( "X-KDE-akregator2-email" ).toStringList().join( "\n" ) );
    str += body.arg( i18nc( "Plugin version", "Version" ),                              s->property( "X-KDE-akregator2-version" ).toString() );
    str += body.arg( i18nc( "Framework version plugin requires", "Framework Version" ), s->property( "X-KDE-akregator2-framework-version" ).toString() );

    str += "</table></body></html>";

    KMessageBox::information( 0, str, i18n( "Plugin Information" ) );
}


void
PluginManager::dump( const KService::Ptr service )
{
    kDebug()
      << "PluginManager Service Info:" << endl
      << "---------------------------" << endl
      << "name                          : " << service->name() << endl
      << "library                       : " << service->library() << endl
      << "desktopEntryPath              : " << service->entryPath() << endl
      << "X-KDE-akregator2-plugintype       : " << service->property( "X-KDE-akregator2-plugintype" ).toString() << endl
      << "X-KDE-akregator2-name             : " << service->property( "X-KDE-akregator2-name" ).toString() << endl
      << "X-KDE-akregator2-authors          : " << service->property( "X-KDE-akregator2-authors" ).toStringList() << endl
      << "X-KDE-akregator2-rank             : " << service->property( "X-KDE-akregator2-rank" ).toString() << endl
      << "X-KDE-akregator2-version          : " << service->property( "X-KDE-akregator2-version" ).toString() << endl
      << "X-KDE-akregator2-framework-version: " << service->property( "X-KDE-akregator2-framework-version" ).toString()
      << endl;

}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE INTERFACE
/////////////////////////////////////////////////////////////////////////////////////

vector<PluginManager::StoreItem>::iterator
PluginManager::lookupPlugin( const Plugin* plugin )
{
    vector<StoreItem>::iterator iter;

    //search plugin pointer in store
    vector<StoreItem>::const_iterator end;
    for ( iter = m_store.begin(); iter != end; ++iter ) {
        if ( (*iter).plugin == plugin )
            break;
    }

    return iter;
}

} // namespace Akregator2
