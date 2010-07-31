/*
 *  combobox.cpp  -  combo box with read-only option
 *  Program:  kalarm
 *  Copyright (c) 2002 by David Jarvie <software@astrojar.org.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <tqlineedit.h>
#include "combobox.moc"


ComboBox::ComboBox(TQWidget* parent, const char* name)
	: TQComboBox(parent, name),
	  mReadOnly(false)
{ }

ComboBox::ComboBox(bool rw, TQWidget* parent, const char* name)
	: TQComboBox(rw, parent, name),
	  mReadOnly(false)
{ }

void ComboBox::setReadOnly(bool ro)
{
	if ((int)ro != (int)mReadOnly)
	{
		mReadOnly = ro;
		if (lineEdit())
			lineEdit()->setReadOnly(ro);
	}
}

void ComboBox::mousePressEvent(TQMouseEvent* e)
{
	if (mReadOnly)
	{
		// Swallow up the event if it's the left button
		if (e->button() == Qt::LeftButton)
			return;
	}
	TQComboBox::mousePressEvent(e);
}

void ComboBox::mouseReleaseEvent(TQMouseEvent* e)
{
	if (!mReadOnly)
		TQComboBox::mouseReleaseEvent(e);
}

void ComboBox::mouseMoveEvent(TQMouseEvent* e)
{
	if (!mReadOnly)
		TQComboBox::mouseMoveEvent(e);
}

void ComboBox::keyPressEvent(TQKeyEvent* e)
{
	if (!mReadOnly  ||  e->key() == Qt::Key_Escape)
		TQComboBox::keyPressEvent(e);
}

void ComboBox::keyReleaseEvent(TQKeyEvent* e)
{
	if (!mReadOnly)
		TQComboBox::keyReleaseEvent(e);
}
