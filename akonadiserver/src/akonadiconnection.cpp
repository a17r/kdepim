/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include <QDebug>
#include <QEventLoop>

#include "akonadiconnection.h"
#include "handler.h"
#include "response.h"

#include <assert.h>

using namespace Akonadi;

AkonadiConnection::AkonadiConnection( int socketDescriptor, QObject *parent )
    : QThread(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_tcpSocket( 0 )
    , m_currentHandler( 0 )
    , m_connectionState( NonAuthenticated )
{
}


AkonadiConnection::~AkonadiConnection()
{
    qDebug() << "Connection closed";
    delete m_tcpSocket;
}

void AkonadiConnection::run()
{
    m_tcpSocket = new QTcpSocket();

    if ( !m_tcpSocket->setSocketDescriptor( m_socketDescriptor ) ) {
        emit error(m_tcpSocket->error());
        return;
    }

    writeOut( "* OK Akonadi Almost IMAP Server");

    /* Start a local event loop and start processing incoming data. Whenever
     * a full command has been read, it is delegated to the responsible
     * handler and processed by that. If that command needs to do something
     * asynchronous such as ask the db for data, it returns and the input
     * queue can continue to be processed. Whenever there is something to
     * be sent back to the user it is queued in the form of a Response object.
     * All this is meant to make it possible to process large incoming or
     * outgoing data transfers in a streaming manner, without having to
     * hold them in memory 'en gros'. */

    connect( m_tcpSocket, SIGNAL( readyRead() ),
             this, SLOT( slotNewData() ), Qt::DirectConnection );
    connect( m_tcpSocket, SIGNAL( disconnected() ),
             this, SLOT( slotDisconnected() ), Qt::DirectConnection );
    qDebug() << "on: " << currentThreadId() <<  endl;
    exec();
}

void AkonadiConnection::slotDisconnected()
{
    quit();
}

void AkonadiConnection::slotNewData()
{
    QByteArray line = m_tcpSocket->readLine().trimmed();
    qDebug() << "GOT:" << line <<  endl;
    if ( !m_currentHandler ) {
        // this is a new command, which means the line must start with a tag
        // followed by a non-empty command. First get the tag
        int separator = line.indexOf(' ');
        if ( separator == -1 || separator == line.size() ) {
            writeOut( "* BAD Untagged client command cannot be processed" );
            return;
        }
        // parse our the tag
        const QByteArray tag = line.left( separator );
        // and the command
        int commandStart = line.indexOf( ' ', 0 ) + 1;
        int commandEnd = line.indexOf( ' ', commandStart );
        if ( commandEnd == -1 ) commandEnd = line.size();
        const QByteArray command = line.mid( commandStart, commandEnd - commandStart ).toUpper();

        m_currentHandler = findHandlerForCommand( command );
        m_currentHandler->setTag( tag );
        assert( m_currentHandler );
        connect( m_currentHandler, SIGNAL( responseAvailable( const Response & ) ),
                 this, SLOT( slotResponseAvailable( const Response & ) ), Qt::DirectConnection );
        connect( m_currentHandler, SIGNAL( connectionStateChange( ConnectionState ) ),
                 this, SLOT( slotConnectionStateChange( ConnectionState ) ),
                 Qt::DirectConnection );
    }
    if ( m_currentHandler->handleLine( line ) )
        m_currentHandler = 0;
}

void AkonadiConnection::writeOut( const char* str )
{
    qDebug() << "writing out: " << str << endl;
    QByteArray block;
    QTextStream out(&block, QIODevice::WriteOnly);
    out << str << endl;
    out.flush();
    m_tcpSocket->write(block);
    m_tcpSocket->waitForBytesWritten();
}


Handler * AkonadiConnection::findHandlerForCommand( const QByteArray & command )
{
    Handler * handler = Handler::findHandlerForCommandAlwaysAllowed( command );
    if ( handler ) return handler;
    
    switch ( m_connectionState ) {
        case NonAuthenticated:
            handler =  Handler::findHandlerForCommandNonAuthenticated( command );            break;
        case Authenticated:
            handler =  Handler::findHandlerForCommandAuthenticated( command );
            break;
        case Selected:
            break;
        case LoggingOut:
            break; 
    }
    // we didn't have a handler for this, let the default one do its thing
    if ( !handler ) handler = new Handler();
    return handler;
}

void AkonadiConnection::slotResponseAvailable( const Response& response )
{
    // FIXME handle reentrancy in the presence of continuation. Something like:
    // "if continuation pending, queue responses, once continuation is done, replay them"
    writeOut( response.asString().data() );
}

void AkonadiConnection::slotConnectionStateChange( ConnectionState state )
{
    if ( state == m_connectionState ) return;
    m_connectionState = state;
    switch ( m_connectionState ) {
        case NonAuthenticated:
            assert( 0 ); // can't happen, it's only the initial state, we can't go back to it
            break;
        case Authenticated:
            break;
        case Selected:
            break;
        case LoggingOut:
            m_tcpSocket->disconnectFromHost();
            break;
    }
}
