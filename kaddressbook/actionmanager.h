/*
    This file is part of KAddressBook.
    Copyright (c) 2002 Mike Pilone <mpilone@slac.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <qptrlist.h>
#include <qobject.h>

#include <kaction.h>

class KAddressBook;
class KXMLGUIClient;
class ViewManager;

/**
  The ActionManager creates all the actions in KAddressBook. This class
  is shared between the main application and the part so all common
  actions are in one location.
 */
class ActionManager : public QObject
{
  Q_OBJECT

  public:
    ActionManager( KXMLGUIClient *client, KAddressBook *widget,
                   bool readWrite, QObject *parent );
    ~ActionManager();

    void setReadWrite( bool rw );

    bool isModified() const;

  public slots:
    void initActionViewList();

  protected slots:
    /**
      Called whenever an addressee is selected or unselected.
    
      @param selected True if there is an addressee select, false otherwise
     */
    void addresseeSelected( bool selected );

    /**
      Rebuilds content of the filter combo box.
     */
    void filtersEdited();

    /**
      Called whenever the addressbook is modified.
    
      @see KAddressBook
     */
    void modified( bool mod );

    /**
      Called whenever the view selection changes.
     */
    void slotViewSelected();

    /**
      Called whenever the view configuration changes. This usually means
      a view was added or deleted.
     */
    void viewConfigChanged( const QString &newActive );

    /**
      Called whenever the user clicks changes the view policy
      of a quick tool. Handles Feature Bar and Jump Button Bar.
     */
    void quickToolsAction();

    void updateEditMenu();

  private slots:
    void clipboardDataChanged();
    void slotFilterActivated();
    void currentFilterChanged( const QString& );
    void keyBindings();
    void reloadExtensionNames();
    void insertFilters();

  signals:
    /**
      Announce filter selection changes.
     */
    void filterActivated( int );

  private:
    /**
      Create all the read only actions. These are all the actions that
      cannot modify the addressbook.
     */
    void initReadOnlyActions();

    /**
      Create all the read write actions. These are all the actions that
      can modify the addressbook.
     */
    void initReadWriteActions();

    /**
      Destroys all the read write actions.
     */
    void destroyReadWriteActions();


    QString mActiveViewName;

    KXMLGUIClient *mGUIClient;
    KAddressBook *mWidget;
    KActionCollection *mACollection;
    ViewManager *mViewManager;

    bool mModified;
    bool mReadWrite;
    bool mIsPart;

    KAction *mActionPaste;
    KAction *mActionCut;
    KAction *mActionDelete;
    KAction *mActionCopy;
    KAction *mActionEditAddressee;
    KAction *mActionMail;
    KAction *mActionMailVCard;
    KAction *mActionUndo;
    KAction *mActionRedo;
    KAction *mActionSave;
    KAction *mActionDeleteView;
    KAction *mActionSetPersonal;
    KToggleAction *mActionJumpBar;
    KSelectAction *mActionExtensions;
    KToggleAction *mActionDetails;
    KSelectAction *mActionSelectFilter;
    KSelectAction *mActionViewList;
};

#endif
