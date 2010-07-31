/*
 *   This file is part of ScalixAdmin.
 *
 *   Copyright (C) 2007 Trolltech ASA. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <klocale.h>

#include "otherusermanager.h"

#include "otheruserview.h"

class OtherUserItem : public QListViewItem
{
  public:
    OtherUserItem( TQListView *parent, const TQString &user )
      : TQListViewItem( parent ), mUser( user )
    {
      setText( 0, mUser );
    }

    TQString user() const { return mUser; }

  private:
    TQString mUser;
};

OtherUserView::OtherUserView( OtherUserManager *manager, TQWidget *parent )
  : KListView( parent ), mManager( manager )
{
  addColumn( i18n( "Registered Accounts" ) );
  setFullWidth( true );

  connect( mManager, TQT_SIGNAL( changed() ), TQT_SLOT( userChanged() ) );

  userChanged();
}

TQString OtherUserView::selectedUser() const
{
  OtherUserItem *item = dynamic_cast<OtherUserItem*>( selectedItem() );
  if ( item )
    return item->user();

  return TQString();
}

void OtherUserView::userChanged()
{
  clear();

  TQStringList users = mManager->otherUsers();
  for ( uint i = 0; i < users.count(); ++i )
    new OtherUserItem( this, users[ i ] );
}

#include "otheruserview.moc"
