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

#include "autocorrection_gui.h"
#include "messagecomposer/autocorrection/composerautocorrection.h"
#include "messagecomposer/autocorrection/composerautocorrectionwidget.h"
#include "messagecomposer/settings/messagecomposersettings.h"

#include <kdebug.h>
#include <kapplication.h>
#include <KCmdLineArgs>
#include <KLocale>
#include <KSharedConfig>

#include <QDebug>
#include <QPointer>
#include <QVBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QToolBar>
#include <QAction>


ConfigureTestDialog::ConfigureTestDialog(MessageComposer::ComposerAutoCorrection *autoCorrection, QWidget *parent)
    : KDialog(parent)
{
    setCaption( QLatin1String("Configure Autocorrection") );
    setButtons( Ok | Cancel);

    setDefaultButton( KDialog::Ok );

    QHBoxLayout *lay = new QHBoxLayout( mainWidget() );
    mWidget = new MessageComposer::ComposerAutoCorrectionWidget;
    lay->addWidget(mWidget);
    mWidget->setAutoCorrection(autoCorrection);
    mWidget->loadConfig();
    connect(this, SIGNAL(okClicked()), this, SLOT(slotSaveSettings()));
}

ConfigureTestDialog::~ConfigureTestDialog()
{
}

void ConfigureTestDialog::slotSaveSettings()
{
    mWidget->writeConfig();
}

TextEditAutoCorrectionWidget::TextEditAutoCorrectionWidget(MessageComposer::ComposerAutoCorrection *autoCorrection, QWidget *parent)
    : QTextEdit(parent),
      mAutoCorrection(autoCorrection)
{
    setAcceptRichText(false);
}

TextEditAutoCorrectionWidget::~TextEditAutoCorrectionWidget()
{
}

void TextEditAutoCorrectionWidget::keyPressEvent ( QKeyEvent *e )
{
    if((e->key() == Qt::Key_Space) || (e->key() == Qt::Key_Enter) || (e->key() == Qt::Key_Return)) {
        if(mAutoCorrection && mAutoCorrection->isEnabledAutoCorrection()) {
            mAutoCorrection->autocorrect(acceptRichText(), *document(),textCursor().position());
        }
    }
    QTextEdit::keyPressEvent( e );
}

AutocorrectionTestWidget::AutocorrectionTestWidget(QWidget *parent)
    : QWidget(parent)
{
    mConfig = KSharedConfig::openConfig( QLatin1String("autocorrectionguirc") );
    MessageComposer::MessageComposerSettings::self()->setSharedConfig( mConfig );
    MessageComposer::MessageComposerSettings::self()->readConfig();

    mAutoCorrection = new MessageComposer::ComposerAutoCorrection;
    QVBoxLayout *lay = new QVBoxLayout;
    QToolBar *bar = new QToolBar;
    lay->addWidget(bar);
    bar->addAction(QLatin1String("Configure..."), this, SLOT(slotConfigure()));
    QAction *richText = new QAction(QLatin1String("HTML mode"), this);
    richText->setCheckable(true);
    connect(richText, SIGNAL(toggled(bool)), this, SLOT(slotChangeMode(bool)));
    bar->addAction(richText);
    mEdit = new TextEditAutoCorrectionWidget(mAutoCorrection);
    lay->addWidget(mEdit);


    setLayout(lay);
}

AutocorrectionTestWidget::~AutocorrectionTestWidget()
{
    mAutoCorrection->writeConfig();
    delete mAutoCorrection;
}

void AutocorrectionTestWidget::slotChangeMode(bool mode)
{
    mEdit->setAcceptRichText(mode);
}

void AutocorrectionTestWidget::slotConfigure()
{
    QPointer<ConfigureTestDialog> dlg = new ConfigureTestDialog(mAutoCorrection, this);
    if(dlg->exec())
        MessageComposer::MessageComposerSettings::self()->writeConfig();
    delete dlg;
}

int main (int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, "autocorrectiontest_gui", 0, ki18n("AutoCorrectionTest_Gui"),
                       "1.0", ki18n("Test for autocorrection widget"));
    KApplication app;

    AutocorrectionTestWidget *w = new AutocorrectionTestWidget();
    w->resize(800,600);

    w->show();
    app.exec();
    delete w;
    return 0;
}

