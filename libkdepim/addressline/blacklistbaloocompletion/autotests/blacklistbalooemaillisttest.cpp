/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "blacklistbalooemaillisttest.h"
#include "../blacklistbalooemaillist.h"
#include <qtest_kde.h>

BlackListBalooEmailListTest::BlackListBalooEmailListTest(QObject *parent)
    : QObject(parent)
{

}

BlackListBalooEmailListTest::~BlackListBalooEmailListTest()
{

}

void BlackListBalooEmailListTest::shouldHaveDefaultValue()
{
    KPIM::BlackListBalooEmailList blackList;
    QVERIFY(blackList.count() == 0);
}

void BlackListBalooEmailListTest::shouldFillListEmail()
{
    KPIM::BlackListBalooEmailList blackList;
    blackList.slotEmailFound(QStringList() << QLatin1String("foo") << QLatin1String("bla") << QLatin1String("bli"));
    QCOMPARE(blackList.count(), 3);
}

QTEST_KDEMAIN(BlackListBalooEmailListTest, GUI)
