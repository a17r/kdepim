/*
   Copyright (C) 2015-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "articleviewerwebenginepage.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
using namespace Akregator;

ArticleViewerWebEnginePage::ArticleViewerWebEnginePage(QWebEngineProfile *profile, QObject *parent)
    : WebEngineViewer::WebEnginePage(profile, parent)
{
    settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, false);
#endif
}

ArticleViewerWebEnginePage::~ArticleViewerWebEnginePage()
{

}

bool ArticleViewerWebEnginePage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type);
    if (isMainFrame && type == NavigationTypeLinkClicked) {
        Q_EMIT urlClicked(url);
        return false;
    }
    return true;
}
