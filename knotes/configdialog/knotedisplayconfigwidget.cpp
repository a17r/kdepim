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

#include "knotedisplayconfigwidget.h"

#include <KLocale>
#include <KColorButton>
#include <KIntNumInput>
#include <KDialog>

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>

KNoteDisplayConfigWidget::KNoteDisplayConfigWidget( bool defaults, QWidget *parent )
    : QWidget( parent )
{
    QGridLayout *layout = new QGridLayout( this );
    layout->setSpacing( KDialog::spacingHint() );
    layout->setMargin( defaults ? KDialog::marginHint() : 0 );

    QLabel *label_FgColor = new QLabel( i18n( "&Text color:" ),this );
    label_FgColor->setObjectName( QLatin1String("label_FgColor") );
    layout->addWidget( label_FgColor, 0, 0 );

    KColorButton *kcfg_FgColor = new KColorButton( this );
    kcfg_FgColor->setObjectName( QLatin1String("kcfg_FgColor") );
    label_FgColor->setBuddy( kcfg_FgColor );
    layout->addWidget( kcfg_FgColor, 0, 1 );

    QLabel *label_BgColor = new QLabel( i18n( "&Background color:" ),
                                        this );
    label_BgColor->setObjectName( QLatin1String("label_BgColor") );
    layout->addWidget( label_BgColor, 1, 0 );

    KColorButton *kcfg_BgColor = new KColorButton( this );
    kcfg_BgColor->setObjectName( QLatin1String("kcfg_BgColor") );
    label_BgColor->setBuddy( kcfg_BgColor );
    layout->addWidget( kcfg_BgColor, 1, 1 );

    QCheckBox *kcfg_ShowInTaskbar =
            new QCheckBox( i18n( "&Show note in taskbar" ), this );
    kcfg_ShowInTaskbar->setObjectName( QLatin1String("kcfg_ShowInTaskbar") );
#ifdef Q_WS_X11
    QCheckBox *kcfg_RememberDesktop =
            new QCheckBox( i18n( "&Remember desktop" ), this );
    kcfg_RememberDesktop->setObjectName( QLatin1String("kcfg_RememberDesktop") );
#endif
    if ( defaults ) {
        QLabel *label_Width = new QLabel( i18n( "Default &width:" ), this );

        layout->addWidget( label_Width, 2, 0 );

        KIntNumInput *kcfg_Width = new KIntNumInput( this );
        kcfg_Width->setObjectName( QLatin1String("kcfg_Width") );
        label_Width->setBuddy( kcfg_Width );
        kcfg_Width->setRange( 50, 2000, 10 );
        kcfg_Width->setSliderEnabled( false );
        layout->addWidget( kcfg_Width, 2, 1 );

        QLabel *label_Height = new QLabel( i18n( "Default &height:" ),
                                           this );
        layout->addWidget( label_Height, 3, 0 );

        KIntNumInput *kcfg_Height = new KIntNumInput( this );
        kcfg_Height->setObjectName( QLatin1String("kcfg_Height") );
        kcfg_Height->setRange( 50, 2000, 10 );
        kcfg_Height->setSliderEnabled( false );
        label_Height->setBuddy( kcfg_Height );
        layout->addWidget( kcfg_Height, 3, 1 );

        layout->addWidget( kcfg_ShowInTaskbar, 4, 0 );
#ifdef Q_WS_X11
        layout->addWidget( kcfg_RememberDesktop, 5, 0 );
#endif
    } else {
        layout->addWidget( kcfg_ShowInTaskbar, 2, 0 );
#ifdef Q_WS_X11
        layout->addWidget( kcfg_RememberDesktop, 3, 0 );
#endif
    }
    layout->setRowStretch(4,1);
}
