/*
    This file is part of KMail.
    Copyright (c) 2002-2003 Don Sanders <sanders@kde.org>,
    Copyright (c) 2003      Zack Rusin  <zack@kde.org>,
    Based on the work of Cornelius Schumacher <schumacher@kde.org>

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

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/
#ifndef KMail_PART_H
#define KMail_PART_H

#include <kparts/browserextension.h>
#if KDE_IS_VERSION( 3, 1, 90 )
#include <kparts/statusbarextension.h>
#endif
#include <kparts/factory.h>
#include <kparts/event.h>
#include <qwidget.h>

#include "kmailpartIface.h"

class KInstance;
class KAboutData;
class KMailBrowserExtension;
class KMailStatusBarExtension;
class KMKernel;
class KMMainWidget;
class KMLittleProgressDlg;

class ActionManager;

class KMailPart: public KParts::ReadOnlyPart, virtual public KMailPartIface
{
    Q_OBJECT
  public:
    KMailPart(QWidget *parentWidget, const char *widgetName,
              QObject *parent, const char *name, const QStringList &);
    virtual ~KMailPart();

    QWidget* parentWidget() const;

    static KAboutData *createAboutData();

  public slots:
    virtual void save() { /*TODO*/ }
    virtual void exit();
    virtual void updateEditMenu() {};

  protected:
    virtual bool openFile();
    virtual void guiActivateEvent(KParts::GUIActivateEvent *e);

  private:
    KMKernel *kmailKernel;
    KMMainWidget *mainWidget;
    QWidget *widget;
    ActionManager *mActionManager;
    KMailBrowserExtension *m_extension;
#if KDE_IS_VERSION( 3, 1, 90 )
    KMailStatusBarExtension *mStatusBar;
#endif
    QWidget *mParentWidget;
};

class KMailBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
    friend class KMailPart;
  public:
    KMailBrowserExtension(KMailPart *parent);
    virtual ~KMailBrowserExtension();
};

#if KDE_IS_VERSION( 3, 1, 90 )
class KMailStatusBarExtension : public KParts::StatusBarExtension
{
public:
  KMailStatusBarExtension( KMailPart *parent );

  KMainWindow *mainWindow() const;

private:
  KMailPart *mParent;
  KMLittleProgressDlg *mLittleProgress;
};
#endif

#endif
