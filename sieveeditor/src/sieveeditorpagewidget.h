/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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

#ifndef SIEVEEDITORPAGEWIDGET_H
#define SIEVEEDITORPAGEWIDGET_H
#include <QWidget>
#include <QUrl>
#include "ksieveui/sieveeditorwidget.h"
namespace KManageSieve
{
class SieveJob;
}
namespace KSieveUi
{
class SieveEditorWidget;
}

class SieveEditorPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SieveEditorPageWidget(QWidget *parent = Q_NULLPTR);
    ~SieveEditorPageWidget();

    void loadScript(const QUrl &url, const QStringList &capabilities);
    QUrl currentUrl() const;
    void setIsNewScript(bool isNewScript);
    void uploadScript(bool showInformation = true, bool forceSave = false);
    bool needToSaveScript();

    bool isModified() const;
    void goToLine();
    KSieveUi::SieveEditorWidget::EditorMode pageMode() const;

    void find();
    void replace();
    void redo();
    void undo();
    bool isUndoAvailable() const;
    bool isRedoAvailable() const;
    void paste();
    void cut();
    void copy();
    bool hasSelection() const;

    void selectAll();
    void saveAs();
    void checkSpelling();
    void shareScript();
    void import();
    void createRulesGraphically();
    void checkSyntax();
    void comment();
    void uncomment();
    void lowerCase();
    void upperCase();
    void sentenceCase();
    void reverseCase();
    void zoomIn();
    void zoomOut();
    QString currentHelpTitle() const;
    QUrl currentHelpUrl() const;
    void openBookmarkUrl(const QUrl &url);
    void debugSieveScript();
    void zoomReset();
    void wordWrap(bool state);
    bool isWordWrap() const;
    void print();
    void printPreview();
    bool printSupportEnabled() const;
    bool isTextEditor() const;
Q_SIGNALS:
    void refreshList();
    void scriptModified(bool, SieveEditorPageWidget *);
    void modeEditorChanged(KSieveUi::SieveEditorWidget::EditorMode);
    void undoAvailable(bool);
    void redoAvailable(bool);
    void copyAvailable(bool);
    void sieveEditorTabCurrentChanged();

private Q_SLOTS:
    void slotGetResult(KManageSieve::SieveJob *, bool success, const QString &script, bool isActive);
    void slotCheckSyntaxClicked();
    void slotPutResultDebug(KManageSieve::SieveJob *, bool success);
    void slotPutResult(KManageSieve::SieveJob *, bool success);
    void slotValueChanged(bool b);

private:
    void setModified(bool b);
    QUrl mCurrentURL;
    KSieveUi::SieveEditorWidget *mSieveEditorWidget;
    bool mWasActive;
    bool mIsNewScript;
};

#endif // SIEVEEDITORPAGEWIDGET_H
