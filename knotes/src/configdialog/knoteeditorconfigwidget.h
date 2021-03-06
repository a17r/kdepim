/*
   Copyright (C) 2013-2016 Montel Laurent <montel@kde.org>

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
#ifndef KNOTEEDITORCONFIGWIDGET_H
#define KNOTEEDITORCONFIGWIDGET_H

#include <QWidget>
class QSpinBox;
class QCheckBox;
class KFontRequester;
namespace NoteShared
{
class NoteDisplayAttribute;
}

class KNoteEditorConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KNoteEditorConfigWidget(QWidget *parent = Q_NULLPTR);
    ~KNoteEditorConfigWidget();

    void load(NoteShared::NoteDisplayAttribute *attr, bool isRichText);
    void save(NoteShared::NoteDisplayAttribute *attr, bool &isRichText);

private:
    QSpinBox *kcfg_TabSize;
    QCheckBox *kcfg_AutoIndent;
    QCheckBox *kcfg_RichText;
    KFontRequester *kcfg_Font;
    KFontRequester *kcfg_TitleFont;
};

#endif // KNOTEEDITORCONFIGWIDGET_H
