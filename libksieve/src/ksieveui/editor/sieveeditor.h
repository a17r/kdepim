/* Copyright (c) 2011-2015 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KSIEVE_KSIEVEUI_SIEVEEDITOR_H
#define KSIEVE_KSIEVEUI_SIEVEEDITOR_H

#include "ksieveui_export.h"

#include <QDialog>
namespace KSieveUi
{
class SieveEditorPrivate;
class KSIEVEUI_EXPORT SieveEditor : public QDialog
{
    Q_OBJECT
public:
    explicit SieveEditor(QWidget *parent = Q_NULLPTR);
    ~SieveEditor();

    QString script() const;
    QString originalScript() const;
    void setScript(const QString &script);
    void setDebugScript(const QString &debug);
    void addFailedMessage(const QString &err);
    void addOkMessage(const QString &msg);
    void setScriptName(const QString &name);

    void resultDone();

    void setSieveCapabilities(const QStringList &capabilities);

private Q_SLOTS:
    void slotEnableButtonOk(bool b);
    void slotAccepted();
    void slotCanceled();

Q_SIGNALS:
    void checkSyntax();
    void valueChanged(bool);
    void okClicked();
    void cancelClicked();

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;

private:
    void writeConfig();
    void readConfig();
    SieveEditorPrivate *const d;
};

}

#endif
