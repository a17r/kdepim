/*
 *  radiobutton.cpp  -  radio button with read-only option
 *  Program:  kalarm
 *  Copyright (c) 2002, 2003 by David Jarvie <software@astrojar.org.uk>
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

#include "radiobutton.moc"


RadioButton::RadioButton(TQWidget* parent, const char* name)
	: TQRadioButton(parent, name),
	  mFocusPolicy(focusPolicy()),
	  mFocusWidget(0),
	  mReadOnly(false)
{ }

RadioButton::RadioButton(const TQString& text, TQWidget* parent, const char* name)
	: TQRadioButton(text, parent, name),
	  mFocusPolicy(focusPolicy()),
	  mFocusWidget(0),
	  mReadOnly(false)
{ }

/******************************************************************************
*  Set the read-only status. If read-only, the button can be toggled by the
*  application, but not by the user.
*/
void RadioButton::setReadOnly(bool ro)
{
	if ((int)ro != (int)mReadOnly)
	{
		mReadOnly = ro;
		setFocusPolicy(ro ? TQWidget::NoFocus : mFocusPolicy);
		if (ro)
			clearFocus();
	}
}

/******************************************************************************
*  Specify a widget to receive focus when the button is clicked on.
*/
void RadioButton::setFocusWidget(TQWidget* w, bool enable)
{
	mFocusWidget = w;
	mFocusWidgetEnable = enable;
	if (w)
		connect(this, TQT_SIGNAL(clicked()), TQT_SLOT(slotClicked()));
	else
		disconnect(this, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotClicked()));
}

/******************************************************************************
*  Called when the button is clicked.
*  If it is now checked, focus is transferred to any specified focus widget.
*/
void RadioButton::slotClicked()
{
	if (mFocusWidget  &&  isChecked())
	{
		if (mFocusWidgetEnable)
			mFocusWidget->setEnabled(true);
		mFocusWidget->setFocus();
	}
}

/******************************************************************************
*  Event handlers to intercept events if in read-only mode.
*  Any events which could change the button state are discarded.
*/
void RadioButton::mousePressEvent(TQMouseEvent* e)
{
	if (mReadOnly)
	{
		// Swallow up the event if it's the left button
		if (e->button() == Qt::LeftButton)
			return;
	}
	TQRadioButton::mousePressEvent(e);
}

void RadioButton::mouseReleaseEvent(TQMouseEvent* e)
{
	if (mReadOnly)
	{
		// Swallow up the event if it's the left button
		if (e->button() == Qt::LeftButton)
			return;
	}
	TQRadioButton::mouseReleaseEvent(e);
}

void RadioButton::mouseMoveEvent(TQMouseEvent* e)
{
	if (!mReadOnly)
		TQRadioButton::mouseMoveEvent(e);
}

void RadioButton::keyPressEvent(TQKeyEvent* e)
{
	if (mReadOnly)
		switch (e->key())
		{
			case Qt::Key_Up:
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Down:
				// Process keys which shift the focus
			case Qt::Key_Escape:
				break;
			default:
				return;
		}
	TQRadioButton::keyPressEvent(e);
}

void RadioButton::keyReleaseEvent(TQKeyEvent* e)
{
	if (!mReadOnly)
		TQRadioButton::keyReleaseEvent(e);
}
