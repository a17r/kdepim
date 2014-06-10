/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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


#include "followupreminderagent.h"
#include "followupremindermanager.h"
#include "followupreminderadaptor.h"
#include "followupreminderinfodialog.h"
#include "followupreminderagentsettings.h"
#include <KWindowSystem>
#include <KLocale>
#include <KMime/Message>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <akonadi/dbusconnectionpool.h>

#include <QPointer>
#include <QDebug>

FollowUpReminderAgent::FollowUpReminderAgent(const QString &id)
    : Akonadi::AgentBase( id )
{
    KGlobal::locale()->insertCatalog( QLatin1String("akonadi_followupreminder_agent") );
    new FollowUpReminderAgentAdaptor(this);
    Akonadi::DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/FollowUpReminder" ), this, QDBusConnection::ExportAdaptors );
    Akonadi::DBusConnectionPool::threadConnection().registerService( QLatin1String( "org.freedesktop.Akonadi.FollowUpReminder" ) );
    mManager = new FollowUpReminderManager(this);
    if (FollowUpReminderAgentSettings::enabled()) {
        mManager->load();
    }
    changeRecorder()->itemFetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->fetchCollection( true );
    changeRecorder()->setChangeRecordingEnabled( false );
}

FollowUpReminderAgent::~FollowUpReminderAgent()
{
}

void FollowUpReminderAgent::setEnableAgent(bool b)
{
    FollowUpReminderAgentSettings::self()->setEnabled(b);
}

bool FollowUpReminderAgent::enabledAgent() const
{
    return FollowUpReminderAgentSettings::self()->enabled();
}

void FollowUpReminderAgent::showConfigureDialog(qlonglong windowId)
{
    QPointer<FollowUpReminderInfoDialog> dialog = new FollowUpReminderInfoDialog();
    if (windowId) {
#ifndef Q_WS_WIN
        KWindowSystem::setMainWindow( dialog, windowId );
#else
        KWindowSystem::setMainWindow( dialog, (HWND)windowId );
#endif
    }
    if (dialog->exec()) {
        //TODO
    }
    delete dialog;
}

void FollowUpReminderAgent::configure( WId windowId )
{
    showConfigureDialog((qulonglong)windowId);
}

void FollowUpReminderAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
    if ( item.mimeType() != KMime::Message::mimeType() ) {
        kDebug() << "MailFilterAgent::itemAdded called for a non-message item!";
        return;
    }
    mManager->checkFollowUp(item);
}


AKONADI_AGENT_MAIN( FollowUpReminderAgent )

#include "followupreminderagent.moc"