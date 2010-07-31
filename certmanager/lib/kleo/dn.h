/*
    dn.h

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KLEO_DN_H__
#define __KLEO_DN_H__

#include <tqstring.h>
#include <tqvaluevector.h>
#include <kdepimmacros.h>

class QStringList;
class QWidget;

namespace Kleo {
  class DNAttributeOrderConfigWidget;
}

namespace Kleo {

  /**
     @short DN Attribute mapper
  */
  class KDE_EXPORT DNAttributeMapper {
    DNAttributeMapper();
    ~DNAttributeMapper();
  public:
    static const DNAttributeMapper * instance();

    TQString name2label( const TQString & s ) const;
    TQStringList names() const;

    const TQStringList & attributeOrder() const;

    void setAttributeOrder( const TQStringList & order );

    DNAttributeOrderConfigWidget * configWidget( TQWidget * parent=0, const char * name=0 ) const;

  private:
    class Private;
    Private * d;
    static DNAttributeMapper * mSelf;
  };

  /**
     @short DN parser and reorderer
  */
  class KDE_EXPORT DN  {
  public:
    class Attribute;
    typedef TQValueVector<Attribute> AttributeList;
    typedef AttributeList::const_iterator const_iterator;

    DN();
    DN( const TQString & dn );
    DN( const char * utf8DN );
    DN( const DN & other );
    ~DN();

    const DN & operator=( const DN & other );

    /** @return the value in rfc-2253-escaped form */
    static TQString escape( const TQString & value );

    /** @return the DN in a reordered form, according to the settings in
	the [DN] group of the application's config file */
    TQString prettyDN() const;
    /** @return the DN in the original form */
    TQString dn() const;

    TQString operator[]( const TQString & attr ) const;

    void append( const Attribute & attr );

    const_iterator begin() const;
    const_iterator end() const;

  private:
    void detach();
  private:
    class Private;
    Private * d;
  };

  class KDE_EXPORT DN::Attribute {
  public:
    typedef DN::AttributeList List;

    Attribute( const TQString & name=TQString::null, const TQString & value=TQString::null )
      : mName( name.upper() ), mValue( value ) {}
    Attribute( const Attribute & other )
      : mName( other.name() ), mValue( other.value() ) {}

    const Attribute & operator=( const Attribute & other ) {
      if ( this != &other ) {
	mName = other.name();
	mValue = other.value();
      }
      return *this;
    }

    const TQString & name() const { return mName; }
    const TQString & value() const { return mValue; }

    void setValue( const TQString & value ) { mValue = value; }
    
  private:
    TQString mName;
    TQString mValue;
  };

}

#endif // __KLEO_DN_H__
