/*
  This file is part of the KDE project

  Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>
  Copyright (C) 2004 Michael Brade <brade@kde.org>

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

#ifndef KNOTES_PART_H
#define KNOTES_PART_H

#include <KParts/ReadOnlyPart>
#include <QListWidgetItem>
#include <KViewStateMaintainer>
#include <Akonadi/Item>

class KNotesIconView;
class KNotesWidget;
class KNotesIconViewItem;
class KNoteTip;
class KAction;
class KToggleAction;
class QTcpServer;

namespace DNSSD {
class PublicService;
}
namespace Akonadi {
class ChangeRecorder;
class Collection;
class EntityTreeModel;
class ETMViewStateSaver;
}
namespace NoteShared {
class NotesChangeRecorder;
class NotesAkonadiTreeModel;
}
class KCheckableProxyModel;

class KNotesPart : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    explicit KNotesPart(QObject *parent = 0 );
    ~KNotesPart();

    bool openFile();

    NoteShared::NotesAkonadiTreeModel *noteTreeModel() const {return mNoteTreeModel;}

public slots:
    void newNote( const QString &name = QString(),
                     const QString &text = QString() );
    void newNoteFromClipboard( const QString &name = QString() );
    QStringList notesList() const;


public:
    void killNote( const QString &id );
    void killNote( const QString &id, bool force );

    QString name(const Akonadi::Item::Id &id ) const;
    QString text( const Akonadi::Item::Id &id ) const;

    void setName( const Akonadi::Item::Id &id, const QString &newName );
    void setText( const Akonadi::Item::Id &id, const QString &newText );

    QMap<QString, QString> notes() const;
    void popupRMB( QListWidgetItem *item, const QPoint &pos, const QPoint &globalPos );

private slots:
    void editNote( QListWidgetItem *item );
    void editNote();

    void renameNote();

    void slotOnCurrentChanged( );

    void killSelectedNotes();

    void slotPrintSelectedNotes();
    void slotPrintPreviewSelectedNotes();
    void requestToolTip( const QModelIndex & );

    void hideToolTip();
    void slotNotePreferences();
    void slotPreferences();
    void slotApplyConfig();
    void slotMail();
    void slotSendToNetwork();
    void slotConfigUpdated();
    void slotAcceptConnection();
    void slotSetAlarm();
    void slotNewNoteFromClipboard();
    void slotSaveAs();
    void slotUpdateReadOnly();

    void slotNoteCreationFinished(KJob *job);
private:
    void updateNetworkListener();
    void printSelectedNotes(bool preview);
    KNotesWidget *mNotesWidget;
    KNoteTip *mNoteTip;

    QMultiHash<QString, KNotesIconViewItem*> mNoteList;
    QTcpServer *mListener;
    DNSSD::PublicService *mPublisher;
    KAction *mNoteEdit;
    KAction *mNoteRename;
    KAction *mNoteDelete;
    KAction *mNotePrint;
    KAction *mNotePrintPreview;
    KAction *mNoteConfigure;
    KAction *mNoteSendMail;
    KAction *mNoteSendNetwork;
    KAction *mNoteSetAlarm;
    KAction *mNewNote;
    KAction *mSaveAs;
    KToggleAction *mReadOnly;
    NoteShared::NotesChangeRecorder *mNoteRecorder;
    NoteShared::NotesAkonadiTreeModel *mNoteTreeModel;
    QItemSelectionModel *mSelectionModel;
    KCheckableProxyModel *mModelProxy;
    KViewStateMaintainer<Akonadi::ETMViewStateSaver> *mModelState;
};

#endif
