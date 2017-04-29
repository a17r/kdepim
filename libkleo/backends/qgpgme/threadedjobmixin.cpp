/*
    threadedjobmixin.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2008 Klarälvdalens Datakonsult AB

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

#include "threadedjobmixin.h"

#include <kde4_qgpgme/dataprovider.h>

#include <kde4_gpgme++/data.h>

#include <QString>
#include <QStringList>
#include <QByteArray>

#ifndef Q_MOC_RUN
#include <boost/mem_fn.hpp>
#endif

#include <algorithm>
#include <iterator>

using namespace Kleo;
using namespace GpgME;
using namespace boost;

static const unsigned int GetAuditLogFlags = Context::AuditLogWithHelp|Context::HtmlAuditLog;

QString _detail::audit_log_as_html( Context * ctx, GpgME::Error & err ) {
  assert( ctx );
  QGpgME::QByteArrayDataProvider dp;
  Data data( &dp );
  assert( !data.isNull() );
  if ( ( err = ctx->lastError() ) || ( err = ctx->getAuditLog( data, GetAuditLogFlags ) ) )
    return QString::fromLocal8Bit( err.asString() );
  const QByteArray ba = dp.data();
  return QString::fromUtf8( ba.data(), ba.size() );
}

static QList<QByteArray> from_sl( const QStringList & sl ) {
  QList<QByteArray> result;
  std::transform( sl.begin(), sl.end(), std::back_inserter( result ),
                  mem_fn( &QString::toUtf8 ) );
  return result;
}

static QList<QByteArray> single( const QByteArray & ba ) {
  QList<QByteArray> result;
  result.push_back( ba );
  return result;
}

_detail::PatternConverter::PatternConverter( const QByteArray & ba )
  : m_list( single( ba ) ), m_patterns( 0 ) {}
_detail::PatternConverter::PatternConverter( const QString & s )
  : m_list( single( s.toUtf8() ) ), m_patterns( 0 ) {}
_detail::PatternConverter::PatternConverter( const QList<QByteArray> & lba )
  : m_list( lba ), m_patterns( 0 ) {}
_detail::PatternConverter::PatternConverter( const QStringList & sl )
  :  m_list( from_sl( sl ) ), m_patterns( 0 ) {}

const char ** _detail::PatternConverter::patterns() const {
  if ( !m_patterns ) {
    m_patterns = new const char*[ m_list.size() + 1 ];
    const char ** end = std::transform( m_list.begin(), m_list.end(), m_patterns,
                                        mem_fn( &QByteArray::constData ) );
    *end = 0;
  }
  return m_patterns;
}

_detail::PatternConverter::~PatternConverter() {
  delete [] m_patterns;
}
