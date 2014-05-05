/*
  This file is part of Kontact.

  Copyright (c) 2003-2013 Kontact Developer <kde-pim@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef KMAIL_PLUGIN_H
#define KMAIL_PLUGIN_H

#include <KontactInterface/kontactinterface/Plugin>
#include <KontactInterface/kontactinterface/UniqueAppHandler>

#include <KUrl>

class OrgKdeKmailKmailInterface;

namespace KontactInterface {
class UniqueAppWatcher;
}

class KMailUniqueAppHandler : public KontactInterface::UniqueAppHandler
{
public:
    explicit KMailUniqueAppHandler( KontactInterface::Plugin *plugin )
        : KontactInterface::UniqueAppHandler( plugin ) {}
    virtual void loadCommandLineOptions();
    virtual int newInstance();
};

class KMailPlugin : public KontactInterface::Plugin
{
    Q_OBJECT

public:
    KMailPlugin( KontactInterface::Core *core, const QVariantList & );
    ~KMailPlugin();

    bool isRunningStandalone() const;
    bool createDBUSInterface( const QString &serviceType );
    virtual KontactInterface::Summary *createSummaryWidget( QWidget *parent );
    QString tipFile() const;
    int weight() const { return 200; }

    QStringList invisibleToolbarActions() const;
    virtual bool queryClose() const;

    void shortcutChanged();

protected:
    virtual KParts::ReadOnlyPart *createPart();
    void openComposer( const KUrl &attach = KUrl() );
    void openComposer( const QString &to );
    bool canDecodeMimeData( const QMimeData * ) const;
    void processDropEvent( QDropEvent * );

protected slots:
    void slotNewMail();
    void slotSyncFolders();

private:
    OrgKdeKmailKmailInterface *m_instance;
    KontactInterface::UniqueAppWatcher *mUniqueAppWatcher;
};

#endif
