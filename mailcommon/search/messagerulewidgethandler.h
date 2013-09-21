/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#ifndef MESSAGERULEWIDGETHANDLER_H
#define MESSAGERULEWIDGETHANDLER_H

#include "interfaces/rulewidgethandler.h"

namespace MailCommon {
class MessageRuleWidgetHandler : public MailCommon::RuleWidgetHandler
{
public:
    MessageRuleWidgetHandler() : MailCommon::RuleWidgetHandler()
    {
    }

    ~MessageRuleWidgetHandler()
    {
    }

    QWidget *createFunctionWidget(int number,
                                   QStackedWidget *functionStack,
                                   const QObject *receiver , bool isNepomukSearch) const;

    QWidget *createValueWidget( int number,
                                QStackedWidget *valueStack,
                                const QObject *receiver ) const;

    SearchRule::Function function( const QByteArray & field,
                                   const QStackedWidget *functionStack ) const;

    QString value( const QByteArray &field,
                   const QStackedWidget *functionStack,
                   const QStackedWidget *valueStack ) const;

    QString prettyValue( const QByteArray & field,
                         const QStackedWidget *functionStack,
                         const QStackedWidget *valueStack ) const;

    bool handlesField( const QByteArray & field ) const;

    void reset( QStackedWidget *functionStack,
                QStackedWidget *valueStack ) const;

    bool setRule(QStackedWidget *functionStack,
                  QStackedWidget *valueStack,
                  const SearchRule::Ptr rule , bool isNepomukSearch) const;

    bool update( const QByteArray & field,
                 QStackedWidget *functionStack,
                 QStackedWidget *valueStack ) const;

private:
    SearchRule::Function currentFunction( const QStackedWidget *functionStack ) const;
    QString currentValue( const QStackedWidget *valueStack,
                          SearchRule::Function func ) const;
};
}

#endif // MESSAGERULEWIDGETHANDLER_H
