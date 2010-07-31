/*
   This file is part of KDE Kontact.

   Copyright (c) 2001 Matthias Hoelzer-Kluepfel <mhk@kde.org>
   Copyright (c) 2002-2003 Daniel Molkentin <molkentin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/
#ifndef KONTACT_CORE_H
#define KONTACT_CORE_H

#include <tqdatetime.h>
#include <kdepimmacros.h>
#include <kparts/mainwindow.h>
#include <kparts/part.h>

class KAction;

namespace Kontact
{

class Plugin;

/**
  This class provides the interface to the Kontact core for the plugins.
*/
class KDE_EXPORT Core : public KParts::MainWindow
{
  Q_OBJECT
  public:
    virtual ~Core();

    /**
      Selects the given plugin @param plugin and raises the associated
      part.
     */
    virtual void selectPlugin( Kontact::Plugin *plugin ) = 0;

    /**
      This is an overloaded member function. It behaves essentially like the
      above function.
     */
    virtual void selectPlugin( const TQString &plugin ) = 0;

    /**
      Returns the pointer list of available plugins.
     */
    virtual TQValueList<Kontact::Plugin*> pluginList() const = 0;

    /**
     @internal (for Plugin)
     */
    KParts::ReadOnlyPart *createPart( const char *libname );

    /**
     @internal (for Plugin)
     Tell kontact that a part was loaded
     */
    virtual void partLoaded( Plugin* plugin, KParts::ReadOnlyPart * part ) = 0;

  signals:
    /**
      Emitted when a new day starts
      */
    void dayChanged( const TQDate& );

  protected:
    Core( TQWidget *parentWidget = 0, const char *name = 0 );

    TQString lastErrorMessage() const;

  private slots:
    void slotPartDestroyed( TQObject * );
    void checkNewDay();

  private:
    TQMap<TQCString,KParts::ReadOnlyPart *> mParts;
    TQDate mLastDate;

    class Private;
    Private *d;
};

}

#endif

// vim: sw=2 sts=2 et tw=80
