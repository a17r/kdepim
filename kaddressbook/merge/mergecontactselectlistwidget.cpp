/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "mergecontactselectlistwidget.h"
#include <KABC/Addressee>

using namespace KABMergeContacts;
using namespace KABC;

MergeContactSelectListWidget::MergeContactSelectListWidget(QWidget *parent)
    : QTreeWidget(parent)
{

}



MergeContactSelectListWidget::~MergeContactSelectListWidget()
{

}

void MergeContactSelectListWidget::setContacts(const Akonadi::Item::List &lst)
{
    //TODO fillContact ?
    clear();
    if (lst.isEmpty()) {
        return;
    }
}

void MergeContactSelectListWidget::fillList()
{

}
