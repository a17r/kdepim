/*
    This file is part of KAddressbook.
    Copyright (c) 2003 - 2003 Daniel Molkentin <molkentin@kde.org>
                              Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <tqfile.h>
#include <tqregexp.h>

#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kurl.h>

#include <kdebug.h>

#include "opera_xxport.h"

K_EXPORT_KADDRESSBOOK_XXFILTER( libkaddrbk_opera_xxport, OperaXXPort )

OperaXXPort::OperaXXPort( KABC::AddressBook *ab, TQWidget *parent, const char *name )
  : KAB::XXPort( ab, parent, name )
{
  createImportAction( i18n( "Import Opera Addressbook..." ) );
}

KABC::AddresseeList OperaXXPort::importContacts( const TQString& ) const
{
  KABC::AddresseeList addrList;

  TQString fileName = KFileDialog::getOpenFileName( TQDir::homeDirPath() + TQString::fromLatin1( "/.opera/contacts.adr" ) );
  if ( fileName.isEmpty() )
    return addrList;

  TQFile file( fileName );
  if ( !file.open( IO_ReadOnly ) ) {
    TQString msg = i18n( "<qt>Unable to open <b>%1</b> for reading.</qt>" );
    KMessageBox::error( parentWidget(), msg.arg( fileName ) );
    return addrList;
  }

  TQTextStream stream( &file );
  stream.setEncoding( TQTextStream::UnicodeUTF8 );
  TQString line, key, value;
  bool parseContact = false;
  KABC::Addressee addr;

  TQRegExp separator( "\x02\x02" );

  while ( !stream.atEnd() ) {
    line = stream.readLine();
    line = line.stripWhiteSpace();
    if ( line == TQString::fromLatin1( "#CONTACT" ) ) {
      parseContact = true;
      addr = KABC::Addressee();
      continue;
    } else if ( line.isEmpty() ) {
      parseContact = false;
      if ( !addr.isEmpty() ) {
        addrList.append( addr );
        addr = KABC::Addressee();
      }
      continue;
    }

    if ( parseContact == true ) {
      int sep = line.find( '=' );
      key = line.left( sep ).lower();
      value = line.mid( sep + 1 );
      if ( key == TQString::fromLatin1( "name" ) )
        addr.setNameFromString( value );
      else if ( key == TQString::fromLatin1( "mail" ) ) {
        TQStringList emails = TQStringList::split( separator, value );

        TQStringList::Iterator it = emails.begin();
        bool preferred = true;
        for ( ; it != emails.end(); ++it ) {
          addr.insertEmail( *it, preferred );
          preferred = false;
        }
      } else if ( key == TQString::fromLatin1( "phone" ) )
        addr.insertPhoneNumber( KABC::PhoneNumber( value ) );
      else if ( key == TQString::fromLatin1( "fax" ) )
        addr.insertPhoneNumber( KABC::PhoneNumber( value,
                              KABC::PhoneNumber::Fax | KABC::PhoneNumber::Home ) );
      else if ( key == TQString::fromLatin1( "postaladdress" ) ) {
        KABC::Address address( KABC::Address::Home );
        address.setLabel( value.replace( separator, "\n" ) );
        addr.insertAddress( address );
      } else if ( key == TQString::fromLatin1( "description" ) )
        addr.setNote( value.replace( separator, "\n" ) );
      else if ( key == TQString::fromLatin1( "url" ) )
        addr.setUrl( KURL( value ) );
      else if ( key == TQString::fromLatin1( "pictureurl" ) ) {
        KABC::Picture pic( value );
        addr.setPhoto( pic );
      }
    }
  }

  file.close();

  return addrList;
}

#include "opera_xxport.moc"
