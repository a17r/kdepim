/*
   This file is part of libkdepim.

   Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/** @file
* This file defines a generic subscription widget and some support classes.
*/

#ifndef __KSUBSCRIPTION
#define __KSUBSCRIPTION

#include <tqlistview.h>
#include <tqcheckbox.h>

#include <kdialogbase.h>
#include <kdepimmacros.h>
#include "kfoldertree.h"

class KSubscription;

class KLineEdit;
class QLayout;
class QLabel;
class QGridLayout;
class KAccount;

//==========================================================================

class KDE_EXPORT KGroupInfo
{
  public:
    enum Status {
      unknown,
      readOnly,
      postingAllowed,
      moderated
    };

    KGroupInfo( const TQString &name, const TQString &description = TQString::null,
        bool newGroup = false, bool subscribed = false,
        Status status = unknown, TQString path = TQString::null );

    TQString name, description;
    bool newGroup, subscribed;
    Status status;
    TQString path;

    bool operator== (const KGroupInfo &gi2);
    bool operator< (const KGroupInfo &gi2);

};

//==========================================================================

/** A class representing a single group item (what's that?) */
class KDE_EXPORT GroupItem : public QCheckListItem
{
  public:
    GroupItem( TQListView *v, const KGroupInfo &gi, KSubscription* browser,
        bool isCheckItem = false );
    GroupItem( TQListViewItem *i, const KGroupInfo &gi, KSubscription* browser,
        bool isCheckItem = false );

    /**
     * Get/Set the KGroupInfo
     */
    KGroupInfo info() { return mInfo; }
    void setInfo( KGroupInfo info );

    /**
     * Get/Set the original parent
     */
    TQListViewItem* originalParent() { return mOriginalParent; }
    void setOriginalParent( TQListViewItem* parent ) { mOriginalParent = parent; }

    /**
     * Get/Set the last open state
     */
    bool lastOpenState() { return mLastOpenState; }
    void setLastOpenState( bool last ) { mLastOpenState = last; }

    /**
     * Sets the description from the KGroupInfo
     * Reimplement this for special cases
     */
    virtual void setDescription();

    /**
     * Get if this is a checkable item
     */
    bool isCheckItem() const { return mIsCheckItem; }

    /**
     * Get/Set if state changes should be ignored
     */
    bool ignoreStateChange() { return mIgnoreStateChange; }
    void setIgnoreStateChange( bool ignore ) { mIgnoreStateChange = ignore; }

    /**
     * Reimplemented
     * Sets the subscribed property (only while items are loaded)
     */
    virtual void setOn( bool on );

    /**
     * Reimlemented
     * Calls KSubscription::changeItemState if mIgnoreStateChange == false
     */
    virtual void stateChange( bool on );

    /**
     * Reimplemented
     * Sets items invisible or disabled or even moves them
     */
    void setVisible( bool b );

    /**
     * Reimplemented
     * Calls TQListViewItem or QCheckListItem
     */
    virtual void paintCell( TQPainter * p, const TQColorGroup & cg,
        int column, int width, int align );

    /**
     * Reimplemented
     * Calls TQListViewItem or QCheckListItem
     */
    virtual void paintFocus( TQPainter *, const TQColorGroup & cg,
                 const TQRect & r );

    /**
     * Reimplemented
     * Calls TQListViewItem or QCheckListItem
     */
    virtual int width( const TQFontMetrics&, const TQListView*, int column) const;

    /**
     * Reimplemented
     * Calls TQListViewItem or QCheckListItem
     */
    virtual void setup();

    /** Reimplemented */
    virtual int rtti () const { return 15689; }

  protected:
    KGroupInfo mInfo;
    KSubscription* mBrowser;
    TQListViewItem* mOriginalParent;
    // remember last open state
    bool mLastOpenState;
    // is this a checkable item
    bool mIsCheckItem;
    // ignore state changes
    bool mIgnoreStateChange;
};

//==========================================================================

/**
 * This class provides a generic subscription widget
 * The dialog itself has a main listview that holds all items and two listviews that
 * show all changes. The user can change the state of the items via checkable items.
 * When you construct a new instance you need to provide an account and a caption
 * After inserting your items (checkable or not) you need to call slotLoadingComplete()
 * You should at least connect slots to the signals okClicked() (to save your changes)
 * and user1Clicked() (to reload the list)
 * You can hide unwanted checkboxes via the respective hide<checkboxname> methods
 *
 */

