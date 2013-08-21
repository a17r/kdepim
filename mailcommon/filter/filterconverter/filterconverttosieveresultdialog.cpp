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


#include "filterconverttosieveresultdialog.h"
#include "pimcommon/sievehighlighter/sievesyntaxhighlighter.h"
#include "pimcommon/sievehighlighter/sievesyntaxhighlighterutil.h"

#include <KTextEdit>
#include <KLocale>
#include <KFileDialog>
#include <KMessageBox>

#include <QFile>
#include <QHBoxLayout>
#include <QTextStream>
#include <QPointer>

#include <errno.h>

using namespace MailCommon;

FilterConvertToSieveResultDialog::FilterConvertToSieveResultDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Convert to sieve script" ) );
    setButtons( User1|Ok|Cancel );
    setButtonText(User1, i18n("Save..."));
    setDefaultButton( Ok );
    setModal( true );
    connect(this, SIGNAL(user1Clicked()), SLOT(slotSave()));

    QWidget *mainWidget = new QWidget( this );
    QHBoxLayout *mainLayout = new QHBoxLayout( mainWidget );

    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );
    mEditor = new KTextEdit;
    mSyntaxHighlighter = new PimCommon::SieveSyntaxHighlighter( mEditor->document() );
    mSyntaxHighlighter->addCapabilities(PimCommon::SieveSyntaxHighlighterUtil::fullCapabilities());
    mEditor->setAcceptRichText(false);
    mainLayout->addWidget(mEditor);
    setMainWidget( mainWidget );
    readConfig();
}

FilterConvertToSieveResultDialog::~FilterConvertToSieveResultDialog()
{
    writeConfig();
}

void FilterConvertToSieveResultDialog::slotSave()
{
    KUrl url;
    const QString filter = i18n( "*.siv|sieve files (*.siv)\n*|all files (*)" );
    QPointer<KFileDialog> fdlg( new KFileDialog( url, filter, this) );

    fdlg->setMode( KFile::File );
    fdlg->setOperationMode( KFileDialog::Saving );
    fdlg->setConfirmOverwrite(true);
    if ( fdlg->exec() == QDialog::Accepted && fdlg ) {
        const QString fileName = fdlg->selectedFile();
        if ( !saveToFile( fileName ) ) {
            KMessageBox::error( this,
                                i18n( "Could not write the file %1:\n"
                                      "\"%2\" is the detailed error description.",
                                      fileName,
                                      QString::fromLocal8Bit( strerror( errno ) ) ),
                                i18n( "Sieve Editor Error" ) );
        }
    }
    delete fdlg;
}

bool FilterConvertToSieveResultDialog::saveToFile( const QString &filename )
{
    QFile file( filename );
    if ( !file.open( QIODevice::WriteOnly|QIODevice::Text ) )
        return false;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << mEditor->toPlainText();
    return true;
}


void FilterConvertToSieveResultDialog::setCode(const QString &code)
{
    mEditor->setPlainText(code);
}

static const char *myConfigGroupName = "FilterConvertToSieveResultDialog";

void FilterConvertToSieveResultDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), myConfigGroupName );

    const QSize size = group.readEntry( "Size", QSize() );
    if ( size.isValid() ) {
        resize( size );
    } else {
        resize( 500, 300 );
    }
}

void FilterConvertToSieveResultDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), myConfigGroupName );
    group.writeEntry( "Size", size() );
    group.sync();
}


#include "filterconverttosieveresultdialog.moc"
