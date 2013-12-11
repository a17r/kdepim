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

#include "notesagent.h"
#include "notesmanager.h"
#include "notesagentadaptor.h"
#include "notesagentsettings.h"
#include "notesagentsettingsdialog.h"

#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <akonadi/dbusconnectionpool.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>
#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionFetchScope>
#include <KMime/Message>

#include <KWindowSystem>
#include <KLocale>

#include <QPointer>

//#define DEBUG_NOTESAGENT 1

NotesAgent::NotesAgent(const QString &id)
    : Akonadi::AgentBase( id )
{
    mNotesManager = new NotesManager(this);
    KGlobal::locale()->insertCatalog( QLatin1String("akonadi_notes_agent") );
    new NotesAgentAdaptor( this );
    Akonadi::DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/NotesAgent" ), this, QDBusConnection::ExportAdaptors );
    Akonadi::DBusConnectionPool::threadConnection().registerService( QLatin1String( "org.freedesktop.Akonadi.NotesAgent" ) );

    changeRecorder()->setMimeTypeMonitored( KMime::Message::mimeType() );
    changeRecorder()->itemFetchScope().setCacheOnly( true );
    changeRecorder()->itemFetchScope().setFetchModificationTime( false );
    changeRecorder()->setChangeRecordingEnabled( false );
    changeRecorder()->ignoreSession( Akonadi::Session::defaultSession() );
    setNeedsNetwork(true);

    if (NotesAgentSettings::enabled()) {
#ifdef DEBUG_NOTESAGENT
        QTimer::singleShot(1000, mNotesManager, SLOT(load()));
#else
        QTimer::singleShot(1000*60*4, mNotesManager, SLOT(load()));
#endif
    }
}

NotesAgent::~NotesAgent()
{
}

void NotesAgent::doSetOnline( bool online )
{
    if (online) {
        reload();
    } else {
        mNotesManager->stopAll();
    }
}

void NotesAgent::reload()
{
    if (NotesAgentSettings::enabled())
        mNotesManager->load(true);
}

void NotesAgent::setEnableAgent(bool enabled)
{
    if (NotesAgentSettings::enabled() == enabled)
        return;

    NotesAgentSettings::setEnabled(enabled);
    NotesAgentSettings::self()->writeConfig();
    if (enabled) {
        mNotesManager->load();
    } else {
        mNotesManager->stopAll();
    }
}

bool NotesAgent::enabledAgent() const
{
    return NotesAgentSettings::enabled();
}

void NotesAgent::configure( WId windowId )
{
    showConfigureDialog(windowId);
}

void NotesAgent::showConfigureDialog(qlonglong windowId)
{
    QPointer<NotesAgentSettingsDialog> dialog = new NotesAgentSettingsDialog;
    if (windowId) {
#ifndef Q_WS_WIN
        KWindowSystem::setMainWindow( dialog, windowId );
#else
        KWindowSystem::setMainWindow( dialog, (HWND)windowId );
#endif
    }
    dialog->exec();
    delete dialog;
}

void NotesAgent::printDebugInfo()
{
    mNotesManager->printDebugInfo();
}

AKONADI_AGENT_MAIN( NotesAgent )

