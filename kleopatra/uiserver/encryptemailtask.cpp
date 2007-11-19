/* -*- mode: c++; c-basic-offset:4 -*-
    uiserver/encryptemailtask.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2007 Klarälvdalens Datakonsult AB

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "encryptemailtask.h"

#include "kleo-assuan.h"

#include "input.h"
#include "output.h"

#include <utils/stl_util.h>

#include <kleo/cryptobackendfactory.h>
#include <kleo/cryptobackend.h>
#include <kleo/encryptjob.h>

#include <gpgme++/encryptionresult.h>
#include <gpgme++/key.h>

#include <KLocale>

#include <QPointer>
#include <QTextDocument> // for Qt::escape

#include <boost/bind.hpp>

using namespace Kleo;
using namespace boost;
using namespace GpgME;

namespace {

    class EncryptEMailResult : public Task::Result {
        const EncryptionResult m_result;
    public:
        explicit EncryptEMailResult( const EncryptionResult & r )
            : Task::Result(), m_result( r ) {}

        /* reimp */ QString overview() const;
        /* reimp */ QString details() const;
    };


    static QString makeErrorString( const EncryptionResult & result ) {
        const Error err = result.error();

        assuan_assert( err || err.isCanceled() );

        if ( err.isCanceled() )
            return i18n("Encryption canceled.");
        else // if ( err )
            return i18n("Encryption failed: %1.", Qt::escape( QString::fromLocal8Bit( err.asString() ) ) );
    }

}

class EncryptEMailTask::Private {
    friend class ::Kleo::EncryptEMailTask;
    EncryptEMailTask * const q;
public:
    explicit Private( EncryptEMailTask * qq );

private:
    std::auto_ptr<Kleo::EncryptJob> createJob( GpgME::Protocol proto );

private:
    void slotResult( const EncryptionResult & );

private:
    shared_ptr<Input> input;
    shared_ptr<Output> output;
    std::vector<Key> recipients;

    QPointer<Kleo::EncryptJob> job;
};

EncryptEMailTask::Private::Private( EncryptEMailTask * qq )
    : q( qq ),
      input(),
      output(),
      job( 0 )
{

}

EncryptEMailTask::EncryptEMailTask( QObject * p )
    : Task( p ), d( new Private( this ) )
{

}

EncryptEMailTask::~EncryptEMailTask() {}

void EncryptEMailTask::setInput( const shared_ptr<Input> & input ) {
    assuan_assert( !d->job );
    assuan_assert( input );
    d->input = input;
}

void EncryptEMailTask::setOutput( const shared_ptr<Output> & output ) {
    assuan_assert( !d->job );
    assuan_assert( output );
    d->output = output;
}

void EncryptEMailTask::setRecipients( const std::vector<Key> & recipients ) {
    assuan_assert( !d->job );
    assuan_assert( !recipients.empty() );
    d->recipients = recipients;
}

Protocol EncryptEMailTask::protocol() const {
    assuan_assert( !d->recipients.empty() );
    return d->recipients.front().protocol();
}

void EncryptEMailTask::start() {
    assuan_assert( !d->job );
    assuan_assert( d->input );
    assuan_assert( d->output );
    assuan_assert( !d->recipients.empty() );

    std::auto_ptr<Kleo::EncryptJob> job = d->createJob( protocol() );
    assuan_assert( job.get() );

    job->start( d->recipients,
                d->input->ioDevice(), d->output->ioDevice(),
                /*alwaysTrust=*/true );

    d->job = job.release();
}

void EncryptEMailTask::cancel() {
    if ( d->job )
        d->job->slotCancel();
}

std::auto_ptr<Kleo::EncryptJob> EncryptEMailTask::Private::createJob( GpgME::Protocol proto ) {
    const CryptoBackend::Protocol * const backend = CryptoBackendFactory::instance()->protocol( proto );
    assuan_assert( backend );
    std::auto_ptr<Kleo::EncryptJob> encryptJob( backend->encryptJob( /*armor=*/true, /*textmode=*/false ) );
    assuan_assert( encryptJob.get() );
    connect( encryptJob.get(), SIGNAL(progress(QString,int,int)),
             q, SIGNAL(progress(QString,int,int)) );
    connect( encryptJob.get(), SIGNAL(result(GpgME::EncryptionResult,QByteArray)),
             q, SLOT(slotResult(GpgME::EncryptionResult)) );
    return encryptJob;
}

void EncryptEMailTask::Private::slotResult( const EncryptionResult & result ) {
    if ( result.error().code() ) {
        output->cancel();
        emit q->error( result.error(), makeErrorString( result ) );
    } else {
        output->finalize();
        emit q->result( shared_ptr<Result>( new EncryptEMailResult( result ) ) );
    }
}

QString EncryptEMailResult::overview() const {
    return i18n("Not yet implemented");
}

QString EncryptEMailResult::details() const {
    return i18n("Not yet implemented");
}


#include "moc_encryptemailtask.cpp"


