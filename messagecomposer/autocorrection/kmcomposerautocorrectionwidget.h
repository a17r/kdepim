/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>
  
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

#ifndef KMCOMPOSERAUTOCORRECTIONWIDGET_H
#define KMCOMPOSERAUTOCORRECTIONWIDGET_H

#include "messagecomposer_export.h"
#include "autocorrection/kmcomposerautocorrection.h"
#include <QWidget>
#include <KDialog>
#include <KCharSelect>

namespace Ui {
class KMComposerAutoCorrectionWidget;
}

class KMComposerAutoCorrection;

class MESSAGECOMPOSER_EXPORT KMComposerAutoCorrectionWidget : public QWidget
{
  Q_OBJECT
    
public:
  explicit KMComposerAutoCorrectionWidget(KMComposerAutoCorrection * autoCorrect,QWidget *parent = 0);
  ~KMComposerAutoCorrectionWidget();
  void loadConfig();
  void writeConfig();
  void resetToDefault();

private Q_SLOTS:
  /* tab 2 */
  void enableSingleQuotes(int state);
  void enableDoubleQuotes(int state);
  void selectSingleQuoteCharOpen();
  void selectSingleQuoteCharClose();
  void setDefaultSingleQuotes();
  void selectDoubleQuoteCharOpen();
  void selectDoubleQuoteCharClose();
  void setDefaultDoubleQuotes();

  /* tab 3 */
  void enableAdvAutocorrection(int state);
  void addAutocorrectEntry();
  void removeAutocorrectEntry();
  void setFindReplaceText(int row, int column);
  void enableAddRemoveButton();
  void changeCharFormat();

  /* tab 4 */
  void abbreviationChanged(const QString &text);
  void twoUpperLetterChanged(const QString &text);
  void addAbbreviationEntry();
  void removeAbbreviationEntry();
  void addTwoUpperLetterEntry();
  void removeTwoUpperLetterEntry();

private:
  KMComposerAutoCorrection::TypographicQuotes m_singleQuotes;
  KMComposerAutoCorrection::TypographicQuotes m_doubleQuotes;
  QSet<QString> m_upperCaseExceptions;
  QSet<QString> m_twoUpperLetterExceptions;
  QHash<QString, QString> m_autocorrectEntries;
  Ui::KMComposerAutoCorrectionWidget *ui;
  KMComposerAutoCorrection *mAutoCorrection;
};

class CharSelectDialog : public KDialog
{
  Q_OBJECT
public:
  explicit CharSelectDialog(QWidget *parent);
  QChar currentChar() const;
  void setCurrentChar(const QChar &c);

private:
  KCharSelect *m_charSelect;
};

#endif // KMCOMPOSERAUTOCORRECTIONWIDGET_H
