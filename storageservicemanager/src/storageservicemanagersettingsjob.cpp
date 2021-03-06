/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

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

#include "storageservicemanagersettingsjob.h"

using namespace PimCommon;

StorageServiceManagerSettingsJob::StorageServiceManagerSettingsJob()
{
}

StorageServiceManagerSettingsJob::~StorageServiceManagerSettingsJob()
{

}

QString StorageServiceManagerSettingsJob::youSendItApiKey() const
{
    //TODO customize it
    return QStringLiteral("fnab8fkgwrka7v6zs2ycd34a");
}

QString StorageServiceManagerSettingsJob::dropboxOauthConsumerKey() const
{
    //TODO customize it
    return QStringLiteral("e40dvomckrm48ci");
}

QString StorageServiceManagerSettingsJob::dropboxOauthSignature() const
{
    //TODO customize it
    return QStringLiteral("0icikya464lny9g&");
}

QString StorageServiceManagerSettingsJob::boxClientId() const
{
    //StorageServiceManager
    return QStringLiteral("00g0i42i7qwfg1honq4tvwdek1rqx3jl");
}

QString StorageServiceManagerSettingsJob::boxClientSecret() const
{
    //StorageServiceManager
    return QStringLiteral("sO5ce1jU7hQZaqWCcIUCLy4mfWEEsqtq");
}

QString StorageServiceManagerSettingsJob::hubicClientId() const
{
    return QStringLiteral("api_hubic_zBKQ6UDUj2vDT7ciDsgjmXA78OVDnzJi");
}

QString StorageServiceManagerSettingsJob::hubicClientSecret() const
{
    return QStringLiteral("pkChgk2sRrrCEoVHmYYCglEI9E2Y2833Te5Vn8n2J6qPdxLU6K8NPUvzo1mEhyzf");
}

QString StorageServiceManagerSettingsJob::dropboxRootPath() const
{
    return QStringLiteral("dropbox");
}

QString StorageServiceManagerSettingsJob::oauth2RedirectUrl() const
{
    return QStringLiteral("https://bugs.kde.org/");
}

QString StorageServiceManagerSettingsJob::hubicScope() const
{
    return QStringLiteral("usage.r,account.r,credentials.r,links.wd");
}

QString StorageServiceManagerSettingsJob::gdriveClientId() const
{
    return QStringLiteral("735222197981-mrcgtaqf05914buqjkts7mk79blsquas.apps.googleusercontent.com");
    //return QStringLiteral("76182239499-2krm3lvlrqrj446loaqrrep594n3u2o8.apps.googleusercontent.com");
}

QString StorageServiceManagerSettingsJob::gdriveClientSecret() const
{
    return QStringLiteral("4MJOS0u1-_AUEKJ0ObA-j22U");
    //return QStringLiteral("7SinUSCxfbrJYN7az3VvxTJ9");
}

QString StorageServiceManagerSettingsJob::defaultUploadFolder() const
{
    return QString();
}
