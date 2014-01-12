/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "dropboxutil.h"
#include <qjson/parser.h>

QStringList PimCommon::DropBoxUtil::parseListFolder(const QString &data)
{
    QJson::Parser parser;
    bool ok;
    QStringList listFolder;
    QMap<QString, QVariant> info = parser.parse(data.toUtf8(), &ok).toMap();
    if (info.contains(QLatin1String("contents"))) {
        const QVariantList lst = info.value(QLatin1String("contents")).toList();
        Q_FOREACH (const QVariant &variant, lst) {
            const QVariantMap qwer = variant.toMap();
            const QList<QString> b = qwer.uniqueKeys();
            for(int i=0;i<qwer.size();i++) {
                const QString identifier = b.at(i);
                if((identifier == QLatin1String("is_dir")) || (identifier == QLatin1String("path"))) {
                    if(identifier == QLatin1String("path") && i==4) {
                        const QString name = qwer[b[i]].toString().section(QLatin1Char('/'),-2);
                        listFolder.append(name);
                    }
                }
            }
        }
    }
    return listFolder;
}
