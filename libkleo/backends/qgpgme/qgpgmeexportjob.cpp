/*
    qgpgmeexportjob.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
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

#include "qgpgmeexportjob.h"

#include <qgpgme/eventloopinteractor.h>
#include <qgpgme/dataprovider.h>

#include <gpgme++/context.h>
#include <gpgme++/data.h>

#include <QStringList>

#include <assert.h>

Kleo::QGpgMEExportJob::QGpgMEExportJob( GpgME::Context * context )
  : ExportJob( QGpgME::EventLoopInteractor::instance() ),
    QGpgMEJob( this, context )
{
  assert( context );
}

Kleo::QGpgMEExportJob::~QGpgMEExportJob() {
}

GpgME::Error Kleo::QGpgMEExportJob::start( const QStringList & pats ) {
  assert( !patterns() );
  assert( !mOutData );

  createOutData();
  setPatterns( pats );
  hookupContextToEventLoopInteractor();

  const GpgME::Error err = mCtx->startPublicKeyExport( patterns(), *mOutData );

  if ( err )
    deleteLater();
  return err;
}

void Kleo::QGpgMEExportJob::doOperationDoneEvent( const GpgME::Error & error ) {
  const QByteArray data = outData();
  getAuditLog();
  emit result( error, data );
}

#include "qgpgmeexportjob.moc"