class KDE_EXPORT KSubscription : public KDialogBase
{
  Q_OBJECT

  public:
    /**
     * The direction of the buttons
     */
    enum Direction {
      Left,
      Right
    };

    KSubscription( TQWidget *parent, const TQString &caption, KAccount* acct,
        int buttons = 0, const TQString &user1 = TQString::null,
        bool descriptionColumn = true );

    ~KSubscription();

    /**
     * Get/Set the account
     */
    KAccount* account() { return mAcct; }
    void setAccount( KAccount * acct ) { mAcct = acct; }

    /**
     * Access to the treewidget that holds the GroupItems
     */
    TQListView* folderTree() { return groupView; }

    /**
     * Access to the searchfield
     */
    KLineEdit* searchField() { return filterEdit; }

    /**
     * The item that should be selected on startup
     */
    void setStartItem( const KGroupInfo &info );

    /**
     * Removes the item from the listview
     */
    void removeListItem( TQListView *view, const KGroupInfo &gi );

    /**
     * Gets the item from the listview
     * Returns 0 if the item can't be found
     */
    TQListViewItem* getListItem( TQListView *view, const KGroupInfo &gi );

    /**
     * Is the item in the given listview
     */
    bool itemInListView( TQListView *view, const KGroupInfo &gi );

    /**
     * Makes all changes after an item is toggled
     * called by the item's stateChange-method
     */
    void changeItemState( GroupItem* item, bool on );

    /**
     * Get/Set the direction of button1
     */
    Direction directionButton1() { return mDirButton1; }
    void setDirectionButton1( Direction dir );

    /**
     * Get/Set the direction of button2
     */
    Direction directionButton2() { return mDirButton2; }
    void setDirectionButton2( Direction dir );

    /**
     * Returns true if items are being constructed
     * Call 'slotLoadingComplete' to switch this
     */
    bool isLoading() { return mLoading; }

    /**
     * Hide 'Disable tree view' checkbox
     */
    void hideTreeCheckbox() { noTreeCB->hide(); }

    /**
     * Hide 'New Only' checkbox
     */
    void hideNewOnlyCheckbox() { newCB->hide(); }

    /**
     * Update the item-states (visible, enabled) when a filter
     * criteria changed
     */
    void filterChanged( TQListViewItem* item = 0,
        const TQString & text = TQString::null );

    /**
     * The amount of items that are visible and enabled
     */
    uint activeItemCount();

    /**
     * Moves all items from toplevel back to their original position
     */
    void restoreOriginalParent();

    /**
     * Saves the open states
     */
    void saveOpenStates();

    /**
     * Restores the saved open state
     */
    void restoreOpenStates();


  public slots:
    /**
     * Call this slot when you have created all items
     */
    void slotLoadingComplete();

    /**
     * Changes the current state of the buttons
     */
    void slotChangeButtonState( TQListViewItem* );

    /**
     * Buttons are clicked
     */
    void slotButton1();
    void slotButton2();

    /**
     * Updates the status-label
     */
    void slotUpdateStatusLabel();

    /**
     * The reload-button is pressed
     */
    void slotLoadFolders();

  protected slots:
    /**
     * Slot for the checkboxes
     */
    void slotCBToggled();

    /**
     * Filter text changed
     */
    void slotFilterTextChanged( const TQString & text );

  signals:
    /**
     * Emitted when the amount of items in the
     * groupView changes (e.g. on filtering)
     */
    void listChanged();


  protected:
    // current account
    KAccount* mAcct;

    // widgets
    TQWidget *page;
    TQListView *groupView;
    TQListView *subView, *unsubView;
    KLineEdit *filterEdit;
    TQCheckBox *noTreeCB, *subCB, *newCB;
    TQPushButton  *arrowBtn1, *arrowBtn2;
    TQIconSet pmRight, pmLeft;
    TQGridLayout *listL;
    TQLabel *leftLabel, *rightLabel;

    // false if all items are loaded
    bool mLoading;

    // directions
    Direction mDirButton1;
    Direction mDirButton2;

    // remember last searchtext
    TQString mLastText;

    // remember description column
    int mDescrColumn;
};

#endif
