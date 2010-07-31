/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <kglobalsettings.h>

#include <libkdepim/htmldiffalgodisplay.h>

using namespace KPIM;

static TQString textToHTML( const TQString &text )
{
  return TQStyleSheet::convertFromPlainText( text );
}

HTMLDiffAlgoDisplay::HTMLDiffAlgoDisplay( TQWidget *parent )
  : KTextBrowser( parent )
{
  setWrapPolicy( TQTextEdit::AtWordBoundary );
  setVScrollBarMode( TQScrollView::AlwaysOff );
  setHScrollBarMode( TQScrollView::AlwaysOff );
}

void HTMLDiffAlgoDisplay::begin()
{
  clear();
  mText = "";

  mText.append( "<html>" );
  mText.append( TQString( "<body text=\"%1\" bgcolor=\"%2\">" )
               .arg( KGlobalSettings::textColor().name() )
               .arg( KGlobalSettings::baseColor().name() ) );

  mText.append( "<center><table>" );
  mText.append( TQString( "<tr><th></th><th align=\"center\">%1</th><td>         </td><th align=\"center\">%2</th></tr>" )
               .arg( mLeftTitle )
               .arg( mRightTitle ) );
}

void HTMLDiffAlgoDisplay::end()
{
  mText.append( "</table></center>"
                "</body>"
                "</html>" );

  setText( mText );
}

void HTMLDiffAlgoDisplay::setLeftSourceTitle( const TQString &title )
{
  mLeftTitle = title;
}

void HTMLDiffAlgoDisplay::setRightSourceTitle( const TQString &title )
{
  mRightTitle = title;
}

void HTMLDiffAlgoDisplay::additionalLeftField( const TQString &id, const TQString &value )
{
  mText.append( TQString( "<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#9cff83\">%2</td><td></td><td></td></tr>" )
               .arg( id )
               .arg( textToHTML( value ) ) );
}

void HTMLDiffAlgoDisplay::additionalRightField( const TQString &id, const TQString &value )
{
  mText.append( TQString( "<tr><td align=\"right\"><b>%1:</b></td><td></td><td></td><td bgcolor=\"#9cff83\">%2</td></tr>" )
               .arg( id )
               .arg( textToHTML( value ) ) );
}

void HTMLDiffAlgoDisplay::conflictField( const TQString &id, const TQString &leftValue,
                                          const TQString &rightValue )
{
  mText.append( TQString( "<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#ff8686\">%2</td><td></td><td bgcolor=\"#ff8686\">%3</td></tr>" )
               .arg( id )
               .arg( textToHTML( leftValue ) )
               .arg( textToHTML( rightValue ) ) );
}
