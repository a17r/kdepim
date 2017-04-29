/*
  Copyright (C) 2009 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>

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

#ifndef MESSAGECOMPOSER_SIGNJOB_H
#define MESSAGECOMPOSER_SIGNJOB_H


#include "contentjobbase.h"
#include "part/infopart.h"
#include "messagecomposer_export.h"
#include "kleo/enum.h"

#include <kde4_gpgme++/key.h>
#include <vector>

namespace KMime {
class Content;

}

namespace GpgME {
class Error;
}


namespace MessageComposer {

class SignJobPrivate;

/**
  Signs the contents of a message.
  Used as a subjob of CryptoMessage
*/
class MESSAGECOMPOSER_EXPORT SignJob : public ContentJobBase
{
    Q_OBJECT

public:
    explicit SignJob( QObject *parent = 0 );
    virtual ~SignJob();

    void setContent( KMime::Content* content );
    void setCryptoMessageFormat( Kleo::CryptoMessageFormat format);
    void setSigningKeys( std::vector<GpgME::Key>& signers );

    KMime::Content* origContent();

protected Q_SLOTS:
    virtual void process();

private:
    Q_DECLARE_PRIVATE( SignJob )
};

}

#endif
