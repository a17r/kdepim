/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

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

#ifndef NOTESAGENTNOTEDIALOG_H
#define NOTESAGENTNOTEDIALOG_H

#include <QDialog>
#include <Item>
class QLineEdit;
class KJob;
namespace KPIMTextEdit
{
class RichTextEditorWidget;
}
class NotesAgentNoteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NotesAgentNoteDialog(QWidget *parent = Q_NULLPTR);
    ~NotesAgentNoteDialog();

    void setNoteId(Akonadi::Item::Id id);

private Q_SLOTS:
    void slotFetchItem(KJob *job);

private:
    void readConfig();
    void writeConfig();
    KPIMTextEdit::RichTextEditorWidget *mNote;
    QLineEdit *mSubject;
};

#endif // NOTESAGENTNOTEDIALOG_H