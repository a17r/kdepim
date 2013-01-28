/*
  Copyright (c) 2012-2013 Montel Laurent <montel.org>

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

#include "composerview.h"
#include "composereditorutils_p.h"

#include "dialog/composerlinkdialog.h"
#include "dialog/composerlistdialog.h"
#include "image/composerimagedialog.h"
#include "image/composerimageresizewidget.h"
#include "pagecolor/pagecolorbackgrounddialog.h"
#include "table/composertabledialog.h"
#include "table/composertableformatdialog.h"
#include "table/composertablecellformatdialog.h"
#include "table/composertableactionmenu.h"
#include "globalsettings_base.h"

#include <kpimtextedit/emoticontexteditaction.h>
#include <kpimtextedit/inserthtmldialog.h>

#include <Sonnet/Dialog>
#include <sonnet/backgroundchecker.h>


#include <KMenu>
#include <KMessageBox>
#include <KToolInvocation>
#include <KLocale>
#include <KAction>
#include <KFontAction>
#include <KToggleAction>
#include <KAction>
#include <KSelectAction>
#include <KActionCollection>
#include <KColorDialog>
#include <KMessageBox>
#include <KStandardDirs>
#include <KDebug>
#include <KFontAction>
#include <KMenu>
#include <KFileDialog>
#include <KPrintPreview>
#include <kdeprintdialog.h>


#include <QAction>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QWebFrame>
#include <QWebElement>
#include <QContextMenuEvent>
#include <QDebug>
#include <QPointer>
#include <QPrinter>
#include <QPrintDialog>


namespace ComposerEditorNG {

#define FORWARD_ACTION(action1, action2) \
    connect(action1, SIGNAL(triggered()), d->getAction(action2), SLOT(trigger()));\
    connect(d->getAction(action2), SIGNAL(changed()), SLOT(_k_slotAdjustActions()));

#define FOLLOW_CHECK(a1, a2) a1->setChecked(getAction(a2)->isChecked())


class ComposerViewPrivate
{
public:
    ComposerViewPrivate( ComposerView *qq)
        : action_text_bold(0),
          action_text_italic(0),
          action_text_underline(0),
          action_text_strikeout(0),
          action_align_left(0),
          action_align_center(0),
          action_align_right(0),
          action_align_justify(0),
          action_direction_ltr(0),
          action_direction_rtl(0),
          action_text_subscript(0),
          action_text_superscript(0),
          action_insert_horizontal_rule(0),
          action_list_indent(0),
          action_list_dedent(0),
          action_ordered_list(0),
          action_unordered_list(0),
          action_format_type(0),
          action_font_size(0),
          action_font_family(0),
          action_add_emoticon(0),
          action_insert_html(0),
          action_insert_image(0),
          action_insert_table(0),
          action_text_foreground_color(0),
          action_text_background_color(0),
          action_format_reset(0),
          action_insert_link(0),
          action_spell_check(0),
          action_find(0),
          action_replace(0),
          action_page_color(0),
          action_block_quote(0),
          action_save_as(0),
          action_print(0),
          action_print_preview(0),
          q(qq),
          imageResizeWidget(0)
    {
    }


    QWebHitTestResult contextMenuResult;
    enum FormatType {
        Paragraph,
        Header1,
        Header2,
        Header3,
        Header4,
        Header5,
        Header6,
        Pre,
        Address
    };

    void _k_slotAdjustActions();
    void _k_setFormatType(QAction* action);
    void _k_slotAddEmoticon(const QString&);
    void _k_slotInsertHtml();
    void _k_slotInsertTable();
    void _k_slotAddImage();
    void _k_setTextForegroundColor();
    void _k_setTextBackgroundColor();
    void _k_slotInsertHorizontalRule();
    void _k_insertLink();
    void _k_slotEditLink();
    void _k_setFontSize(int);
    void _k_setFontFamily(const QString&);
    void _k_adjustActions();
    void _k_slotSpellCheck();
    void _k_spellCheckerCorrected(const QString& original, int pos, const QString& replacement);
    void _k_spellCheckerMisspelling(const QString& , int);
    void _k_slotSpellCheckDone(const QString&);
    void _k_slotFind();
    void _k_slotReplace();
    void _k_slotSpeakText();
    void _k_slotDeleteText();
    void _k_slotChangePageColorAndBackground();
    void _k_slotToggleBlockQuote();
    void _k_slotEditImage();
    void _k_slotSaveAs();
    void _k_slotPrint();
    void _k_slotPrintPreview();
    void _k_changeAutoSpellChecking(bool);
    void _k_slotEditList();

    QAction* getAction ( QWebPage::WebAction action ) const;
    QVariant evaluateJavascript(const QString& command);
    void execCommand(const QString &cmd);
    void execCommand(const QString &cmd, const QString &arg);
    bool queryCommandState(const QString &cmd);

    void hideImageResizeWidget();
    void showImageResizeWidget();
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 3, 0)
    bool checkSpellingEnabled();
#endif


    int spellTextSelectionStart;
    int spellTextSelectionEnd;

    QList<KAction*> htmlEditorActionList;
    KToggleAction *action_text_bold;
    KToggleAction *action_text_italic;
    KToggleAction *action_text_underline;
    KToggleAction *action_text_strikeout;
    KToggleAction *action_align_left;
    KToggleAction *action_align_center;
    KToggleAction *action_align_right;
    KToggleAction *action_align_justify;
    KToggleAction *action_direction_ltr;
    KToggleAction *action_direction_rtl;
    KToggleAction *action_text_subscript;
    KToggleAction *action_text_superscript;
    KAction *action_insert_horizontal_rule;
    KAction *action_list_indent;
    KAction *action_list_dedent;
    KToggleAction *action_ordered_list;
    KToggleAction *action_unordered_list;
    KSelectAction *action_format_type;
    KSelectAction *action_font_size;
    KFontAction *action_font_family;
    KPIMTextEdit::EmoticonTextEditAction *action_add_emoticon;
    KAction *action_insert_html;
    KAction *action_insert_image;
    KAction *action_insert_table;
    KAction *action_text_foreground_color;
    KAction *action_text_background_color;
    KAction *action_format_reset;
    KAction *action_insert_link;
    KAction *action_spell_check;
    KAction *action_find;
    KAction *action_replace;
    KAction *action_page_color;
    KAction *action_block_quote;
    KAction *action_save_as;
    KAction *action_print;
    KAction *action_print_preview;

    ComposerView *q;
    ComposerImageResizeWidget *imageResizeWidget;
};
}

Q_DECLARE_METATYPE(ComposerEditorNG::ComposerViewPrivate::FormatType)

namespace ComposerEditorNG {

#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 3, 0)
bool ComposerViewPrivate::checkSpellingEnabled()
{
    return ComposerEditorNG::GlobalSettingsBase::autoSpellChecking();
}
#endif

void ComposerViewPrivate::_k_changeAutoSpellChecking(bool checked)
{
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 3, 0)
    ComposerEditorNG::GlobalSettingsBase::setAutoSpellChecking(checked);
#else
    Q_UNUSED( checked );
#endif
}

QAction* ComposerViewPrivate::getAction ( QWebPage::WebAction action ) const
{
    if ( action >= 0 && action <= 66 )
        return q->page()->action( static_cast<QWebPage::WebAction>( action ));
    else
        return 0;
}

void ComposerViewPrivate::hideImageResizeWidget()
{
    delete imageResizeWidget;
    imageResizeWidget = 0;
}

void ComposerViewPrivate::showImageResizeWidget()
{
    if (!imageResizeWidget) {
        imageResizeWidget = new ComposerImageResizeWidget(contextMenuResult.element(),q);
        imageResizeWidget->move(contextMenuResult.element().geometry().topLeft());
        imageResizeWidget->show();
    }
}


static QVariant execJScript(QWebElement element, const QString& script)
{
    if (element.isNull())
        return QVariant();
    return element.evaluateJavaScript(script);
}

void ComposerViewPrivate::_k_setFormatType(QAction *act)
{
    if (!act) {
        return;
    }
    QString command;
    switch(act->data().value<ComposerEditorNG::ComposerViewPrivate::FormatType>())
    {
    case Paragraph:
        command = QLatin1String("p");
        break;
    case Header1:
        command = QLatin1String("h1");
        break;
    case Header2:
        command = QLatin1String("h2");
        break;
    case Header3:
        command = QLatin1String("h3");
        break;
    case Header4:
        command = QLatin1String("h4");
        break;
    case Header5:
        command = QLatin1String("h5");
        break;
    case Header6:
        command = QLatin1String("h6");
        break;
    case Pre:
        command = QLatin1String("pre");
        break;
    case Address:
        command = QLatin1String("address");
        break;
    }
    execCommand ( QLatin1String("formatBlock"), command );
}

void ComposerViewPrivate::_k_slotToggleBlockQuote()
{
    execCommand( QLatin1String("formatBlock"), QLatin1String("BLOCKQUOTE"));
}

void ComposerViewPrivate::_k_slotAddEmoticon(const QString& emoticon)
{
    execCommand(QLatin1String("insertHTML"), emoticon);
}

void ComposerViewPrivate::_k_slotInsertHtml()
{
    QPointer<KPIMTextEdit::InsertHtmlDialog> dialog = new KPIMTextEdit::InsertHtmlDialog( q );
    if (dialog->exec()) {
        const QString str = dialog->html().remove(QLatin1String("\n"));
        if (!str.isEmpty()) {
            execCommand(QLatin1String("insertHTML"), str);
        }
    }
    delete dialog;
}

void ComposerViewPrivate::_k_setTextBackgroundColor()
{
    QColor newColor = ComposerEditorNG::Utils::convertRgbToQColor(evaluateJavascript(QLatin1String("getTextBackgroundColor()")).toString());
    const int result = KColorDialog::getColor(newColor,q);
    if (result == QDialog::Accepted) {
        execCommand(QLatin1String("hiliteColor"), newColor.name());
    }
}

QVariant ComposerViewPrivate::evaluateJavascript(const QString& command)
{
    return q->page()->mainFrame()->evaluateJavaScript( command );
}

void ComposerViewPrivate::_k_slotDeleteText()
{
    evaluateJavascript(QLatin1String("setDeleteSelectedText()"));
}

void ComposerViewPrivate::_k_setTextForegroundColor()
{
    QColor newColor = ComposerEditorNG::Utils::convertRgbToQColor(evaluateJavascript(QLatin1String("getTextForegroundColor()")).toString());
    const int result = KColorDialog::getColor(newColor,q);
    if (result == QDialog::Accepted) {
        execCommand(QLatin1String("foreColor"), newColor.name());
    }
}

void ComposerViewPrivate::_k_slotAddImage()
{
    QPointer<ComposerImageDialog> dlg = new ComposerImageDialog( q );
    if (dlg->exec() == KDialog::Accepted) {
        execCommand(QLatin1String("insertHTML"), dlg->html());
    }
    delete dlg;
}

void ComposerViewPrivate::_k_slotEditImage()
{
    showImageResizeWidget();
    ComposerImageDialog dlg( contextMenuResult.element(),q );
    dlg.exec();
}

void ComposerViewPrivate::_k_slotInsertTable()
{
    QPointer<ComposerTableDialog> dlg = new ComposerTableDialog( q );
    if (dlg->exec() == KDialog::Accepted) {
        execCommand(QLatin1String("insertHTML"), dlg->html());
    }
    delete dlg;
}

void ComposerViewPrivate::_k_slotInsertHorizontalRule()
{
    execCommand(QLatin1String("insertHTML"), QLatin1String("<hr>"));
}

void ComposerViewPrivate::_k_insertLink()
{
    const QString selectedText = q->selectedText();
    QPointer<ComposerEditorNG::ComposerLinkDialog> dlg = new ComposerEditorNG::ComposerLinkDialog( selectedText, q );
    if (dlg->exec() == KDialog::Accepted) {
        const QString html(dlg->html());
        if (!html.isEmpty())
            execCommand ( QLatin1String("insertHTML"), html );
    }
    delete dlg;
}

void ComposerViewPrivate::_k_slotEditLink()
{
    ComposerEditorNG::ComposerLinkDialog dlg( contextMenuResult.linkElement(), q );
    dlg.exec();
}

void ComposerViewPrivate::_k_setFontSize(int fontSize)
{
    execCommand(QLatin1String("fontSize"), QString::number(fontSize+1)); //Verify
}

void ComposerViewPrivate::_k_setFontFamily(const QString& family)
{
    execCommand(QLatin1String("fontName"), family);
}

void ComposerViewPrivate::_k_slotSpellCheck()
{
    QString text(execJScript(contextMenuResult.element(), QLatin1String("this.value")).toString());
    if (contextMenuResult.isContentSelected()) {
        spellTextSelectionStart = qMax(0, execJScript(contextMenuResult.element(), QLatin1String("this.selectionStart")).toInt());
        spellTextSelectionEnd = qMax(0, execJScript(contextMenuResult.element(), QLatin1String("this.selectionEnd")).toInt());
        text = text.mid(spellTextSelectionStart, (spellTextSelectionEnd - spellTextSelectionStart));
    } else {
        spellTextSelectionStart = 0;
        spellTextSelectionEnd = 0;
    }

    if (text.isEmpty())
        return;

    Sonnet::BackgroundChecker *backgroundSpellCheck = new Sonnet::BackgroundChecker;
    Sonnet::Dialog* spellDialog = new Sonnet::Dialog(backgroundSpellCheck, q);
    backgroundSpellCheck->setParent(spellDialog);
    spellDialog->setAttribute(Qt::WA_DeleteOnClose, true);

    spellDialog->showSpellCheckCompletionMessage(true);
    q->connect(spellDialog, SIGNAL(replace(QString,int,QString)), q, SLOT(_k_spellCheckerCorrected(QString,int,QString)));
    q->connect(spellDialog, SIGNAL(misspelling(QString,int)), q, SLOT(_k_spellCheckerMisspelling(QString,int)));
    if (contextMenuResult.isContentSelected())
        q->connect(spellDialog, SIGNAL(done(QString)), q, SLOT(_k_slotSpellCheckDone(QString)));
    spellDialog->setBuffer(text);
    spellDialog->show();
}

void ComposerViewPrivate::_k_spellCheckerCorrected(const QString& original, int pos, const QString& replacement)
{
    // Adjust the selection end...
    if (spellTextSelectionEnd > 0)
        spellTextSelectionEnd += qMax(0, (replacement.length() - original.length()));

    const int index = pos + spellTextSelectionStart;
    QString script(QLatin1String("this.value=this.value.substring(0,"));
    script += QString::number(index);
    script += QLatin1String(") + \"");
    script +=  replacement;
    script += QLatin1String("\" + this.value.substring(");
    script += QString::number(index + original.length());
    script += QLatin1String(")");

    //kDebug() << "**** script:" << script;
    execJScript(contextMenuResult.element(), script);
}

void ComposerViewPrivate::_k_spellCheckerMisspelling(const QString& text, int pos)
{
    // kDebug() << text << pos;
    QString selectionScript(QLatin1String("this.setSelectionRange("));
    selectionScript += QString::number(pos + spellTextSelectionStart);
    selectionScript += QLatin1Char(',');
    selectionScript += QString::number(pos + text.length() + spellTextSelectionStart);
    selectionScript += QLatin1Char(')');
    execJScript(contextMenuResult.element(), selectionScript);
}

void ComposerViewPrivate::_k_slotSpellCheckDone(const QString&)
{
    // Restore the text selection if one was present before we started the
    // spell check.
    if (spellTextSelectionStart > 0 || spellTextSelectionEnd > 0) {
        QString script(QLatin1String("; this.setSelectionRange("));
        script += QString::number(spellTextSelectionStart);
        script += QLatin1Char(',');
        script += QString::number(spellTextSelectionEnd);
        script += QLatin1Char(')');
        execJScript(contextMenuResult.element(), script);
    }
}

void ComposerViewPrivate::_k_slotFind()
{
    Q_EMIT q->showFindBar();
}

void ComposerViewPrivate::_k_slotReplace()
{
    //TODO
}

void ComposerViewPrivate::_k_slotSaveAs()
{
    QString fn = KFileDialog::getSaveFileName(QString(), i18n("HTML-Files (*.htm *.html);;All Files (*)") , q, i18n("Save as..."));
    //TODO add KMessageBox
    if (fn.isEmpty())
        return;
    if (!(fn.endsWith(QLatin1String(".htm"), Qt::CaseInsensitive) ||
          fn.endsWith(QLatin1String(".html"), Qt::CaseInsensitive))) {
        fn += QLatin1String(".htm");
    }
    QFile file(fn);
    bool success = file.open(QIODevice::WriteOnly);
    if (success) {
        // FIXME: here we always use UTF-8 encoding
        const QString content = q->page()->mainFrame()->toHtml();
        QByteArray data = content.toUtf8();
        const qint64 c = file.write(data);
        success = (c >= data.length());
    }
}

void ComposerViewPrivate::_k_slotPrint()
{
    QPrinter printer;
    QPointer<QPrintDialog> dlg(KdePrint::createPrintDialog(&printer));

    if ( dlg->exec() == QDialog::Accepted ) {
        q->print( &printer );
    }
    delete dlg;
}

void ComposerViewPrivate::_k_slotPrintPreview()
{
    QPrinter printer;
    KPrintPreview previewdlg( &printer, q );
    q->print( &printer );
    previewdlg.exec();
}

void ComposerViewPrivate::_k_slotChangePageColorAndBackground()
{
    const QWebElement element = q->page()->mainFrame()->findFirstElement(QLatin1String("body"));
    if (!element.isNull()) {
        QPointer<PageColorBackgroundDialog> dlg = new PageColorBackgroundDialog(element, q);
        dlg->exec();
        delete dlg;
    }
}

void ComposerViewPrivate::_k_slotEditList()
{
    QPointer<ComposerListDialog> dlg = new ComposerListDialog(contextMenuResult.element(),q);
    if (dlg->exec()) {
        //TODO
    }
}

void ComposerViewPrivate::_k_slotAdjustActions()
{
    if (action_text_bold)
        FOLLOW_CHECK(action_text_bold, QWebPage::ToggleBold);
    if (action_text_italic)
        FOLLOW_CHECK(action_text_italic, QWebPage::ToggleItalic);
    if (action_text_strikeout)
        FOLLOW_CHECK(action_text_strikeout, QWebPage::ToggleStrikethrough);
    if (action_text_underline)
        FOLLOW_CHECK(action_text_underline, QWebPage::ToggleUnderline);
    if (action_text_subscript)
        FOLLOW_CHECK(action_text_subscript, QWebPage::ToggleSubscript);
    if (action_text_superscript)
        FOLLOW_CHECK(action_text_superscript, QWebPage::ToggleSuperscript);
    if (action_ordered_list)
        FOLLOW_CHECK(action_ordered_list, QWebPage::InsertOrderedList);
    if (action_unordered_list)
        FOLLOW_CHECK(action_unordered_list, QWebPage::InsertUnorderedList);
    if (action_direction_ltr)
        FOLLOW_CHECK(action_direction_ltr, QWebPage::SetTextDirectionLeftToRight);
    if (action_direction_rtl)
        FOLLOW_CHECK(action_direction_rtl, QWebPage::SetTextDirectionRightToLeft);

    const QString alignment = evaluateJavascript(QLatin1String("getAlignment()")).toString();
    if (alignment == QLatin1String("left")) {
        if (action_align_left)
            action_align_left->setChecked(true);
    } else if(alignment == QLatin1String("right")) {
        if (action_align_right)
            action_align_right->setChecked(true);
    } else if(alignment == QLatin1String("center")) {
        if (action_align_center)
            action_align_center->setChecked(true);
    } else if(alignment == QLatin1String("-webkit-auto")) {
        if (action_align_justify)
            action_align_justify->setChecked(true);
    }

    if (action_font_family) {
        const QString font = evaluateJavascript(QLatin1String("getFontFamily()")).toString();
        if(!font.isEmpty()) {
            action_font_family->setFont(font);
        }
    }
}

void ComposerViewPrivate::execCommand(const QString &cmd)
{
    QWebFrame *frame = q->page()->mainFrame();
    const QString js = QString::fromLatin1("document.execCommand(\"%1\", false, null)").arg(cmd);
    frame->evaluateJavaScript(js);
}

void ComposerViewPrivate::execCommand(const QString &cmd, const QString &arg)
{
    QWebFrame *frame = q->page()->mainFrame();
    const QString js = QString::fromLatin1("document.execCommand(\"%1\", false, \"%2\")").arg(cmd).arg(arg);
    frame->evaluateJavaScript(js);
}


bool ComposerViewPrivate::queryCommandState(const QString &cmd)
{
    QWebFrame *frame = q->page()->mainFrame();
    QString js = QString::fromLatin1("document.queryCommandState(\"%1\", false, null)").arg(cmd);
    const QVariant result = frame->evaluateJavaScript(js);
    return result.toString().simplified().toLower() == QLatin1String("true");
}


void ComposerViewPrivate::_k_slotSpeakText()
{
    // If KTTSD not running, start it.
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String("org.kde.kttsd"))) {
        QString error;
        if (KToolInvocation::startServiceByDesktopName(QLatin1String("kttsd"), QStringList(), &error)) {
            KMessageBox::error(q, i18n( "Starting Jovie Text-to-Speech Service Failed"), error );
            return;
        }
    }
    QDBusInterface ktts(QLatin1String("org.kde.kttsd"), QLatin1String("/KSpeech"), QLatin1String("org.kde.KSpeech"));

    QString text = q->selectedText();
    if(text.isEmpty())
        text = q->page()->mainFrame()->toPlainText();

    ktts.asyncCall(QLatin1String("say"), text, 0);
}

ComposerView::ComposerView(QWidget *parent)
    : KWebView(parent),d(new ComposerViewPrivate(this))
{
    QFile file ( KStandardDirs::locate ( "data", QLatin1String("composereditor/composereditorinitialhtml") ) );
    kDebug() <<file.fileName();

    if ( !file.open ( QIODevice::ReadOnly ) )
        KMessageBox::error(this, i18n ( "Cannot open template file." ), i18n ( "composer editor" ));
    else
        setContent ( file.readAll());//, "application/xhtml+xml" );

    page()->setContentEditable(true);
    page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(this, SIGNAL(linkClicked(QUrl)), SIGNAL(openLink(QUrl)));
    connect(page(), SIGNAL(selectionChanged()), this, SLOT(_k_slotAdjustActions()) );

    setWindowModified(false);

}

ComposerView::~ComposerView()
{
    delete d;
}

void ComposerView::createActions(const QList<ComposerViewAction>& lstActions)
{
    Q_FOREACH(ComposerViewAction action, lstActions) {
        createAction( action );
    }
}

void ComposerView::createAllActions()
{
    for ( uint i = 0; i < LastType; ++i ) {
      createAction( (ComposerViewAction)i );
    }
}

void ComposerView::createAction(ComposerViewAction type)
{
    switch(type) {
    case Bold: {
        if (!d->action_text_bold) {
            d->action_text_bold = new KToggleAction(KIcon(QLatin1String("format-text-bold")), i18nc("@action boldify selected text", "&Bold"), this);
            QFont bold;
            bold.setBold(true);
            d->action_text_bold->setFont(bold);
            d->action_text_bold->setShortcut(KShortcut(Qt::CTRL + Qt::Key_B));
            FORWARD_ACTION(d->action_text_bold, QWebPage::ToggleBold);
            d->htmlEditorActionList.append(d->action_text_bold);
        }
        break;
    }
    case Italic: {
        if (!d->action_text_italic) {
            d->action_text_italic = new KToggleAction(KIcon(QLatin1String("format-text-italic")), i18nc("@action italicize selected text", "&Italic"), this);
            QFont italic;
            italic.setItalic(true);
            d->action_text_italic->setFont(italic);
            d->action_text_italic->setShortcut(KShortcut(Qt::CTRL + Qt::Key_I));
            FORWARD_ACTION(d->action_text_italic, QWebPage::ToggleItalic);
            d->htmlEditorActionList.append(d->action_text_italic);
        }
        break;
    }
    case Underline: {
        if (!d->action_text_underline) {
            d->action_text_underline = new KToggleAction(KIcon(QLatin1String("format-text-underline")), i18nc("@action underline selected text", "&Underline"), this);
            QFont underline;
            underline.setUnderline(true);
            d->action_text_underline->setFont(underline);
            d->action_text_underline->setShortcut(KShortcut(Qt::CTRL + Qt::Key_U));
            FORWARD_ACTION(d->action_text_underline, QWebPage::ToggleUnderline);
            d->htmlEditorActionList.append(d->action_text_underline);
        }
        break;
    }
    case StrikeOut: {
        if (!d->action_text_strikeout) {
            d->action_text_strikeout = new KToggleAction(KIcon(QLatin1String("format-text-strikethrough")), i18nc("@action", "&Strike Out"), this);
            d->action_text_strikeout->setShortcut(KShortcut(Qt::CTRL + Qt::Key_L));
            FORWARD_ACTION(d->action_text_strikeout, QWebPage::ToggleStrikethrough);
            d->htmlEditorActionList.append(d->action_text_strikeout);
        }
        break;
    }
    case AlignLeft: {
        if (!d->action_align_left) {
            d->action_align_left = new KToggleAction(KIcon(QLatin1String("format-justify-left")), i18nc("@action", "Align &Left"), this);
            d->action_align_left->setIconText(i18nc("@label left justify", "Left"));
            d->htmlEditorActionList.append((d->action_align_left));
            FORWARD_ACTION(d->action_align_left, QWebPage::AlignLeft);
        }

        break;
    }
    case AlignCenter: {
        if (!d->action_align_center) {
            d->action_align_center = new KToggleAction(KIcon(QLatin1String("format-justify-center")), i18nc("@action", "Align &Center"), this);
            d->action_align_center->setIconText(i18nc("@label center justify", "Center"));
            d->htmlEditorActionList.append((d->action_align_center));
            FORWARD_ACTION(d->action_align_center, QWebPage::AlignCenter);
        }
        break;
    }
    case AlignRight: {
        if (!d->action_align_right) {
            d->action_align_right = new KToggleAction(KIcon(QLatin1String("format-justify-right")), i18nc("@action", "Align &Right"), this);
            d->action_align_right->setIconText(i18nc("@label right justify", "Right"));
            d->htmlEditorActionList.append((d->action_align_right));
            FORWARD_ACTION(d->action_align_right, QWebPage::AlignRight);
        }
        break;
    }
    case AlignJustify:
    {
        if (!d->action_align_justify) {
            d->action_align_justify = new KToggleAction(KIcon(QLatin1String("format-justify-fill")), i18nc("@action", "&Justify"), this);
            d->action_align_justify->setIconText(i18nc("@label justify fill", "Justify"));
            d->htmlEditorActionList.append((d->action_align_justify));
            FORWARD_ACTION(d->action_align_justify, QWebPage::AlignJustified);
        }
        break;
    }
    case DirectionLtr:
    {
        if (!d->action_direction_ltr) {
            d->action_direction_ltr = new KToggleAction(KIcon(QLatin1String("format-text-direction-ltr")), i18nc("@action", "Left-to-Right"), this);
            d->action_direction_ltr->setIconText(i18nc("@label left-to-right", "Left-to-Right"));
            d->htmlEditorActionList.append(d->action_direction_ltr);
            FORWARD_ACTION(d->action_direction_ltr, QWebPage::SetTextDirectionLeftToRight);
        }
        break;
    }
    case DirectionRtl:
    {
        if (!d->action_direction_rtl) {
            d->action_direction_rtl = new KToggleAction(KIcon(QLatin1String("format-text-direction-rtl")), i18nc("@action", "Right-to-Left"), this);
            d->action_direction_rtl->setIconText(i18nc("@label right-to-left", "Right-to-Left"));
            d->htmlEditorActionList.append(d->action_direction_rtl);
            FORWARD_ACTION(d->action_direction_ltr, QWebPage::SetTextDirectionRightToLeft);
        }
        break;
    }
    case SubScript:
    {
        if (!d->action_text_subscript) {
            d->action_text_subscript = new KToggleAction(KIcon(QLatin1String("format-text-subscript")), i18nc("@action", "Subscript"), this);
            d->htmlEditorActionList.append((d->action_text_subscript));
            FORWARD_ACTION(d->action_text_subscript, QWebPage::ToggleSubscript);
        }
        break;
    }
    case SuperScript:
    {
        if (!d->action_text_superscript) {
            d->action_text_superscript = new KToggleAction(KIcon(QLatin1String("format-text-superscript")), i18nc("@action", "Superscript"), this);
            d->htmlEditorActionList.append((d->action_text_superscript));
            FORWARD_ACTION(d->action_text_superscript, QWebPage::ToggleSuperscript);
        }
        break;
    }
    case HorizontalRule:
    {
        if (!d->action_insert_horizontal_rule) {
            //horizontal line
            d->action_insert_horizontal_rule = new KAction(KIcon(QLatin1String("insert-horizontal-rule")), i18nc("@action", "Insert Rule Line"), this);
            d->htmlEditorActionList.append((d->action_insert_horizontal_rule));
            connect( d->action_insert_horizontal_rule, SIGNAL(triggered(bool)), SLOT(_k_slotInsertHorizontalRule()) );
        }
        break;
    }
    case ListIndent:
    {
        if (!d->action_list_indent) {
            d->action_list_indent = new KAction(KIcon(QLatin1String("format-indent-more")), i18nc("@action", "Increase Indent"), this);
            d->htmlEditorActionList.append((d->action_list_indent));
            FORWARD_ACTION(d->action_list_indent, QWebPage::Indent);
        }
        break;
    }
    case ListDedent:
    {
        if (!d->action_list_dedent) {
            d->action_list_dedent = new KAction(KIcon(QLatin1String("format-indent-less")), i18nc("@action", "Decrease Indent"), this);
            d->htmlEditorActionList.append(d->action_list_dedent);
            FORWARD_ACTION(d->action_list_dedent, QWebPage::Outdent);
        }
        break;
    }
    case OrderedList:
    {
        if (!d->action_ordered_list) {
            d->action_ordered_list = new KToggleAction(KIcon(QLatin1String("format-list-ordered")), i18n("Ordered Style"), this);
            d->htmlEditorActionList.append(d->action_ordered_list);
            FORWARD_ACTION(d->action_ordered_list, QWebPage::InsertOrderedList);
        }
        break;
    }
    case UnorderedList:
    {
        if (!d->action_unordered_list) {
            d->action_unordered_list = new KToggleAction( KIcon( QLatin1String("format-list-unordered" )), i18n( "Unordered List" ), this);
            d->htmlEditorActionList.append(d->action_unordered_list);
            FORWARD_ACTION(d->action_unordered_list, QWebPage::InsertUnorderedList);
        }
        break;
    }
    case FormatType:
    {
        if (!d->action_format_type) {
            d->action_format_type = new KSelectAction(KIcon(QLatin1String("format-list-unordered")), i18nc("@title:menu", "List Style"), this);
            KAction *act = d->action_format_type->addAction(i18n( "Paragraph" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Paragraph));
            act = d->action_format_type->addAction(i18n( "Heading 1" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Header1));
            act = d->action_format_type->addAction(i18n( "Heading 2" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Header2));
            act = d->action_format_type->addAction(i18n( "Heading 3" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Header3));
            act = d->action_format_type->addAction(i18n( "Heading 4" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Header4));
            act = d->action_format_type->addAction(i18n( "Heading 5" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Header5));
            act = d->action_format_type->addAction(i18n( "Heading 6" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Header6));
            act = d->action_format_type->addAction(i18n( "Pre" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Pre));
            act = d->action_format_type->addAction(i18n( "Address" ));
            act->setData(QVariant::fromValue(ComposerViewPrivate::Address));
            d->action_format_type->setCurrentItem(0);
            d->htmlEditorActionList.append(d->action_format_type);

            connect(d->action_format_type, SIGNAL(triggered(QAction*)),
                    this, SLOT(_k_setFormatType(QAction*)));
        }
        break;
    }
    case FontSize:
    {
        if (!d->action_font_size) {
            d->action_font_size = new KSelectAction(i18nc("@action", "Font &Size"), this);
            d->htmlEditorActionList.append(d->action_font_size);
            QStringList sizes;
            sizes << QLatin1String("xx-small");
            sizes << QLatin1String("x-small");
            sizes << QLatin1String("small");
            sizes << QLatin1String("medium");
            sizes << QLatin1String("large");
            sizes << QLatin1String("x-large");
            sizes << QLatin1String("xx-large");
            d->action_font_size->setItems(sizes);
            d->action_font_size->setCurrentItem(0);
            connect(d->action_font_size, SIGNAL(triggered(int)), this, SLOT(_k_setFontSize(int)));
        }
        break;
    }
    case FontFamily:
    {
        if (!d->action_font_family) {
            d->action_font_family = new KFontAction(i18nc("@action", "&Font"), this);
            d->htmlEditorActionList.append((d->action_font_family));
            connect(d->action_font_family, SIGNAL(triggered(QString)), this, SLOT(_k_setFontFamily(QString)));
        }
        break;
    }
    case Emoticon:
    {
        if (!d->action_add_emoticon) {
            d->action_add_emoticon = new KPIMTextEdit::EmoticonTextEditAction(this);
            connect( d->action_add_emoticon, SIGNAL(emoticonActivated(QString)),
                     this, SLOT(_k_slotAddEmoticon(QString)) );
        }
        break;
    }
    case InsertHtml:
    {
        if (!d->action_insert_image) {
            d->action_insert_image = new KAction( KIcon( QLatin1String( "insert-image" ) ), i18n( "Add Image" ), this);
            connect( d->action_insert_image, SIGNAL(triggered(bool)), SLOT(_k_slotAddImage()) );
        }
        break;
    }
    case InsertImage:
    {
        if (!d->action_insert_html) {
            d->action_insert_html = new KAction( i18n( "Insert HTML" ), this );
            connect( d->action_insert_html, SIGNAL(triggered(bool)), SLOT(_k_slotInsertHtml()) );
        }
        break;
    }
    case InsertTable:
    {
        if (!d->action_insert_table) {
            d->action_insert_table = new KAction( i18n( "Table..." ), this );
            d->htmlEditorActionList.append(d->action_insert_table);
            connect( d->action_insert_table, SIGNAL(triggered(bool)), SLOT(_k_slotInsertTable()) );
        }
        break;
    }
    case InsertLink:
    {
        if (!d->action_insert_link) {
            d->action_insert_link = new KAction(KIcon(QLatin1String("insert-link")), i18nc("@action", "Link"), this);
            d->htmlEditorActionList.append(d->action_insert_link);
            connect(d->action_insert_link, SIGNAL(triggered(bool)), this, SLOT(_k_insertLink()));
        }
        break;
    }
    case TextForegroundColor:
    {
        if (!d->action_text_foreground_color) {
            d->action_text_foreground_color = new KAction(KIcon(QLatin1String("format-stroke-color")), i18nc("@action", "Text &Color..."), this);
            d->action_text_foreground_color->setIconText(i18nc("@label stroke color", "Color"));
            d->htmlEditorActionList.append((d->action_text_foreground_color));
            connect(d->action_text_foreground_color, SIGNAL(triggered()), this, SLOT(_k_setTextForegroundColor()));
        }
        break;
    }
    case TextBackgroundColor:
    {
        if (!d->action_text_background_color) {
            d->action_text_background_color = new KAction(KIcon(QLatin1String("format-fill-color")), i18nc("@action", "Text &Highlight..."), this);
            d->htmlEditorActionList.append((d->action_text_background_color));
            connect(d->action_text_background_color, SIGNAL(triggered()), this, SLOT(_k_setTextBackgroundColor()));
        }
        break;
    }
    case FormatReset:
    {
        if (!d->action_format_reset) {
            d->action_format_reset = new KAction( KIcon( QLatin1String("draw-eraser") ), i18n("Reset Font Settings"), this);
            FORWARD_ACTION(d->action_format_reset, QWebPage::RemoveFormat);
        }
        break;
    }
    case SpellCheck:
    {
        if (!d->action_spell_check) {
            d->action_spell_check = new KAction(KIcon(QLatin1String("tools-check-spelling")), i18n("Check Spelling..."), this);
            d->htmlEditorActionList.append(d->action_spell_check);
            connect(d->action_spell_check, SIGNAL(triggered(bool)), this, SLOT(_k_slotSpellCheck()));
        }
        break;
    }
    case PageColor:
    {
        if (!d->action_page_color) {
            d->action_page_color = new KAction( i18n( "Page Color and Background..." ), this );
            d->htmlEditorActionList.append(d->action_page_color);
            connect( d->action_page_color, SIGNAL(triggered(bool)), SLOT(_k_slotChangePageColorAndBackground()) );
        }
        break;
    }
    case BlockQuote:
    {
        if (!d->action_block_quote) {
            d->action_block_quote = new KAction(KIcon(QLatin1String("format-text-blockquote")), i18n( "Blockquote" ), this );
            d->htmlEditorActionList.append(d->action_block_quote);
            connect( d->action_block_quote, SIGNAL(triggered()), this, SLOT(_k_slotToggleBlockQuote()) );
        }
        break;
    }
    case Find:
    {
        if (!d->action_find) {

        }
        break;
    }
    case Replace:
    {
        if (!d->action_replace) {

        }
        break;
    }
    case SaveAs:
    {
        if (!d->action_save_as) {

        }
        break;
    }
    case Print:
    {
        if (!d->action_print) {

        }
        break;
    }
    case PrintPreview:
    {
        if (!d->action_print_preview) {

        }
        break;
    }
    default:
        break;
    }

    if (d->action_align_left || d->action_align_center || d->action_align_right || d->action_align_justify) {
        QActionGroup *alignmentGroup = new QActionGroup(this);
        alignmentGroup->addAction(d->action_align_left);
        alignmentGroup->addAction(d->action_align_center);
        alignmentGroup->addAction(d->action_align_right);
        alignmentGroup->addAction(d->action_align_justify);
    }

    if (d->action_direction_ltr && d->action_direction_rtl) {
        QActionGroup *directionGroup = new QActionGroup(this);
        directionGroup->addAction(d->action_direction_ltr);
        directionGroup->addAction(d->action_direction_rtl);
    }


}

void ComposerView::addCreatedActionsToActionCollection(KActionCollection *actionCollection)
{
    if (actionCollection) {
        if (d->action_text_bold)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_bold"), d->action_text_bold);
        if (d->action_text_italic)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_italic"), d->action_text_italic);
        if (d->action_text_underline)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_underline"), d->action_text_underline);
        if (d->action_text_strikeout)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_strikeout"), d->action_text_strikeout);
        if (d->action_align_left)
            actionCollection->addAction(QLatin1String("htmleditor_format_align_left"), d->action_align_left);
        if (d->action_align_center)
            actionCollection->addAction(QLatin1String("htmleditor_format_align_center"), d->action_align_center);
        if (d->action_align_right)
            actionCollection->addAction(QLatin1String("htmleditor_format_align_right"), d->action_align_right);
        if (d->action_align_justify)
            actionCollection->addAction(QLatin1String("htmleditor_format_align_justify"), d->action_align_justify);
        if (d->action_direction_ltr)
            actionCollection->addAction(QLatin1String("htmleditor_direction_ltr"), d->action_direction_ltr);
        if (d->action_direction_rtl)
            actionCollection->addAction(QLatin1String("htmleditor_direction_rtl"), d->action_direction_rtl);
        if (d->action_text_subscript)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_subscript"), d->action_text_subscript);
        if (d->action_text_superscript)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_superscript"), d->action_text_superscript);
        if (d->action_page_color)
            actionCollection->addAction( QLatin1String( "htmleditor_page_color_and_background" ), d->action_page_color );
        if (d->action_insert_table)
            actionCollection->addAction( QLatin1String( "htmleditor_insert_new_table" ), d->action_insert_table );
        if (d->action_insert_link)
            actionCollection->addAction(QLatin1String("htmleditor_insert_link"), d->action_insert_link);
        if (d->action_insert_horizontal_rule)
            actionCollection->addAction(QLatin1String("htmleditor_insert_horizontal_rule"), d->action_insert_horizontal_rule);
        if (d->action_text_foreground_color)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_foreground_color"), d->action_text_foreground_color);
        if (d->action_text_background_color)
            actionCollection->addAction(QLatin1String("htmleditor_format_text_background_color"), d->action_text_background_color);
        if (d->action_add_emoticon)
            actionCollection->addAction(QLatin1String("htmleditor_add_emoticon"), d->action_add_emoticon);
        if (d->action_insert_html)
            actionCollection->addAction( QLatin1String( "htmleditor_insert_html" ), d->action_insert_html);
        if (d->action_insert_image)
            actionCollection->addAction( QLatin1String( "htmleditor_add_image" ), d->action_insert_image);
        if (d->action_spell_check)
            actionCollection->addAction(QLatin1String("htmleditor_spell_check"), d->action_spell_check);
        if (d->action_format_reset)
            actionCollection->addAction( QLatin1String("htmleditor_format_reset"), d->action_format_reset);
        if (d->action_font_family)
            actionCollection->addAction(QLatin1String("htmleditor_format_font_family"), d->action_font_family);
        if (d->action_font_size)
            actionCollection->addAction(QLatin1String("htmleditor_format_font_size"), d->action_font_size);
        if (d->action_format_type)
            actionCollection->addAction(QLatin1String("htmleditor_format_type"), d->action_format_type);
        if (d->action_block_quote)
            actionCollection->addAction(QLatin1String("htmleditor_block_quote"), d->action_block_quote);
        if (d->action_ordered_list)
            actionCollection->addAction(QLatin1String("htmleditor_format_list_ordered"), d->action_ordered_list);
        if (d->action_unordered_list)
            actionCollection->addAction(QLatin1String("htmleditor_format_list_unordered"), d->action_unordered_list);
        if (d->action_list_indent)
            actionCollection->addAction(QLatin1String("htmleditor_format_list_indent_more"), d->action_list_indent);
        if (d->action_list_dedent)
            actionCollection->addAction(QLatin1String("htmleditor_format_list_indent_less"), d->action_list_dedent);
    }
}

void ComposerView::createActions(KActionCollection *actionCollection)
{
    Q_ASSERT(actionCollection);
    //Find
    d->action_find = KStandardAction::find(this, SLOT(_k_slotFind()), this);
    d->htmlEditorActionList.append(d->action_find);

    //Replace
    d->action_replace = KStandardAction::replace(this, SLOT(_k_slotReplace()), this);
    d->htmlEditorActionList.append(d->action_replace);

    //Save As
    d->action_save_as = KStandardAction::saveAs(this,SLOT(_k_slotSaveAs()), this);
    d->htmlEditorActionList.append(d->action_save_as);

    //Print
    d->action_print = KStandardAction::print( this, SLOT(_k_slotPrint()), this);
    d->htmlEditorActionList.append(d->action_print);

    d->action_print_preview = KStandardAction::printPreview( this, SLOT(_k_slotPrintPreview()), this );
    d->htmlEditorActionList.append(d->action_print_preview);

}

void ComposerView::contextMenuEvent(QContextMenuEvent* event)
{
    d->hideImageResizeWidget();
    d->contextMenuResult = page()->mainFrame()->hitTestContent(event->pos());

    const bool linkSelected = !d->contextMenuResult.linkElement().isNull();
    const bool imageSelected = !d->contextMenuResult.imageUrl().isEmpty();

    const QWebElement elm = d->contextMenuResult.element();
    const bool tableCellSelected = (elm.tagName().toLower() == QLatin1String("td"));
    const bool tableSelected = (elm.tagName().toLower() == QLatin1String("table") ||
                                tableCellSelected );

    const bool listSelected = (elm.tagName().toLower() == QLatin1String("ol") ||
                               elm.tagName().toLower() == QLatin1String("ul") ||
                               elm.tagName().toLower() == QLatin1String("li") );

    qDebug()<<" elm.tagName().toLower() "<<elm.tagName().toLower();

    KMenu *menu = new KMenu;
    const QString selectedText = page()->mainFrame()->toPlainText().simplified();
    const bool emptyDocument = selectedText.isEmpty();

    menu->addAction(page()->action(QWebPage::Undo));
    menu->addAction(page()->action(QWebPage::Redo));
    menu->addSeparator();
    menu->addAction(page()->action(QWebPage::Cut));
    menu->addAction(page()->action(QWebPage::Copy));
    menu->addAction(page()->action(QWebPage::Paste));
    menu->addSeparator();
    menu->addAction(page()->action(QWebPage::SelectAll));
    menu->addSeparator();
    if (!emptyDocument && d->action_find) {
        menu->addAction(d->action_find);
        menu->addSeparator();
    }
    if (imageSelected) {
        QAction *editImageAction = menu->addAction(i18n("Edit Image..."));
        connect( editImageAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditImage()) );
    } else if (linkSelected) {
        QAction *editLinkAction = menu->addAction(i18n("Edit Link..."));
        connect( editLinkAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditLink()) );
    } else if (tableSelected) {
        ComposerTableActionMenu * tableActionMenu = new ComposerTableActionMenu(elm,menu,this);
        connect(tableActionMenu, SIGNAL(insertNewTable()), this, SLOT(_k_slotInsertTable()));
        menu->addAction(tableActionMenu);
    } else if (listSelected) {
        QAction *editListAction = menu->addAction(i18n("Edit List..."));
        connect( editListAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotEditList()) );
    }
    menu->addSeparator();
    if (!emptyDocument && d->action_spell_check) {
        menu->addAction(d->action_spell_check);
        menu->addSeparator();
    }
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 3, 0)
    QAction *autoSpellCheckingAction = menu->addAction(i18n("Auto Spell Check"));
    autoSpellCheckingAction->setCheckable( true );
    autoSpellCheckingAction->setChecked( d->checkSpellingEnabled() );
    connect( autoSpellCheckingAction, SIGNAL(triggered(bool)), this, SLOT(_k_changeAutoSpellChecking(bool)) );
#endif
    QAction *speakAction = menu->addAction(i18n("Speak Text"));
    speakAction->setIcon(KIcon(QLatin1String("preferences-desktop-text-to-speech")));
    speakAction->setEnabled(!emptyDocument );
    connect( speakAction, SIGNAL(triggered(bool)), this, SLOT(_k_slotSpeakText()) );
    menu->exec(event->globalPos());
    delete menu;
}

void ComposerView::setActionsEnabled(bool enabled)
{
    Q_FOREACH(QAction* action, d->htmlEditorActionList) {
        action->setEnabled(enabled);
    }
}

void ComposerView::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        d->contextMenuResult = page()->mainFrame()->hitTestContent(event->pos());
        const bool imageSelected = !d->contextMenuResult.imageUrl().isEmpty();
        if(imageSelected) {
            d->showImageResizeWidget();
        }
    } else {
        d->hideImageResizeWidget();
    }
    KWebView::mousePressEvent(event);
}

void ComposerView::keyPressEvent(QKeyEvent * event)
{
    d->hideImageResizeWidget();
    KWebView::keyPressEvent(event);
}

void ComposerView::wheelEvent(QWheelEvent * event)
{
    d->hideImageResizeWidget();
    KWebView::wheelEvent(event);
}

void ComposerView::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        d->contextMenuResult = page()->mainFrame()->hitTestContent(event->pos());
        const bool imageSelected = !d->contextMenuResult.imageUrl().isEmpty();
        if(imageSelected) {
            d->showImageResizeWidget();
            d->_k_slotEditImage();
        }
    } else {
        d->hideImageResizeWidget();
    }
    KWebView::mouseDoubleClickEvent(event);
}

void ComposerView::setHtmlContent( const QString& html )
{
    QString text = html;
    text = text.replace(QLatin1Char('\"'), QLatin1String("\\\"")).simplified();
    d->evaluateJavascript(QString::fromLatin1("replaceHtml(\"%1\")").arg(text));
}

void ComposerView::evaluateJavascript( const QString& javascript)
{
    d->evaluateJavascript(javascript);
}

void ComposerView::createToolBar(const QList<ComposerViewAction>&)
{
    //TODO
}

}

#include "composerview.moc"
