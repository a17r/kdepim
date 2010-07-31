/*
   This file is part of KDE Kontact.

   Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
   Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>

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

#include "summary.h"

#include <tqimage.h>
#include <tqdragobject.h>
#include <tqhbox.h>
#include <tqfont.h>
#include <tqlabel.h>
#include <tqpainter.h>

#include <kiconloader.h>
#include <kdialog.h>

using namespace Kontact;

Summary::Summary( TQWidget *parent, const char *name )
  : TQWidget( parent, name )
{
  setAcceptDrops( true );
}

Summary::~Summary()
{
}

TQWidget* Summary::createHeader(TQWidget *parent, const TQPixmap& icon, const TQString& heading)
{
  TQHBox* hbox = new TQHBox( parent );
  hbox->setMargin( 2 );

  TQFont boldFont;
  boldFont.setBold( true );
  boldFont.setPointSize( boldFont.pointSize() + 2 );

  TQLabel *label = new TQLabel( hbox );
  label->setPixmap( icon );
  label->setFixedSize( label->sizeHint() );
  label->setPaletteBackgroundColor( colorGroup().mid() );
  label->setAcceptDrops( true );

  label = new TQLabel( heading, hbox );
  label->setAlignment( AlignLeft|AlignVCenter );
  label->setIndent( KDialog::spacingHint() );
  label->setFont( boldFont );
  label->setPaletteForegroundColor( colorGroup().light() );
  label->setPaletteBackgroundColor( colorGroup().mid() );

  hbox->setPaletteBackgroundColor( colorGroup().mid() );

  hbox->setMaximumHeight( hbox->minimumSizeHint().height() );

  return hbox;
}

void Summary::mousePressEvent( TQMouseEvent *event )
{
  mDragStartPoint = event->pos();

  TQWidget::mousePressEvent( event );
}

void Summary::mouseMoveEvent( TQMouseEvent *event )
{
  if ( (event->state() & LeftButton) &&
       (event->pos() - mDragStartPoint).manhattanLength() > 4 ) {

    TQDragObject *drag = new TQTextDrag( "", this, "SummaryWidgetDrag" );

    TQPixmap pm = TQPixmap::grabWidget( this );
    if ( pm.width() > 300 )
      pm = pm.convertToImage().smoothScale( 300, 300, TQImage::ScaleMin );

    TQPainter painter;
    painter.begin( &pm );
    painter.setPen( Qt::gray );
    painter.drawRect( 0, 0, pm.width(), pm.height() );
    painter.end();
    drag->setPixmap( pm );
    drag->dragMove();
  } else
    TQWidget::mouseMoveEvent( event );
}

void Summary::dragEnterEvent( TQDragEnterEvent *event )
{
  event->accept( TQTextDrag::canDecode( event ) );
}

void Summary::dropEvent( TQDropEvent *event )
{
  int alignment = (event->pos().y() < (height() / 2) ? Qt::AlignTop : Qt::AlignBottom);
  emit summaryWidgetDropped( this, event->source(), alignment );
}

#include "summary.moc"
