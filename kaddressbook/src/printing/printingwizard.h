/*
  This file is part of KAddressBook.
  Copyright (c) 1996-2002 Mirko Boehm <mirko@kde.org>
                          Tobias Koenig <tokoe@kde.org>

  Copyright (c) 2009-2016 Laurent Montel <montel@kde.org>

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

#ifndef PRINTINGWIZARD_H
#define PRINTINGWIZARD_H

#include <KContacts/Addressee>
#include <KAssistantDialog>

#include <QStringList>

class ContactSelectionWidget;
class StylePage;

class QItemSelectionModel;
class QPrinter;

namespace Akonadi
{
class Collection;
}

namespace KABPrinting
{

class PrintProgress;
class PrintStyle;
class PrintStyleFactory;

/**
 * The PrintingWizard combines pages common for all print styles
 * and those provided by the respective style.
 */
class PrintingWizard : public KAssistantDialog
{
    Q_OBJECT

public:
    /**
     * Creates a new printing wizard.
     *
     * @param printer The configured printer.
     * @param selectionModel The selection model to get the selected contacts from.
     * @param parent The parent widget.
     */
    PrintingWizard(QPrinter *printer,
                   QItemSelectionModel *selectionModel,
                   QWidget *parent = Q_NULLPTR);

    /**
     * Destroys the printing wizard.
     */
    ~PrintingWizard();

    /**
     * Sets the default addressbook of the contact selection.
     */
    void setDefaultAddressBook(const Akonadi::Collection &addressBook);

    /**
     * Registers all available printing styles.
     */
    void registerStyles();

    /**
     * Performs the actual printing.
     */
    void print();

    /**
     * Returns the printer to use for printing.
     */
    QPrinter *printer() const;

    /**
     * Returns the index of the selected style
     */
    int printingStyle() const;

    /**
     * Returns the sort order of addressBook
     */
    int sortOrder() const;

protected Q_SLOTS:
    /**
     * A print style has been selected. The argument is the index
     * in the cbStyle combo and in styles.
     */
    void slotStyleSelected(int);

protected:
    class PrintStyleDefinition
    {
    public:
        PrintStyleDefinition(PrintStyleFactory *factory = Q_NULLPTR, PrintStyle *style = Q_NULLPTR)
            : printstyleFactory(factory),
              printStyle(style)
        {

        }
        PrintStyleFactory *printstyleFactory;
        PrintStyle *printStyle;
    };

    QList<PrintStyleDefinition *> mPrintStyleDefinition;
    QPrinter *mPrinter;
    PrintStyle *mStyle;
    PrintProgress *mProgress;

    StylePage *mStylePage;
    ContactSelectionWidget *mSelectionPage;

    /**
     * Overloaded accept slot. This is used to do the actual
     * printing without having the wizard disappearing
     * before. What happens is actually up to the print style,
     * since it does the printing. It could display a progress
     * window, for example (hint, hint).
     */
    void accept() Q_DECL_OVERRIDE;

private:
    void writeConfig();
    void readConfig();
    void loadGrantleeStyle();
};

}

#endif
