/*
    This file is part of KOrganizer.
    Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// $Id$

#include <klocale.h>
#include <iostream.h>
#include "kotodoviewitem.h"
//#include "kotodoviewitem.moc"

KOTodoViewItem::KOTodoViewItem( QListView *parent, Todo *todo )
  : QCheckListItem( parent , "", CheckBox ), mTodo( todo )
{
  construct();
}

KOTodoViewItem::KOTodoViewItem( KOTodoViewItem *parent, Todo *todo )
  : QCheckListItem( parent, "", CheckBox ), mTodo( todo )
{
  construct();
}

QString KOTodoViewItem::key(int column,bool) const
{
  QMap<int,QString>::ConstIterator it = mKeyMap.find(column);
  if (it == mKeyMap.end()) {
    return text(column);
  } else {
    return *it;
  }
}

void KOTodoViewItem::setSortKey(int column,const QString &key)
{
  mKeyMap.insert(column,key);
}

#if QT_VERSION >= 300
void KOTodoViewItem::paintBranches(QPainter *p,const QColorGroup & cg,int w,
                                   int y,int h)
{
  QListViewItem::paintBranches(p,cg,w,y,h);
}
#else
#endif

void KOTodoViewItem::construct()
{
  QString keyd = "==";
  QString keyt = "==";

  setOn(mTodo->isCompleted());
  setText(0,mTodo->summary());
  setText(1,QString::number(mTodo->priority()));
  setText(2,i18n("%1 %").arg(QString::number(mTodo->percentComplete())));
  if (mTodo->percentComplete()<100) {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(mTodo->percentComplete()));
  }
  else {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(99));
  }
  if (mTodo->hasDueDate()) {
    setText(3, mTodo->dtDueDateStr());
    QDate d = mTodo->dtDue().date();
    keyd.sprintf("%04d%02d%02d",d.year(),d.month(),d.day());
    setSortKey(3,keyd);
    if (mTodo->doesFloat()) {
      setText(4,"");
    }
    else {
      setText(4,mTodo->dtDueTimeStr());
      QTime t = mTodo->dtDue().time();
      keyt.sprintf("%02d%02d",t.hour(),t.minute());
      setSortKey(4,keyt);
    }
  } else {
    setText(3,"");
    setText(4,"");
  }
  setSortKey(3,keyd);
  setSortKey(4,keyt);

  if (mTodo->isCompleted()) setSortKey(1,QString::number(9)+keyd+keyt);
  else setSortKey(1,QString::number(mTodo->priority())+keyd+keyt);

  setText(5,mTodo->categoriesStr());
  // Find sort id in description. It's the text behind the last '#' character
  // found in the description. White spaces are removed from beginning and end
  // of sort id.
  int pos = mTodo->description().findRev('#');
  if (pos < 0) {
    setText(6,"");
  } else {
    QString str = mTodo->description().mid(pos+1);
    str.stripWhiteSpace();
    setText(6,str);
  }
}

void KOTodoViewItem::stateChange(bool state)
{
  QString keyd = "==";
  QString keyt = "==";
  
  if (state) mTodo->setCompleted(state);
  else mTodo->setPercentComplete(0);
  if (isOn()!=state) {
    setOn(state);
//    emit isModified(true);
  }

  if (mTodo->hasDueDate()) {
    setText(3, mTodo->dtDueDateStr());
    QDate d = mTodo->dtDue().date();
    keyd.sprintf("%04d%02d%02d",d.year(),d.month(),d.day());
    setSortKey(3,keyd);
    if (mTodo->doesFloat()) {
      setText(4,"");
    }
    else {
      setText(4,mTodo->dtDueTimeStr());
      QTime t = mTodo->dtDue().time();
      keyt.sprintf("%02d%02d",t.hour(),t.minute());
      setSortKey(4,keyt);
    }
  }
  if (mTodo->isCompleted()) setSortKey(1,QString::number(9)+keyd+keyt);
  else setSortKey(1,QString::number(mTodo->priority())+keyd+keyt);
  
  setText(2,i18n("%1 %").arg(QString::number(mTodo->percentComplete())));
  if (mTodo->percentComplete()<100) {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(mTodo->percentComplete()));
  }
  else {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(99));
  }
  QListViewItem * myChild = firstChild();
  KOTodoViewItem *item;
  while( myChild ) {
    item = static_cast<KOTodoViewItem*>(myChild);
    item->stateChange(state);
    myChild = myChild->nextSibling();
  }
}
