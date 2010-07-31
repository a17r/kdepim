/** -*- c++ -*-
 * overlaywidget.h
 *
 *  Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "overlaywidget.h"
using namespace KPIM;

OverlayWidget::OverlayWidget( TQWidget* alignWidget, TQWidget* parent, const char* name )
  : TQHBox( parent, name ), mAlignWidget( 0 )
{
  setAlignWidget( alignWidget );
}

OverlayWidget::~OverlayWidget()
{
}

void OverlayWidget::reposition()
{
  if ( !mAlignWidget )
    return;
  // p is in the alignWidget's coordinates
  TQPoint p;
  // We are always above the alignWidget, right-aligned with it.
  p.setX( mAlignWidget->width() - width() );
  p.setY( -height() );
  // Position in the toplevelwidget's coordinates
  TQPoint pTopLevel = mAlignWidget->mapTo( topLevelWidget(), p );
  // Position in the widget's parentWidget coordinates
  TQPoint pParent = parentWidget()->mapFrom( topLevelWidget(), pTopLevel );
  // Move 'this' to that position.
  move( pParent );
}

void OverlayWidget::setAlignWidget( TQWidget * w )
{
  if (w == mAlignWidget)
    return;

  if (mAlignWidget)
    mAlignWidget->removeEventFilter(this);

  mAlignWidget = w;

  if (mAlignWidget)
    mAlignWidget->installEventFilter(this);

  reposition();
}

bool OverlayWidget::eventFilter( TQObject* o, TQEvent* e)
{
  if ( o == mAlignWidget &&
       ( e->type() == TQEvent::Move || e->type() == TQEvent::Resize ) ) {
    reposition();
  }
  return TQFrame::eventFilter(o,e);
}

void OverlayWidget::resizeEvent( TQResizeEvent* ev )
{
  reposition();
  TQFrame::resizeEvent( ev );
}

#include "overlaywidget.moc"
