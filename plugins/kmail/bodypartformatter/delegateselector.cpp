/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "delegateselector.h"

#include <libkdepim/addresseelineedit.h>

#include <klocale.h>

#include <tqcheckbox.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqvbox.h>

DelegateSelector::DelegateSelector(TQWidget * parent)
  : KDialogBase( parent, 0, true, i18n("Select delegate"), Ok|Cancel, Ok, true )
{
  TQVBox *page = makeVBoxMainWidget();

  TQHBox *delegateBox = new TQHBox( page );
  new TQLabel( i18n("Delegate:"), delegateBox );
  mDelegate = new KPIM::AddresseeLineEdit( delegateBox );

  mRsvp = new TQCheckBox( i18n("Keep me informed about status changes of this incidence."), page );
  mRsvp->setChecked( true );
}

TQString DelegateSelector::delegate() const
{
  return mDelegate->text();
}

bool DelegateSelector::rsvp() const
{
  return mRsvp->isChecked();
}

#include "delegateselector.moc"
