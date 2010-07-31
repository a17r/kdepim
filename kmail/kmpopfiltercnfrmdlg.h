/***************************************************************************
                          kmpopheadersdlg.h  -  description
                             -------------------
    begin                : Sat Nov 3 2001
    copyright            : (C) 2001 by Heiko Hund
    email                : heiko@ist.eigentlich.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KMPOPHEADERSDLG_H
#define KMPOPHEADERSDLG_H

#include "kmpopheaders.h"

#include <kdialogbase.h>
#include <klistview.h>

#include <tqptrlist.h>
#include <tqmap.h>

class QWidget;
class QString;

class KMPopFilterCnfrmDlg;
/**
  * @author Heiko Hund
  */
class KMPopHeadersView : public KListView
{
  Q_OBJECT

public:
  KMPopHeadersView(TQWidget *aParent=0, KMPopFilterCnfrmDlg *aDialog=0);
  ~KMPopHeadersView();
  static const KMPopFilterAction mapToAction(int aColumn) { return (KMPopFilterAction)aColumn;};
  static const int mapToColumn(KMPopFilterAction aAction) { return (int)aAction;};
  static const char *mUnchecked[26];
  static const char *mChecked[26];
protected:
  static const char *mLater[25];
  static const char *mDown[20];
  static const char *mDel[19];
  void keyPressEvent( TQKeyEvent *k);

protected slots: // Protected slots
  void slotPressed(TQListViewItem* aItem, const TQPoint& aPoint, int aColumn);

private:
  KMPopFilterCnfrmDlg *mDialog;
};



class KMPopHeadersViewItem : public KListViewItem
{
public:
  KMPopHeadersViewItem(KMPopHeadersView *aParent, KMPopFilterAction aAction);
  ~KMPopHeadersViewItem();
  void setAction(KMPopFilterAction aAction);
  KMPopFilterAction action() { return mAction; };
  virtual void paintFocus(TQPainter *, const TQColorGroup & cg, const TQRect &r);
  virtual TQString key(int col, bool ascending) const;
protected:
  KMPopHeadersView *mParent;
  KMPopFilterAction mAction;
};


class KMPopFilterCnfrmDlg : public KDialogBase
{
	friend class ::KMPopHeadersView;
  Q_OBJECT
protected:
  KMPopFilterCnfrmDlg() { };
  TQMap<TQListViewItem*, KMPopHeaders*> mItemMap;
  TQPtrList<KMPopHeadersViewItem> mDelList;
  TQPtrList<KMPopHeaders> mDDLList;
  KMPopHeadersView *mFilteredHeaders;
  bool mLowerBoxVisible;
  bool mShowLaterMsgs;
  void setupLVI(KMPopHeadersViewItem *lvi, KMMessage *msg);
	

public:
  KMPopFilterCnfrmDlg(TQPtrList<KMPopHeaders> *aHeaders, const TQString &aAccount, bool aShowLaterMsgs = false, TQWidget *aParent=0, const char *aName=0);
  ~KMPopFilterCnfrmDlg();

public:
  void setAction(TQListViewItem *aItem, KMPopFilterAction aAction);

protected slots: // Protected slots
  /**
    This Slot is called whenever a ListView item was pressed.
    It checks for the column the button was pressed in and changes the action if the
    click happened over a radio button column.
    Of course the radio button state is changed as well if the above is true.
*/
  void slotPressed(TQListViewItem *aItem, const TQPoint &aPnt, int aColumn);
  void slotToggled(bool aOn);
  void slotUpdateMinimumSize();
};

#endif
