/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "mainview.h"
#include "contacteditorview.h"
#include "contactgroupeditorview.h"
#include "contactlistproxy.h"

#include <QtDeclarative/QDeclarativeEngine>

#include <akonadi/agentactionmanager.h>
#include <akonadi/contact/standardcontactactionmanager.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <Akonadi/ItemFetchScope>

MainView::MainView( QWidget *parent ) : KDeclarativeMainView( "kaddressbook-mobile", new ContactListProxy, parent )
{
}

void MainView::delayedInit()
{
  KDeclarativeMainView::delayedInit();

  addMimeType( KABC::Addressee::mimeType() );
  addMimeType( KABC::ContactGroup::mimeType() );
  itemFetchScope().fetchFullPayload();
}

void MainView::newContact()
{
  ContactEditorView *editor = new ContactEditorView;
  connect( editor, SIGNAL( requestLaunchAccountWizard() ), SLOT( launchAccountWizard() ) );
  editor->show();
}

void MainView::newContactGroup()
{
  ContactGroupEditorView *editor = new ContactGroupEditorView;
  connect( editor, SIGNAL( requestLaunchAccountWizard() ), SLOT( launchAccountWizard() ) );
  editor->show();
}

void MainView::editItem()
{
  const Akonadi::Item::List items = mActionManager->selectedItems();
  if ( items.isEmpty() )
    return;

  const Akonadi::Item item = items.first();

  if ( item.hasPayload<KABC::Addressee>() )
    editContact( item );
  else if ( item.hasPayload<KABC::ContactGroup>() )
    editContactGroup( item );
}

void MainView::editContact( const Akonadi::Item &item )
{
  ContactEditorView *editor = new ContactEditorView;
  connect( editor, SIGNAL( requestLaunchAccountWizard() ), SLOT( launchAccountWizard() ) );
  editor->loadContact( item );
  editor->show();
}

void MainView::editContactGroup( const Akonadi::Item &item )
{
  ContactGroupEditorView *editor = new ContactGroupEditorView;
  connect( editor, SIGNAL( requestLaunchAccountWizard() ), SLOT( launchAccountWizard() ) );
  editor->loadContactGroup( item );
  editor->show();
}

void MainView::setupStandardActionManager( QItemSelectionModel *collectionSelectionModel,
                                           QItemSelectionModel *itemSelectionModel )
{
  mActionManager = new Akonadi::StandardContactActionManager( actionCollection(), this );
  mActionManager->setCollectionSelectionModel( collectionSelectionModel );
  mActionManager->setItemSelectionModel( itemSelectionModel );

  mActionManager->createAllActions();
  mActionManager->interceptAction( Akonadi::StandardContactActionManager::CreateContact );
  mActionManager->interceptAction( Akonadi::StandardContactActionManager::CreateContactGroup );
  mActionManager->interceptAction( Akonadi::StandardContactActionManager::EditItem );
  mActionManager->interceptAction( Akonadi::StandardActionManager::CreateResource );

  connect( mActionManager->action( Akonadi::StandardContactActionManager::CreateContact ), SIGNAL( triggered( bool ) ),
           this, SLOT( newContact() ) );
  connect( mActionManager->action( Akonadi::StandardContactActionManager::CreateContactGroup ), SIGNAL( triggered( bool ) ),
           this, SLOT( newContactGroup() ) );
  connect( mActionManager->action( Akonadi::StandardContactActionManager::EditItem ), SIGNAL( triggered( bool ) ),
           this, SLOT( editItem() ) );
  connect( mActionManager->action( Akonadi::StandardActionManager::CreateResource ), SIGNAL( triggered( bool ) ),
           this, SLOT( launchAccountWizard() ) );
}

void MainView::setupAgentActionManager( QItemSelectionModel *selectionModel )
{
  Akonadi::AgentActionManager *manager = new Akonadi::AgentActionManager( actionCollection(), this );
  manager->setSelectionModel( selectionModel );
  manager->createAllActions();

  manager->action( Akonadi::AgentActionManager::CreateAgentInstance )->setText( i18n( "Add Address Book" ) );
  manager->action( Akonadi::AgentActionManager::DeleteAgentInstance )->setText( i18n( "Delete Address Book" ) );
  manager->action( Akonadi::AgentActionManager::ConfigureAgentInstance )->setText( i18n( "Configure Address Book" ) );

  manager->interceptAction( Akonadi::AgentActionManager::CreateAgentInstance );

  connect( manager->action( Akonadi::AgentActionManager::CreateAgentInstance ), SIGNAL( triggered( bool ) ),
           this, SLOT( launchAccountWizard() ) );

  manager->setContextText( Akonadi::AgentActionManager::CreateAgentInstance, Akonadi::AgentActionManager::DialogTitle,
                           i18nc( "@title:window", "New Address Book" ) );
  manager->setContextText( Akonadi::AgentActionManager::CreateAgentInstance, Akonadi::AgentActionManager::ErrorMessageText,
                           i18n( "Could not create address book: %1" ) );
  manager->setContextText( Akonadi::AgentActionManager::CreateAgentInstance, Akonadi::AgentActionManager::ErrorMessageTitle,
                           i18n( "Address Book creation failed" ) );

  manager->setContextText( Akonadi::AgentActionManager::DeleteAgentInstance, Akonadi::AgentActionManager::MessageBoxTitle,
                           i18nc( "@title:window", "Delete Address Book?" ) );
  manager->setContextText( Akonadi::AgentActionManager::DeleteAgentInstance, Akonadi::AgentActionManager::MessageBoxText,
                           i18n( "Do you really want to delete the selected address book?" ) );
}

#include "mainview.moc"
