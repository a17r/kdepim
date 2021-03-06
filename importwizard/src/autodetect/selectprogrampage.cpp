/*
   Copyright (C) 2012-2016 Montel Laurent <montel@kde.org>

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

#include "selectprogrampage.h"
#include "ui_selectprogrampage.h"

SelectProgramPage::SelectProgramPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SelectProgramPage)
{
    ui->setupUi(this);
    connect(ui->listProgramFound, &QListWidget::itemSelectionChanged, this, &SelectProgramPage::slotItemSelectionChanged);
    connect(ui->listProgramFound, &QListWidget::itemDoubleClicked, this, &SelectProgramPage::slotItemDoubleClicked);
    connect(ui->manualSelectionCheckBox, &QCheckBox::clicked, this, &SelectProgramPage::slotSelectManualSelectionChanged);
}

SelectProgramPage::~SelectProgramPage()
{
    delete ui;
}

void SelectProgramPage::setFoundProgram(const QStringList &list)
{
    ui->listProgramFound->setNoProgramFound(list.isEmpty());
    ui->listProgramFound->addItems(list);
}

void SelectProgramPage::slotItemSelectionChanged()
{
    if (ui->listProgramFound->currentItem()) {
        Q_EMIT programSelected(ui->listProgramFound->currentItem()->text());
    }
}

void SelectProgramPage::slotItemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        Q_EMIT doubleClicked();
    }
}

void SelectProgramPage::disableSelectProgram()
{
    ui->listProgramFound->setEnabled(false);
}

void SelectProgramPage::slotSelectManualSelectionChanged(bool b)
{
    ui->listProgramFound->setEnabled(!b);
    Q_EMIT selectManualSelectionChanged(b);
}
