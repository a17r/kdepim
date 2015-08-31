/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#include "richtexteditorwidget.h"
#include "richtexteditor.h"
#include "richtexteditfindbar.h"

#include <QVBoxLayout>
#include <QShortcut>
#include <QTextCursor>
#include <pimcommon/texttospeech/texttospeechwidget.h>

#include "pimcommon/widgets/slidecontainer.h"

using namespace PimCommon;

RichTextEditorWidget::RichTextEditorWidget(RichTextEditor *customEditor, QWidget *parent)
    : QWidget(parent)
{
    init(customEditor);
}

RichTextEditorWidget::RichTextEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

RichTextEditorWidget::~RichTextEditorWidget()
{

}

void RichTextEditorWidget::clear()
{
    mEditor->clear();
}

RichTextEditor *RichTextEditorWidget::editor() const
{
    return mEditor;
}

void RichTextEditorWidget::setAcceptRichText(bool b)
{
    mEditor->setAcceptRichText(b);
}

bool RichTextEditorWidget::acceptRichText() const
{
    return mEditor->acceptRichText();
}

void RichTextEditorWidget::setSpellCheckingConfigFileName(const QString &_fileName)
{
    mEditor->setSpellCheckingConfigFileName(_fileName);
}

void RichTextEditorWidget::setHtml(const QString &html)
{
    mEditor->setHtml(html);
}

QString RichTextEditorWidget::toHtml() const
{
    return mEditor->toHtml();
}

void RichTextEditorWidget::setPlainText(const QString &text)
{
    mEditor->setPlainText(text);
}

QString RichTextEditorWidget::toPlainText() const
{
    return mEditor->toPlainText();
}

void RichTextEditorWidget::init(RichTextEditor *customEditor)
{
    QVBoxLayout *lay = new QVBoxLayout;
    lay->setMargin(0);
    mTextToSpeechWidget = new PimCommon::TextToSpeechWidget(this);
    lay->addWidget(mTextToSpeechWidget);
    if (customEditor) {
        mEditor = customEditor;
    } else {
        mEditor = new RichTextEditor;
    }
    connect(mEditor, &RichTextEditor::say, mTextToSpeechWidget, &PimCommon::TextToSpeechWidget::say);
    lay->addWidget(mEditor);

    mSliderContainer = new PimCommon::SlideContainer(this);

    mFindBar = new PimCommon::RichTextEditFindBar(mEditor, this);
    mFindBar->setHideWhenClose(false);
    connect(mFindBar, &PimCommon::RichTextEditFindBar::displayMessageIndicator, mEditor, &RichTextEditor::slotDisplayMessageIndicator);

    connect(mFindBar, &PimCommon::RichTextEditFindBar::hideFindBar, mSliderContainer, &PimCommon::SlideContainer::slideOut);
    mSliderContainer->setContent(mFindBar);
    lay->addWidget(mSliderContainer);

    QShortcut *shortcut = new QShortcut(this);
    shortcut->setKey(Qt::Key_F + Qt::CTRL);
    connect(shortcut, &QShortcut::activated, this, &RichTextEditorWidget::slotFind);
    connect(mEditor, &RichTextEditor::findText, this, &RichTextEditorWidget::slotFind);

    shortcut = new QShortcut(this);
    shortcut->setKey(Qt::Key_R + Qt::CTRL);
    connect(shortcut, &QShortcut::activated, this, &RichTextEditorWidget::slotReplace);
    connect(mEditor, &RichTextEditor::replaceText, this, &RichTextEditorWidget::slotReplace);

    setLayout(lay);
}

bool RichTextEditorWidget::isReadOnly() const
{
    return mEditor->isReadOnly();
}

void RichTextEditorWidget::setReadOnly(bool readOnly)
{
    mEditor->setReadOnly(readOnly);
}

void RichTextEditorWidget::slotReplace()
{
    if (mEditor->searchSupport()) {
        if (mEditor->textCursor().hasSelection()) {
            mFindBar->setText(mEditor->textCursor().selectedText());
        }
        mFindBar->showReplace();
        mSliderContainer->slideIn();
        mFindBar->focusAndSetCursor();
    }
}

void RichTextEditorWidget::slotFindNext()
{
    if (mEditor->searchSupport()) {
        if (mFindBar->isVisible()) {
            mFindBar->findNext();
        }
    }
}

void RichTextEditorWidget::slotFind()
{
    if (mEditor->searchSupport()) {
        if (mEditor->textCursor().hasSelection()) {
            mFindBar->setText(mEditor->textCursor().selectedText());
        }
        mEditor->moveCursor(QTextCursor::Start);

        mFindBar->showFind();
        mSliderContainer->slideIn();
        mFindBar->focusAndSetCursor();
    }
}

