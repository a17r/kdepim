/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "gravatarconfigwidgettest.h"
#include "../widgets/gravatarconfigwidget.h"
#include <QSpinBox>
#include <QCheckBox>
#include <qtest.h>
#include <QLabel>
#include <QPushButton>
#include <qtestmouse.h>
#include <QSignalSpy>

GravatarConfigWidgetTest::GravatarConfigWidgetTest(QObject *parent)
    : QObject(parent)
{

}

GravatarConfigWidgetTest::~GravatarConfigWidgetTest()
{

}

void GravatarConfigWidgetTest::shouldHaveDefaultValue()
{
    MessageViewer::GravatarConfigWidget w;
    QCheckBox *checkBox = w.findChild<QCheckBox *>(QStringLiteral("gravatarcheckbox"));
    QVERIFY(checkBox);

    QCheckBox *useDefaultImage = w.findChild<QCheckBox *>(QLatin1String("usedefaultimage"));
    QVERIFY(useDefaultImage);

    QPushButton *clearGravatarCache = w.findChild<QPushButton *>(QLatin1String("cleargravatarcachebutton"));
    QVERIFY(clearGravatarCache);

    QLabel *lab = w.findChild<QLabel *>(QLatin1String("gravatarcachesizelabel"));
    QVERIFY(lab);

    QSpinBox *gravatarCacheSize = w.findChild<QSpinBox *>(QLatin1String("gravatarcachesize"));
    QVERIFY(gravatarCacheSize);

    QCheckBox *useLibravatar = w.findChild<QCheckBox *>(QStringLiteral("uselibravatarcheckbox"));
    QVERIFY(useLibravatar);

    QCheckBox *fallbackGravatar = w.findChild<QCheckBox *>(QStringLiteral("fallbackgravatar"));
    QVERIFY(fallbackGravatar);
}

void GravatarConfigWidgetTest::shouldChangeState()
{
    MessageViewer::GravatarConfigWidget w;
    w.show();
    QCheckBox *checkBox = qFindChild<QCheckBox *>(&w, QLatin1String("gravatarcheckbox"));
    QCheckBox *useDefaultImage = qFindChild<QCheckBox *>(&w, QLatin1String("usedefaultimage"));
    QPushButton *clearGravatarCache = qFindChild<QPushButton *>(&w, QLatin1String("cleargravatarcachebutton"));
    QSpinBox *gravatarCacheSize = qFindChild<QSpinBox *>(&w, QLatin1String("gravatarcachesize"));
    QCheckBox *useLibravatar = w.findChild<QCheckBox *>(QStringLiteral("uselibravatarcheckbox"));
    QCheckBox *fallbackGravatar = w.findChild<QCheckBox *>(QStringLiteral("fallbackgravatar"));

    checkBox->setChecked(false);
    QVERIFY(checkBox->isEnabled());
    QVERIFY(!checkBox->isChecked());
    QVERIFY(!useDefaultImage->isEnabled());
    QVERIFY(!clearGravatarCache->isEnabled());
    QVERIFY(!gravatarCacheSize->isEnabled());
    QVERIFY(!fallbackGravatar->isEnabled());
    QVERIFY(!useLibravatar->isEnabled());

    QTest::mouseClick(checkBox, Qt::LeftButton);
    QVERIFY(checkBox->isEnabled());
    QVERIFY(checkBox->isChecked());
    QVERIFY(useDefaultImage->isEnabled());
    QVERIFY(clearGravatarCache->isEnabled());
    QVERIFY(gravatarCacheSize->isEnabled());
    QVERIFY(fallbackGravatar->isEnabled());
    QVERIFY(useLibravatar->isEnabled());

    QTest::mouseClick(checkBox, Qt::LeftButton);
    QVERIFY(checkBox->isEnabled());
    QVERIFY(!useDefaultImage->isEnabled());
    QVERIFY(!clearGravatarCache->isEnabled());
    QVERIFY(!gravatarCacheSize->isEnabled());
    QVERIFY(!fallbackGravatar->isEnabled());
    QVERIFY(!useLibravatar->isEnabled());

}

void GravatarConfigWidgetTest::shoulEmitConfigChangedSignal()
{
    MessageViewer::GravatarConfigWidget w;
    w.show();
    QCheckBox *checkBox = qFindChild<QCheckBox *>(&w, QLatin1String("gravatarcheckbox"));
    QCheckBox *useDefaultImage = qFindChild<QCheckBox *>(&w, QLatin1String("usedefaultimage"));
    QSpinBox *gravatarCacheSize = qFindChild<QSpinBox *>(&w, QLatin1String("gravatarcachesize"));
    QCheckBox *useLibravatar = w.findChild<QCheckBox *>(QStringLiteral("uselibravatarcheckbox"));
    QCheckBox *fallbackGravatar = w.findChild<QCheckBox *>(QStringLiteral("fallbackgravatar"));
    QSignalSpy spy(&w, SIGNAL(configChanged(bool)));
    QTest::mouseClick(checkBox, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    QTest::mouseClick(useDefaultImage, Qt::LeftButton);
    QCOMPARE(spy.count(), 2);
    QTest::mouseClick(useDefaultImage, Qt::LeftButton);
    QCOMPARE(spy.count(), 3);

    QTest::mouseClick(useLibravatar, Qt::LeftButton);
    QCOMPARE(spy.count(), 4);

    QTest::mouseClick(fallbackGravatar, Qt::LeftButton);
    QCOMPARE(spy.count(), 5);

    // Disable other action => not signal emitted
    QTest::mouseClick(checkBox, Qt::LeftButton);
    QCOMPARE(spy.count(), 6);

    QTest::mouseClick(fallbackGravatar, Qt::LeftButton);
    QCOMPARE(spy.count(), 6);

}

QTEST_MAIN(GravatarConfigWidgetTest)
