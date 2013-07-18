/*
  Copyright (c) 2012, 2013 Montel Laurent <montel@kde.org>

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

#include "xmlprintingscriptbuilder.h"
#include <QDebug>

using namespace KSieveUi;
XMLPrintingScriptBuilder::XMLPrintingScriptBuilder()
    : KSieve::ScriptBuilder(),
      mIndent( 0 )
{
    write( "<?xml version='1.0'?>" );
    write( "<script>" );
}

XMLPrintingScriptBuilder::~XMLPrintingScriptBuilder()
{
}

void XMLPrintingScriptBuilder::taggedArgument( const QString & tag )
{
    write( "tag", tag );
}

void XMLPrintingScriptBuilder::stringArgument( const QString & string, bool multiLine, const QString & /*fixme*/ )
{
    write( "string" ,multiLine ? "type=\"multiline\"" : "type=\"quoted\"", string );
}

void XMLPrintingScriptBuilder::numberArgument( unsigned long number, char quantifier )
{
    write( "number", ( quantifier ? QString::fromLatin1("quantifier=\"%1\"").arg( quantifier ) : QString()).toLatin1() , QString::number( number ) );
}

void XMLPrintingScriptBuilder::commandStart( const QString & identifier )
{
    write( "<command>" );
    ++mIndent;
    write( "identifier", identifier );
}

void XMLPrintingScriptBuilder::commandEnd()
{
    --mIndent;
    write( "</command>" );
}

void XMLPrintingScriptBuilder::testStart( const QString & identifier )
{
    write( "<test>" );
    ++mIndent;
    write( "identifier", identifier );
}

void XMLPrintingScriptBuilder::testEnd()
{
    --mIndent;
    write( "</test>" );
}

void XMLPrintingScriptBuilder::testListStart()
{
    write( "<testlist>" );
    ++mIndent;
}

void XMLPrintingScriptBuilder::testListEnd()
{
    --mIndent;
    write( "</testlist>" );
}

void XMLPrintingScriptBuilder::blockStart()
{
    write( "<block>" );
    ++mIndent;
}

void XMLPrintingScriptBuilder::blockEnd()
{
    --mIndent;
    write( "</block>" );
}

void XMLPrintingScriptBuilder::stringListArgumentStart()
{
    write( "<stringlist>" );
    ++mIndent;
}

void XMLPrintingScriptBuilder::stringListArgumentEnd()
{
    --mIndent;
    write( "</stringlist>" );
}

void XMLPrintingScriptBuilder::stringListEntry( const QString & string, bool multiline, const QString & hashComment )
{
    stringArgument( string, multiline, hashComment );
}

void XMLPrintingScriptBuilder::hashComment( const QString & comment )
{
    write( "comment", "type=\"hash\"", comment );
}

void XMLPrintingScriptBuilder::bracketComment( const QString & comment )
{
    write( "comment", "type=\"bracket\"", comment );
}

void XMLPrintingScriptBuilder::lineFeed()
{
    write( "<crlf/>" );
}

void XMLPrintingScriptBuilder::error( const KSieve::Error & error )
{
    mIndent = 0;
    mError = QLatin1String("Error: ") + error.asString();
    write( mError.toLatin1() );
}

void XMLPrintingScriptBuilder::finished()
{
    --mIndent;
    write( "</script>" );
}

void XMLPrintingScriptBuilder::write( const char * msg )
{
    mResult += QString::fromUtf8(msg);
}

void XMLPrintingScriptBuilder::write( const QByteArray & key, const QString & value )
{
    if ( value.isEmpty() ) {
        write( "<" + key + "/>" );
        return;
    }
    write( "<" + key + ">" );
    ++mIndent;
    write( value.toUtf8().data() );
    --mIndent;
    write( "</" + key + ">" );
}

void XMLPrintingScriptBuilder::write( const QByteArray & key, const QByteArray &attribute, const QString & value )
{
    if ( value.isEmpty() ) {
        write( "<" + key + "/>" );
        return;
    }
    write( "<" + key + " " +attribute + ">" );
    ++mIndent;
    write( value.toUtf8().data() );
    --mIndent;
    write( "</" + key + ">" );
}


QString XMLPrintingScriptBuilder::result() const
{
    return mResult;
}

QString XMLPrintingScriptBuilder::error() const
{
    return mError;
}

bool XMLPrintingScriptBuilder::hasError() const
{
    return !mError.isEmpty();
}

void XMLPrintingScriptBuilder::clear()
{
    mResult.clear();
    mError.clear();
    mIndent = 0;
}
