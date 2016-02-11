/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

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

#include "openurlrequest.h"
#include "webengineframe.h"
#include <QVBoxLayout>
#include <QAction>
#include <KIO/FavIconRequestJob>
using namespace Akregator;

WebEngineFrame::WebEngineFrame(KActionCollection *ac, QWidget *parent)
    : Frame(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    setRemovable(true);
    Akregator::WebViewer *viewer = new Akregator::WebViewer(ac, this);
    mArticleViewerWidgetNg = new Akregator::ArticleViewerWidgetNg(viewer, ac, this);

    connect(mArticleViewerWidgetNg->articleViewerNg(), &ArticleViewerNg::titleChanged, this, &WebEngineFrame::slotTitleChanged);
    connect(mArticleViewerWidgetNg->articleViewerNg(), &ArticleViewerNg::loadProgress, this, &WebEngineFrame::slotProgressChanged);
    connect(mArticleViewerWidgetNg->articleViewerNg(), &ArticleViewerNg::signalOpenUrlRequest, this, &WebEngineFrame::signalOpenUrlRequest);
    connect(mArticleViewerWidgetNg->articleViewerNg(), &ArticleViewerNg::loadStarted, this, &WebEngineFrame::slotLoadStarted);
    connect(mArticleViewerWidgetNg->articleViewerNg(), &ArticleViewerNg::loadFinished, this, &WebEngineFrame::slotLoadFinished);
    connect(mArticleViewerWidgetNg->articleViewerNg(), &ArticleViewerNg::showStatusBarMessage, this, &WebEngineFrame::showStatusBarMessage);
    layout->addWidget(mArticleViewerWidgetNg);
}

WebEngineFrame::~WebEngineFrame()
{
}

void WebEngineFrame::slotLoadFinished()
{
    Q_EMIT signalCompleted(this);
}

void WebEngineFrame::slotLoadStarted()
{
    Q_EMIT signalStarted(this);
}

void WebEngineFrame::slotProgressChanged(int progress)
{
    Q_EMIT signalLoadingProgress(this, progress);
}

void WebEngineFrame::slotTitleChanged(const QString &title)
{
    Q_EMIT signalTitleChanged(this, title);
}

KParts::ReadOnlyPart *WebEngineFrame::part() const
{
    return Q_NULLPTR;
}

QUrl WebEngineFrame::url() const
{
    return mArticleViewerWidgetNg->articleViewerNg()->url();
}

bool WebEngineFrame::openUrl(const OpenUrlRequest &request)
{
    const QUrl url = request.url();
    KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(url);
    connect(job, &KIO::FavIconRequestJob::result, this, [job, this](KJob *) {
        if (!job->error()) {
            Q_EMIT signalIconChanged(this, QIcon(job->iconFile()));
        }
    });

    mArticleViewerWidgetNg->articleViewerNg()->load(url);
    return true;
}

void WebEngineFrame::loadConfig(const KConfigGroup &config, const QString &prefix)
{
    const QString url = config.readEntry(QStringLiteral("url").prepend(prefix), QString());
    const qreal zf = config.readEntry(QStringLiteral("zoom").prepend(prefix), 1.0);
    const bool onlyZoomFont = config.readEntry(QStringLiteral("onlyZoomFont").prepend(prefix), false);
    OpenUrlRequest req(url);
    KParts::OpenUrlArguments args;
    req.setArgs(args);
    openUrl(req);
    mArticleViewerWidgetNg->articleViewerNg()->setZoomFactor(zf);
    mArticleViewerWidgetNg->articleViewerNg()->settings()->setAttribute(QWebSettings::ZoomTextOnly, onlyZoomFont);
}

void WebEngineFrame::saveConfig(KConfigGroup &config, const QString &prefix)
{
    config.writeEntry(QStringLiteral("url").prepend(prefix), url().url());
    config.writeEntry(QStringLiteral("zoom").prepend(prefix), mArticleViewerWidgetNg->articleViewerNg()->zoomFactor());
    config.writeEntry(QStringLiteral("onlyZoomFont").prepend(prefix), mArticleViewerWidgetNg->articleViewerNg()->settings()->testAttribute(QWebSettings::ZoomTextOnly));
}

void WebEngineFrame::slotCopyInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotCopy();
}

void WebEngineFrame::slotPrintInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotPrint();
}

void WebEngineFrame::slotPrintPreviewInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotPrintPreview();
}

void WebEngineFrame::slotFindTextInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->slotFind();
}

void WebEngineFrame::slotTextToSpeechInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->slotSpeakText();
}

void WebEngineFrame::slotZoomChangeInFrame(int frameId, qreal value)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->setZoomFactor(value);
}

void WebEngineFrame::slotZoomTextOnlyInFrame(int frameId, bool textOnlyInFrame)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotZoomTextOnlyInFrame(textOnlyInFrame);
}

void WebEngineFrame::slotHistoryForward()
{
    mArticleViewerWidgetNg->articleViewerNg()->pageAction(QWebPage::Forward)->trigger();
}

void WebEngineFrame::slotHistoryBack()
{
    mArticleViewerWidgetNg->articleViewerNg()->pageAction(QWebPage::Back)->trigger();
}

void WebEngineFrame::slotReload()
{
    mArticleViewerWidgetNg->articleViewerNg()->reload();
}

void WebEngineFrame::slotStop()
{
    mArticleViewerWidgetNg->articleViewerNg()->stop();
}

qreal WebEngineFrame::zoomFactor() const
{
    return mArticleViewerWidgetNg->articleViewerNg()->zoomFactor();
}

bool WebEngineFrame::zoomTextOnlyInFrame() const
{
    return mArticleViewerWidgetNg->articleViewerNg()->zoomTextOnlyInFrame();
}

void WebEngineFrame::slotSaveLinkAsInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotSaveLinkAs();
}

void WebEngineFrame::slotCopyLinkAsInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotCopyLinkAddress();
}

void WebEngineFrame::slotSaveImageOnDiskInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotSaveImageOnDiskInFrame();
}

void WebEngineFrame::slotCopyImageLocationInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotCopyImageLocationInFrame();
}

void WebEngineFrame::slotBlockImageInFrame(int frameId)
{
    if (frameId != id()) {
        return;
    }
    mArticleViewerWidgetNg->articleViewerNg()->slotBlockImage();
}
