/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "plaintexteditorwidget.h"
#include "plaintexteditor.h"
#include "plaintexteditfindbar.h"
#include "pimcommon/texttospeech/texttospeechwidget.h"

#include <QVBoxLayout>
#include <QShortcut>
#include <QTextCursor>

using namespace PimCommon;

PlainTextEditorWidget::PlainTextEditorWidget(PlainTextEditor *customEditor, QWidget *parent)
    : QWidget(parent)
{
    init(customEditor);
}

PlainTextEditorWidget::PlainTextEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

PlainTextEditorWidget::~PlainTextEditorWidget()
{

}

PlainTextEditor *PlainTextEditorWidget::editor() const
{
    return mEditor;
}

void PlainTextEditorWidget::setPlainText(const QString &text)
{
    mEditor->setPlainText(text);
}

QString PlainTextEditorWidget::toPlainText() const
{
    return mEditor->toPlainText();
}

void PlainTextEditorWidget::init(PlainTextEditor *customEditor)
{
    QVBoxLayout *lay = new QVBoxLayout;
    lay->setMargin(0);
    mTextToSpeechWidget = new PimCommon::TextToSpeechWidget(this);
    lay->addWidget(mTextToSpeechWidget);
    if (customEditor) {
        mEditor = customEditor;
    } else {
        mEditor = new PlainTextEditor;
    }
    lay->addWidget(mEditor);
    connect(mEditor, &PlainTextEditor::say, mTextToSpeechWidget, &PimCommon::TextToSpeechWidget::say);

    mFindBar = new PimCommon::PlainTextEditFindBar( mEditor, this );
    lay->addWidget(mFindBar);

    QShortcut *shortcut = new QShortcut( this );
    shortcut->setKey( Qt::Key_F+Qt::CTRL );
    connect(shortcut, &QShortcut::activated, this, &PlainTextEditorWidget::slotFind);
    connect(mEditor, &PlainTextEditor::findText, this, &PlainTextEditorWidget::slotFind);

    shortcut = new QShortcut( this );
    shortcut->setKey( Qt::Key_R+Qt::CTRL );
    connect(shortcut, &QShortcut::activated, this, &PlainTextEditorWidget::slotReplace);
    connect(mEditor, &PlainTextEditor::replaceText, this, &PlainTextEditorWidget::slotReplace);

    setLayout(lay);
}

bool PlainTextEditorWidget::isReadOnly() const
{
    return mEditor->isReadOnly();
}

void PlainTextEditorWidget::setReadOnly(bool readOnly)
{
    mEditor->setReadOnly(readOnly);
}

void PlainTextEditorWidget::slotReplace()
{
    if ( mEditor->searchSupport() ) {
        mFindBar->showReplace();
        mFindBar->focusAndSetCursor();
    }
}

void PlainTextEditorWidget::slotFind()
{
    if ( mEditor->searchSupport() ) {
        if ( mEditor->textCursor().hasSelection() )
            mFindBar->setText( mEditor->textCursor().selectedText() );
        mEditor->moveCursor(QTextCursor::Start);
        mFindBar->showFind();
        mFindBar->focusAndSetCursor();
    }
}


