/*
   Copyright (C) 2015-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "selectprogramlistwidget.h"

#include <KLocalizedString>
#include <QPainter>

SelectProgramListWidget::SelectProgramListWidget(QWidget *parent)
    : QListWidget(parent),
      mNoProgramFound(false)
{

}

SelectProgramListWidget::~SelectProgramListWidget()
{

}
void SelectProgramListWidget::setNoProgramFound(bool noProgramFound)
{
    mNoProgramFound = noProgramFound;
}

void SelectProgramListWidget::generalPaletteChanged()
{
    const QPalette palette = viewport()->palette();
    QColor color = palette.text().color();
    color.setAlpha(128);
    mTextColor = color;
}

void SelectProgramListWidget::paintEvent(QPaintEvent *event)
{
    if (mNoProgramFound  && (!model() || model()->rowCount() == 0)) {
        QPainter p(viewport());

        QFont font = p.font();
        font.setItalic(true);
        p.setFont(font);

        if (!mTextColor.isValid()) {
            generalPaletteChanged();
        }
        p.setPen(mTextColor);

        p.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, i18n("No program found."));
    } else {
        QListWidget::paintEvent(event);
    }
}

