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
#include "previewwidget.h"
#include "contactpreviewwidget.h"

#include <QHBoxLayout>

PreviewWidget::PreviewWidget(const QString &projectDirectory, QWidget *parent)
    : GrantleeThemeEditor::PreviewWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout;
    lay->setMargin(0);
    mPreview = new ContactPreviewWidget(projectDirectory);
    lay->addWidget(mPreview);
    setLayout(lay);
}

PreviewWidget::~PreviewWidget()
{
}

void PreviewWidget::loadConfig()
{
    mPreview->loadConfig();
    updateViewer();
}

void PreviewWidget::updateViewer()
{
    mPreview->updateViewer();
}

void PreviewWidget::createScreenShot(const QStringList &fileList)
{
    mPreview->createScreenShot(fileList);
}

void PreviewWidget::setThemePath(const QString &projectDirectory, const QString &mainPageFileName)
{
    Q_UNUSED(mainPageFileName)
    mPreview->setThemePath(projectDirectory);
    updateViewer();
}

