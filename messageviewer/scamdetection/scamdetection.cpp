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

#include "scamdetection.h"
#include "scamdetectiondetailsdialog.h"
#include "globalsettings.h"
#include <QWebElement>
#include <QDebug>

using namespace MessageViewer;
static QString IPv4_PATTERN = QLatin1String("[0-9]{1,3}\\.[0-9]{1,3}(?:\\.[0-9]{0,3})?(?:\\.[0-9]{0,3})?");

static QString addWarningColor(const QString &url)
{
    const QString error = QString::fromLatin1("<font color=#FF0000>%1</font>").arg(url);
    return error;
}

ScamDetection::ScamDetection(QObject *parent)
    : QObject(parent)
{
}

ScamDetection::~ScamDetection()
{
}

void ScamDetection::scanPage(const QWebElement &rootElement)
{
    if (GlobalSettings::self()->scamDetectionEnabled()) {
        mDetails.clear();
        mDetails = QLatin1String("<b>") + i18n("Details:") + QLatin1String("</b><ul>");
        bool foundScam = false;
        QRegExp ip4regExp;
        ip4regExp.setPattern(IPv4_PATTERN);
        const QWebElementCollection allAnchor = rootElement.findAll(QLatin1String("a"));
        Q_FOREACH (const QWebElement &anchorElement, allAnchor) {
            //1) detect if title has a url and title != href
            const QString href = anchorElement.attribute(QLatin1String("href"));
            const QString title = anchorElement.attribute(QLatin1String("title"));
            if (!title.isEmpty()) {
                if (title.startsWith(QLatin1String("http:"))
                        || title.startsWith(QLatin1String("https:"))
                        || title.startsWith(QLatin1String("www."))) {
                    if (href != title) {
                        foundScam = true;
                        mDetails += QLatin1String("<li>") + i18n("This email contains a link which reads as '%1' in the text, but actually points to '%2'. This is often the case in scam emails to mislead the recipient", addWarningColor(title), addWarningColor(href)) + QLatin1String("</li>");
                    }
                }
            }
            //2) detect if url href has ip and not server name.
            const QUrl url(href);
            const QString hostname = url.host();
            const QString path = url.path();
            if (hostname.contains(ip4regExp)) {
                mDetails += QLatin1String("<li>") + i18n("This email contains a link which points to a numerical IP address (%1) instead of a typical textual website address. This is often the case in scam emails.", addWarningColor(hostname))+QLatin1String("</li>");
                foundScam = true;
            } else if (hostname.contains(QLatin1Char('%'))) { //Hexa value for ip
                mDetails += QLatin1String("<li>") + i18n("This email contains a link which points to a hexadecimal IP address (%1) instead of a typical textual website address. This is often the case in scam emails.", addWarningColor(hostname))+QLatin1String("</li>");
                foundScam = true;
            } else if (path.contains(QLatin1String("url?q="))) { //4) redirect url.
                mDetails += QLatin1String("<li>") + i18n("This email contains a link (%1) which has a redirection", addWarningColor(path)) +QLatin1String("</li>");
                foundScam = true;
            } else if ((path.count(QLatin1String("http://")) > 1) ||
                       (path.count(QLatin1String("https://")) > 1)) { //5) more that 1 http in url.
                mDetails += QLatin1String("<li>") + i18n("This email contains a link (%1) which contains multiple http://. This is often the case in scam emails.", addWarningColor(path)) + QLatin1String("</li>");
                foundScam = true;
            }
        }
        //3) has form
        if (rootElement.findAll(QLatin1String("form")).count() > 0) {
            mDetails += QLatin1String("<li></b>") + i18n("Message contains form element. This is often the case in scam emails.") + QLatin1String("</b></li>");
            foundScam = true;
        }
        mDetails += QLatin1String("</ul>");
        if (foundScam)
            Q_EMIT messageMayBeAScam();
    }
}

void ScamDetection::showDetails()
{
    MessageViewer::ScamDetectionDetailsDialog *dlg = new MessageViewer::ScamDetectionDetailsDialog;
    dlg->setDetails(mDetails);
    dlg->show();
}


#include "scamdetection.moc"
