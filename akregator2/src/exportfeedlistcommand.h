/*
    This file is part of Akregator2.

    Copyright (C) 2009 Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef AKREGATOR2_EXPORTFEEDLISTCOMMAND_H
#define AKREGATOR2_EXPORTFEEDLISTCOMMAND_H

#include "command.h"

#include <Akonadi/Collection>

class KUrl;

namespace Akonadi {
    class Session;
    class Collection;
}
namespace Akregator2 {

class ExportFeedListCommand : public Command
{
    Q_OBJECT
public:
    explicit ExportFeedListCommand( QObject* parent = 0 );
    ~ExportFeedListCommand();

    void setResource( const QString& identifier );
    void setSession( Akonadi::Session* session );
    void setOutputFile( const QString& outputFile );

private:
    void doStart();

private:
    class Private;
    Private* const d;
    Q_PRIVATE_SLOT( d, void exportFinished( KJob* ) )
};

}

#endif // AKREGATOR2_EXPORTFEEDLISTCOMMAND_H