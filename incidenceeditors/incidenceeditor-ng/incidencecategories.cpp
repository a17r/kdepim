/*
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>
    Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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

#include "incidencecategories.h"

#include "autochecktreewidget.h"
#include "categoryconfig.h"
#include "categoryhierarchyreader.h"
#include "categoryselectdialog.h"
#include "editorconfig.h"

#include "ui_eventortododesktop.h"

using namespace IncidenceEditors;
using namespace IncidenceEditorsNG;

IncidenceCategories::IncidenceCategories( Ui::EventOrTodoDesktop *ui )
  : mUi( ui )
{
  CategoryConfig cc( EditorConfig::instance()->config() );
  mUi->mCategoryCombo->setDefaultText( i18nc( "@item:inlistbox", "Select Categories" ) );
  mUi->mCategoryCombo->setSqueezeText( true );
  CategoryHierarchyReaderQComboBox( mUi->mCategoryCombo ).read( cc.customCategories() );

  connect( mUi->mCategoryCombo, SIGNAL(checkedItemsChanged(QStringList)),
           SLOT(setCategories(QStringList)) );
}

void IncidenceCategories::load( KCal::Incidence::ConstPtr incidence )
{
  mLoadedIncidence = incidence;
  if ( mLoadedIncidence )
    setCategories( mLoadedIncidence->categories() );
  else
    mSelectedCategories.clear();

  mWasDirty = false;
}

void IncidenceCategories::save( KCal::Incidence::Ptr incidence )
{
  Q_ASSERT( incidence );
  incidence->setCategories( mSelectedCategories );
}

bool IncidenceCategories::isDirty() const
{
  // If no Incidence was loaded, mSelectedCategories should be empty.
  bool categoriesEqual = mSelectedCategories.isEmpty();

  if ( mLoadedIncidence ) { // There was an Incidence loaded
    categoriesEqual = ( mLoadedIncidence->categories().size() == mSelectedCategories.size() );
    if ( categoriesEqual ) {
      QStringListIterator it( mLoadedIncidence->categories() );
      while ( it.hasNext() && categoriesEqual )
        categoriesEqual = mSelectedCategories.contains( it.next() );
    }
  }
  return !categoriesEqual;
}

void IncidenceCategories::selectCategories()
{
#ifdef KDEPIM_MOBILE_UI
  CategoryConfig cc( EditorConfig::instance()->config() );
  QPointer<CategorySelectDialog> dialog( new CategorySelectDialog( &cc ) );
  dialog->setSelected( mSelectedCategories );
  dialog->exec();

  setCategories( dialog->selectedCategories() );
  delete dialog;
#endif
}

void IncidenceCategories::setCategories( const QStringList &categories )
{
  mSelectedCategories = categories;
#ifdef KDEPIM_MOBILE_UI
//  mUi->mCategoriesLabel->setText( mSelectedCategories.join( QLatin1String( "," ) ) );
#endif
  checkDirtyStatus();
}

