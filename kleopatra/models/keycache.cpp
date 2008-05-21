/* -*- mode: c++; c-basic-offset:4 -*-
    models/keycache.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2007,2008 Klarälvdalens Datakonsult AB

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

#include <config-kleopatra.h>

#include "keycache.h"
#include "keycache_p.h"

#include "predicates.h"

#include "smimevalidationpreferences.h"

#include <utils/filesystemwatcher.h>
#include <utils/stl_util.h>

#include <kleo/cryptobackendfactory.h>
#include <kleo/dn.h>
#include <kleo/keylistjob.h>

#include <gpgme++/error.h>
#include <gpgme++/key.h>
#include <gpgme++/decryptionresult.h>
#include <gpgme++/verificationresult.h>
#include <gpgme++/keylistresult.h>

#include <gpg-error.h>

#include <QPointer>
#include <QStringList>
#include <QTimer>

#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/weak_ptr.hpp>

#include <deque>
#include <utility>
#include <algorithm>
#include <functional>
#include <iterator>

using namespace Kleo;
using namespace GpgME;
using namespace boost;

static const unsigned int hours2ms = 1000 * 60 * 60;

//
//
// KeyCache
//
//

namespace {

    make_comparator_str( ByEMail, .first.c_str() );

    struct is_empty : std::unary_function<const char*,bool> {
        bool operator()( const char * s ) const { return !s || !*s; }
    };

    template <typename ForwardIterator, typename BinaryPredicate>
    ForwardIterator unique_by_merge( ForwardIterator first, ForwardIterator last, BinaryPredicate pred ) {
        first = std::adjacent_find( first, last, pred );
        if ( first == last )
            return last;

        ForwardIterator dest = first;
        dest->mergeWith( *++first );
        while ( ++first != last )
            if ( pred( *dest, *first ) )
                dest->mergeWith( *first );
            else
                *++dest = *first;
        return ++dest;
    }
}

class KeyCache::Private {
    friend class ::Kleo::KeyCache;
    KeyCache * const q;
public:
    explicit Private( KeyCache * qq ) : q( qq ) {
        connect( &m_autoKeyListingTimer, SIGNAL( timeout() ), q, SLOT( startKeyListing() ) );
        updateAutoKeyListingTimer();
    }

    template < template <template <typename U> class Op> class Comp >
    std::vector<Key>::const_iterator find( const std::vector<Key> & keys, const char * key ) const {
        const std::vector<Key>::const_iterator it =
            std::lower_bound( keys.begin(), keys.end(), key, Comp<std::less>() );
        if ( it == keys.end() || Comp<std::equal_to>()( *it, key ) )
            return it;
        else
            return keys.end();
    }

    std::vector<Key>::const_iterator find_fpr( const char * fpr ) const {
        return find<_detail::ByFingerprint>( by.fpr, fpr );
    }

    std::pair< std::vector< std::pair<std::string,Key> >::const_iterator,
               std::vector< std::pair<std::string,Key> >::const_iterator >
    find_email( const char * email ) const {
        return std::equal_range( by.email.begin(), by.email.end(),
                                 email, ByEMail<std::less>() );
    }

    std::vector<Key>::const_iterator find_keyid( const char * keyid ) const {
        return find<_detail::ByKeyID>( by.keyid, keyid );
    }

    std::vector<Key>::const_iterator find_shortkeyid( const char * shortkeyid ) const {
        return find<_detail::ByShortKeyID>( by.shortkeyid, shortkeyid );
    }

    std::pair<
        std::vector<Key>::const_iterator,
        std::vector<Key>::const_iterator
    > find_subjects( const char * chain_id ) const {
        return std::equal_range( by.chainid.begin(), by.chainid.end(),
                                 chain_id, _detail::ByChainID<std::less>() );
    }

    void refreshJobDone( const KeyListResult & result );


    void updateAutoKeyListingTimer() {
        setAutoKeyListingInterval( hours2ms * SMimeValidationPreferences().refreshInterval() );
    }
    void setAutoKeyListingInterval( int ms ) {
        m_autoKeyListingTimer.stop();
        m_autoKeyListingTimer.setInterval( ms );
        if ( ms != 0 )
            m_autoKeyListingTimer.start();
    }

private:
    QPointer<RefreshKeysJob> m_refreshJob;
    std::vector<shared_ptr<FileSystemWatcher> > m_fsWatchers;
    QTimer m_autoKeyListingTimer;

    struct By {
        std::vector<Key> fpr, keyid, shortkeyid, chainid;
        std::vector< std::pair<std::string,Key> > email;
    } by;
};


shared_ptr<const KeyCache> KeyCache::instance() {
    return mutableInstance();
}

shared_ptr<KeyCache> KeyCache::mutableInstance() {
    static weak_ptr<KeyCache> self;
    try {
        return shared_ptr<KeyCache>( self );
    } catch ( const bad_weak_ptr & ) {
        const shared_ptr<KeyCache> s( new KeyCache );
        self = s;
        return s;
    }
}

KeyCache::KeyCache()
    : QObject(), d( new Private( this ) )
{

}

KeyCache::~KeyCache() {}

void KeyCache::startKeyListing()
{
    if ( d->m_refreshJob )
        return;

    d->updateAutoKeyListingTimer();

    Q_FOREACH( const shared_ptr<FileSystemWatcher>& i, d->m_fsWatchers )
        i->setEnabled( false );
    d->m_refreshJob = new RefreshKeysJob( this );
    connect( d->m_refreshJob, SIGNAL(done(GpgME::KeyListResult)), this, SLOT(refreshJobDone(GpgME::KeyListResult)) );
    d->m_refreshJob->start();
}

void KeyCache::cancelKeyListing()
{
    if ( !d->m_refreshJob )
        return;
    d->m_refreshJob->cancel();
}

void KeyCache::addFileSystemWatcher( const shared_ptr<FileSystemWatcher>& watcher )
{
    if ( !watcher )
        return;
    d->m_fsWatchers.push_back( watcher );
    connect( watcher.get(), SIGNAL( directoryChanged( QString ) ),
             this, SLOT( startKeyListing() ) );
    connect( watcher.get(), SIGNAL( fileChanged( QString ) ),
             this, SLOT( startKeyListing() ) );

    watcher->setEnabled( d->m_refreshJob == 0 );
}

void KeyCache::Private::refreshJobDone( const KeyListResult& result )
{
    emit q->keyListingDone( result );
    Q_FOREACH( const shared_ptr<FileSystemWatcher>& i, m_fsWatchers )
        i->setEnabled( true );
}

const Key & KeyCache::findByFingerprint( const char * fpr ) const {
    const std::vector<Key>::const_iterator it = d->find_fpr( fpr );
    if ( it == d->by.fpr.end() ) {
        static const Key null;
        return null;
    } else {
        return *it;
    }
}

std::vector<Key> KeyCache::findByFingerprint( const std::vector<std::string> & fprs ) const {
    std::vector<std::string> sorted;
    sorted.reserve( fprs.size() );
    std::remove_copy_if( fprs.begin(), fprs.end(), std::back_inserter( sorted ),
                         bind( is_empty(), bind( &std::string::c_str, _1 ) ) );

    std::sort( sorted.begin(), sorted.end(), _detail::ByFingerprint<std::less>() );

    std::vector<Key> result;
    kdtools::set_intersection( d->by.fpr.begin(), d->by.fpr.end(),
                               sorted.begin(), sorted.end(),
                               std::back_inserter( result ),
                               _detail::ByFingerprint<std::less>() );
    return result;
}

std::vector<Key> KeyCache::findByEMailAddress( const char * email ) const {
    const std::pair<
        std::vector< std::pair<std::string,Key> >::const_iterator,
        std::vector< std::pair<std::string,Key> >::const_iterator
    > pair = d->find_email( email );
    std::vector<Key> result;
    result.reserve( std::distance( pair.first, pair.second ) );
    std::transform( pair.first, pair.second,
                    std::back_inserter( result ),
                    bind( &std::pair<std::string,Key>::second, _1 ) );
    return result;
}

std::vector<Key> KeyCache::findByEMailAddress( const std::string & email ) const {
    return findByEMailAddress( email.c_str() );
}

const Key & KeyCache::findByShortKeyID( const char * id ) const {
    const std::vector<Key>::const_iterator it = d->find_shortkeyid( id );
    if ( it != d->by.shortkeyid.end() )
        return *it;
    static const Key null;
    return null;
}

const Key & KeyCache::findByKeyIDOrFingerprint( const char * id ) const {
    {
        // try by.fpr first:
        const std::vector<Key>::const_iterator it = d->find_fpr( id );
        if ( it != d->by.fpr.end() )
            return *it;
    }{
        // try by.keyid next:
        const std::vector<Key>::const_iterator it = d->find_keyid( id );
        if ( it != d->by.keyid.end() )
            return *it;
    }
    static const Key null;
    return null;
}

std::vector<Key> KeyCache::findByKeyIDOrFingerprint( const std::vector<std::string> & ids ) const {

    std::vector<std::string> keyids;
    std::remove_copy_if( ids.begin(), ids.end(), std::back_inserter( keyids ),
                         bind( is_empty(), bind( &std::string::c_str, _1 ) ) );

    // this is just case-insensitive string search:
    std::sort( keyids.begin(), keyids.end(), _detail::ByFingerprint<std::less>() );

    std::vector<Key> result;
    result.reserve( keyids.size() ); // dups shouldn't happen

    kdtools::set_intersection( d->by.fpr.begin(), d->by.fpr.end(),
                               keyids.begin(), keyids.end(),
                               std::back_inserter( result ),
                               _detail::ByFingerprint<std::less>() );
    if ( result.size() < keyids.size() )
        // note that By{Fingerprint,KeyID,ShortKeyID} define the same
        // order for _strings_
        kdtools::set_intersection( d->by.keyid.begin(), d->by.keyid.end(),
                                   keyids.begin(), keyids.end(),
                                   std::back_inserter( result ),
                                   _detail::ByKeyID<std::less>() );

    // duplicates shouldn't happen, but make sure nonetheless:
    std::sort( result.begin(), result.end(), _detail::ByFingerprint<std::less>() );
    result.erase( std::unique( result.begin(), result.end(), _detail::ByFingerprint<std::equal_to>() ), result.end() );

    // we skip looking into short key ids here, as it's highly
    // unlikely they're used for this purpose. We might need to revise
    // this decision, but only after testing.
    return result;
}

std::vector<Key> KeyCache::findRecipients( const DecryptionResult & res ) const {
    std::vector<std::string> keyids;
    Q_FOREACH( const DecryptionResult::Recipient & r, res.recipients() )
        keyids.push_back( r.keyID() );
    return findByKeyIDOrFingerprint( keyids );
}

std::vector<Key> KeyCache::findSigners( const VerificationResult & res ) const {
    std::vector<std::string> fprs;
    Q_FOREACH( const Signature & s, res.signatures() )
        fprs.push_back( s.fingerprint() );
    return findByKeyIDOrFingerprint( fprs );
}

std::vector<Key> KeyCache::findSubjects( const GpgME::Key & key, Option options ) const {
    return findSubjects( std::vector<Key>( 1, key ), options );
}

std::vector<Key> KeyCache::findSubjects( const std::vector<Key> & keys, Option options ) const {
    return findSubjects( keys.begin(), keys.end(), options );
}

std::vector<Key> KeyCache::findSubjects( std::vector<Key>::const_iterator first, std::vector<Key>::const_iterator last, Option options ) const {

    if ( first == last )
        return std::vector<Key>();

    std::vector<Key> result;
    while ( first != last ) {
        const std::pair<
            std::vector<Key>::const_iterator,
            std::vector<Key>::const_iterator
         > pair = d->find_subjects( first->primaryFingerprint() );
        result.insert( result.end(), pair.first, pair.second );
        ++first;
    }

    std::sort( result.begin(), result.end(), _detail::ByFingerprint<std::less>() );
    result.erase( std::unique( result.begin(), result.end(), _detail::ByFingerprint<std::equal_to>() ), result.end() );

    if ( options & RecursiveSearch )
        return findSubjects( result, options );
    else
        return result;
}

static const unsigned int LIKELY_CHAIN_DEPTH = 3;

std::vector<Key> KeyCache::findIssuers( const Key & key, Option options ) const {

    const Key & issuer = findByFingerprint( key.chainID() );

    if ( issuer.isNull() )
        return std::vector<Key>();

    std::vector<Key> result( 1, issuer );
    if ( !( options & RecursiveSearch ) )
        return result;

    do {
        result.push_back( findByFingerprint( result.back().chainID() ) );
    } while ( !result.back().isNull() );
    result.pop_back();

    return result;
}

std::vector<Key> KeyCache::findIssuers( const std::vector<Key> & keys, Option options ) const {
    return findIssuers( keys.begin(), keys.end(), options );
}

std::vector<Key> KeyCache::findIssuers( std::vector<Key>::const_iterator first, std::vector<Key>::const_iterator last, Option options ) const {

    if ( first == last )
        return std::vector<Key>();

    // extract chain-ids, identifying issuers:
    std::vector<const char *> chainIDs;
    chainIDs.reserve( last - first );
    std::transform( first, last,
                    std::back_inserter( chainIDs ),
                    bind( &Key::chainID, _1 ) );
    std::sort( chainIDs.begin(), chainIDs.end(), _detail::ByFingerprint<std::less>() );

    const std::vector<const char*>::iterator lastUniqueChainID = std::unique( chainIDs.begin(), chainIDs.end(), _detail::ByFingerprint<std::less>() );

    std::vector<Key> result;
    result.reserve( lastUniqueChainID - chainIDs.begin() );

    kdtools::set_intersection( d->by.fpr.begin(), d->by.fpr.end(),
                               chainIDs.begin(), lastUniqueChainID,
                               std::back_inserter( result ),
                               _detail::ByFingerprint<std::less>() );

    if ( !( options & RecursiveSearch ) )
        return result;

    const std::vector<Key> l2result = findIssuers( result, options );

    const unsigned long result_size = result.size();
    result.insert( result.end(), l2result.begin(), l2result.end() );
    std::inplace_merge( result.begin(), result.begin() + result_size, result.end(),
                        _detail::ByFingerprint<std::less>() );
    return result;
}

static std::string email( const UserID & uid ) {
    const std::string email = uid.email();
    if ( email.empty() )
        return DN( uid.id() )["EMAIL"].trimmed().toUtf8().constData();
    if ( email[0] == '<' && email[email.size()-1] == '>' )
        return email.substr( 1, email.size() - 2 );
    else
        return email;
}

static std::vector<std::string> emails( const Key & key ) {
    std::vector<std::string> emails;
    Q_FOREACH( const UserID & uid, key.userIDs() ) {
        const std::string e = email( uid );
        if ( !e.empty() )
            emails.push_back( e );
    }
    std::sort( emails.begin(), emails.end() );
    emails.erase( std::unique( emails.begin(), emails.end() ), emails.end() );
    return emails;
}

void KeyCache::remove( const Key & key ) {
    if ( key.isNull() )
        return;

    const char * fpr = key.primaryFingerprint();
    if ( !fpr )
        return;

    emit aboutToRemove( key );
    
    {
        const std::pair<std::vector<Key>::iterator,std::vector<Key>::iterator> range
            = std::equal_range( d->by.fpr.begin(), d->by.fpr.end(), fpr,
                                _detail::ByFingerprint<std::less>() );
        d->by.fpr.erase( range.first, range.second );
    }

    if ( const char * keyid = key.keyID() ) {
        const std::pair<std::vector<Key>::iterator,std::vector<Key>::iterator> range
            = std::equal_range( d->by.keyid.begin(), d->by.keyid.end(), keyid,
                                _detail::ByKeyID<std::less>() );
        const std::vector<Key>::iterator it
            = std::remove_if( begin( range ), end( range ), bind( _detail::ByFingerprint<std::equal_to>(), fpr, _1 ) );
        d->by.keyid.erase( it, end( range ) );
    }

    if ( const char * shortkeyid = key.shortKeyID() ) {
        const std::pair<std::vector<Key>::iterator,std::vector<Key>::iterator> range
            = std::equal_range( d->by.shortkeyid.begin(), d->by.shortkeyid.end(), shortkeyid,
                                _detail::ByShortKeyID<std::less>() );
        const std::vector<Key>::iterator it
            = std::remove_if( begin( range ), end( range ), bind( _detail::ByFingerprint<std::equal_to>(), fpr, _1 ) );
        d->by.shortkeyid.erase( it, end( range ) );
    }

    if ( const char * chainid = key.chainID() ) {
        const std::pair<std::vector<Key>::iterator,std::vector<Key>::iterator> range
            = std::equal_range( d->by.chainid.begin(), d->by.chainid.end(), chainid,
                                _detail::ByChainID<std::less>() );
        const std::pair< std::vector<Key>::iterator, std::vector<Key>::iterator > range2
            = std::equal_range( begin( range ), end( range ), fpr, _detail::ByFingerprint<std::less>() );
        d->by.chainid.erase( begin( range2 ), end( range2 ) );
    }
        

    Q_FOREACH( const std::string & email, emails( key ) ) {
        const std::pair<std::vector<std::pair<std::string,Key> >::iterator,std::vector<std::pair<std::string,Key> >::iterator> range
            = std::equal_range( d->by.email.begin(), d->by.email.end(), email, ByEMail<std::less>() );
        const std::vector< std::pair<std::string,Key> >::iterator it
            = std::remove_if( begin( range ), end( range ), bind( qstricmp, fpr, bind( &Key::primaryFingerprint, bind( &std::pair<std::string,Key>::second,_1 ) ) ) == 0 );
        d->by.email.erase( it, end( range ) );
    }                
}

void KeyCache::remove( const std::vector<Key> & keys ) {
    Q_FOREACH( const Key & key, keys )
        remove( key );
}

std::vector<GpgME::Key> KeyCache::keys() const
{
    return d->by.fpr;
}

std::vector<Key> KeyCache::secretKeys() const
{
    std::vector<Key> keys = this->keys();
    keys.erase( std::remove_if( keys.begin(), keys.end(), !bind( &Key::hasSecret, _1 ) ) );
    return keys;
}

void KeyCache::refresh( const std::vector<Key> & keys ) {
    // make this better...
    clear();
    insert( keys );
}

void KeyCache::insert( const Key & key ) {
    insert( std::vector<Key>( 1, key ) );
}

namespace {

    template <
        template <template <typename T> class Op> class T1,
        template <template <typename T> class Op> class T2
    > struct lexicographically {
        typedef bool result_type;

        template <typename U, typename V>
        bool operator()( const U & lhs, const V & rhs ) const {
            return
                T1<std::less>()( lhs, rhs ) ||
                T1<std::equal_to>()( lhs, rhs ) &&
                T2<std::less>()( lhs, rhs )
                ;
        }
    };

}

void KeyCache::insert( const std::vector<Key> & keys ) {

    // 1. remove those with empty fingerprints:
    std::vector<Key> sorted;
    sorted.reserve( keys.size() );
    std::remove_copy_if( keys.begin(), keys.end(),
                         std::back_inserter( sorted ),
                         bind( is_empty(), bind( &Key::primaryFingerprint, _1 ) ) );

    Q_FOREACH( const Key & key, sorted )
        remove( key ); // this is sub-optimal, but makes implementation from here on much easier 

    // 2. sort by fingerprint:
    std::sort( sorted.begin(), sorted.end(), _detail::ByFingerprint<std::less>() );

    // 2a. insert into fpr index:
    std::vector<Key> by_fpr;
    by_fpr.reserve( sorted.size() + d->by.fpr.size() );
    std::merge( sorted.begin(), sorted.end(),
                d->by.fpr.begin(), d->by.fpr.end(),
                std::back_inserter( by_fpr ),
                _detail::ByFingerprint<std::less>() );

    // 3. build email index:
    std::vector< std::pair<std::string,Key> > pairs;
    pairs.reserve( sorted.size() );
    Q_FOREACH( const Key & key, sorted ) {
        const std::vector<std::string> emails = ::emails( key );
        std::vector< std::pair<std::string,Key> > tmp, merge_tmp;
        tmp.reserve( emails.size() );
        Q_FOREACH( const std::string & e, emails )
            pairs.push_back( std::make_pair( e, key ) );
        merge_tmp.reserve( tmp.size() + pairs.size() );
        std::merge( pairs.begin(), pairs.end(),
                    tmp.begin(), tmp.end(),
                    std::back_inserter( merge_tmp ),
                    ByEMail<std::less>() );
        pairs.swap( merge_tmp );
    }

    // 3a. insert into email index:
    std::vector< std::pair<std::string,Key> > by_email;
    by_email.reserve( pairs.size() + d->by.email.size() );
    std::merge( pairs.begin(), pairs.end(),
                d->by.email.begin(), d->by.email.end(),
                std::back_inserter( by_email ),
                ByEMail<std::less>() );

    // 3.5: stable-sort by chain-id (effectively lexicographically<ByChainID,ByFingerprint>)
    std::stable_sort( sorted.begin(), sorted.end(), _detail::ByChainID<std::less>() );
    
    // 3.5a: insert into chain-id index:
    std::vector<Key> by_chainid;
    by_chainid.reserve( sorted.size() + d->by.chainid.size() );
    std::merge( sorted.begin(), sorted.end(),
                d->by.chainid.begin(), d->by.chainid.end(),
                std::back_inserter( by_chainid ),
                lexicographically<_detail::ByChainID,_detail::ByFingerprint>() );

    // 4. sort by key id:
    std::sort( sorted.begin(), sorted.end(), _detail::ByKeyID<std::less>() );

    // 4a. insert into keyid index:
    std::vector<Key> by_keyid;
    by_keyid.reserve( sorted.size() + d->by.keyid.size() );
    std::merge( sorted.begin(), sorted.end(),
                d->by.keyid.begin(), d->by.keyid.end(),
                std::back_inserter( by_keyid ),
                _detail::ByKeyID<std::less>() );

    // 5. sort by short key id:
    std::sort( sorted.begin(), sorted.end(), _detail::ByShortKeyID<std::less>() );

    // 5a. insert into short keyid index:
    std::vector<Key> by_shortkeyid;
    by_shortkeyid.reserve( sorted.size() + d->by.shortkeyid.size() );
    std::merge( sorted.begin(), sorted.end(),
                d->by.shortkeyid.begin(), d->by.shortkeyid.end(),
                std::back_inserter( by_shortkeyid ),
                _detail::ByShortKeyID<std::less>() );

    // now commit (well, we already removed keys...)
    by_fpr.swap( d->by.fpr );
    by_keyid.swap( d->by.keyid );
    by_shortkeyid.swap( d->by.shortkeyid );
    by_email.swap( d->by.email );

    Q_FOREACH( const Key & key, sorted )
        emit added( key ); 

    emit keysMayHaveChanged();
}

void KeyCache::clear() {
    d->by = Private::By();
}

//
//
// RefreshKeysJob
//
//

class KeyCache::RefreshKeysJob::Private
{
    RefreshKeysJob * const q;
public:
    enum KeyType {
        PublicKeys,
        SecretKeys
    };
    
    Private( KeyCache * cache, RefreshKeysJob * qq );
    void doStart();
    Error startKeyListing( const char* protocol, KeyType type );
    void jobDone( const KeyListResult & res );
    void emitDone( const KeyListResult & result );
    void nextSecretKey( const Key & key );
    void nextPublicKey( const Key & key );
    void mergeKeysAndUpdateKeyCache();

    KeyCache * m_cache;
    uint m_jobsPending;
    std::deque<Key> m_publicKeys;
    std::deque<Key> m_secretKeys;
    KeyListResult m_mergedResult;
};

KeyCache::RefreshKeysJob::Private::Private( KeyCache * cache, RefreshKeysJob * qq ) : q( qq ), m_cache( cache ), m_jobsPending( 0 )
{
    assert( m_cache );
}

void KeyCache::RefreshKeysJob::Private::jobDone( const KeyListResult & result )
{
    QObject* const sender = q->sender();
    if ( sender )
        sender->disconnect( q );
    assert( m_jobsPending > 0 );
    --m_jobsPending;
    m_mergedResult.mergeWith( result );
    if ( m_jobsPending > 0 )
        return;
    mergeKeysAndUpdateKeyCache();
    emitDone( m_mergedResult );
}

void KeyCache::RefreshKeysJob::Private::emitDone( const KeyListResult & res )
{
    q->deleteLater();
    emit q->done( res );
}

void KeyCache::RefreshKeysJob::Private::nextSecretKey( const Key & key )
{
    m_secretKeys.push_back( key );
}

void KeyCache::RefreshKeysJob::Private::nextPublicKey( const Key & key )
{
    m_publicKeys.push_back( key );
}

KeyCache::RefreshKeysJob::RefreshKeysJob( KeyCache * cache, QObject * parent ) : QObject( parent ), d( new Private( cache, this ) )
{
}

KeyCache::RefreshKeysJob::~RefreshKeysJob() {}


void KeyCache::RefreshKeysJob::start()
{
    QTimer::singleShot( 0, this, SLOT( doStart() ) );
}

void KeyCache::RefreshKeysJob::cancel()
{
    emit canceled();
}

void KeyCache::RefreshKeysJob::Private::doStart()
{
    assert( m_jobsPending == 0 );
    m_mergedResult.mergeWith( KeyListResult( startKeyListing( "openpgp", PublicKeys ) ) );
    m_mergedResult.mergeWith( KeyListResult( startKeyListing( "openpgp", SecretKeys ) ) );
    m_mergedResult.mergeWith( KeyListResult( startKeyListing( "smime", PublicKeys ) ) );
    m_mergedResult.mergeWith( KeyListResult( startKeyListing( "smime", SecretKeys ) ) );
    
    if ( m_jobsPending != 0 )
        return;

    const bool hasError = m_mergedResult.error() || m_mergedResult.error().isCanceled();
    emitDone( hasError ? m_mergedResult : KeyListResult( Error( GPG_ERR_UNSUPPORTED_OPERATION ) ) );
}

void KeyCache::RefreshKeysJob::Private::mergeKeysAndUpdateKeyCache()
{
    std::sort( m_publicKeys.begin(), m_publicKeys.end(), _detail::ByFingerprint<std::less>() );
    std::sort( m_secretKeys.begin(), m_secretKeys.end(), _detail::ByFingerprint<std::less>() );

    std::vector<Key> keys;
    keys.reserve( m_publicKeys.size() + m_secretKeys.size() );

    std::merge( m_publicKeys.begin(), m_publicKeys.end(),
                m_secretKeys.begin(), m_secretKeys.end(),
                std::back_inserter( keys ),
                _detail::ByFingerprint<std::less>() );

    keys.erase( unique_by_merge( keys.begin(), keys.end(), _detail::ByFingerprint<std::equal_to>() ),
                keys.end() );

    std::vector<Key> cachedKeys = m_cache->keys();
    std::sort( cachedKeys.begin(), cachedKeys.end(), _detail::ByFingerprint<std::less>() );
    std::vector<Key> keysToRemove;
    std::set_difference( cachedKeys.begin(), cachedKeys.end(), keys.begin(), keys.end(), std::back_inserter( keysToRemove ), _detail::ByFingerprint<std::less>() );
    m_cache->remove( keysToRemove );
    m_cache->refresh( keys );
}

Error KeyCache::RefreshKeysJob::Private::startKeyListing( const char* backend, KeyType type )
{
    const Kleo::CryptoBackend::Protocol * const protocol = Kleo::CryptoBackendFactory::instance()->protocol( backend );
    if ( !protocol )
        return Error();
    Kleo::KeyListJob * const job = protocol->keyListJob( /*remote*/false, /*includeSigs*/false, /*validate*/true );
    if ( !job )
        return Error();
    connect( job, SIGNAL(result(GpgME::KeyListResult)),
             q, SLOT(jobDone(GpgME::KeyListResult)) );
#if 0
    connect( job, SIGNAL(progress(QString,int,int)),
             q, SIGNAL(progress(QString,int,int)) );
#endif
    connect( q, SIGNAL(canceled()),
             job, SLOT(slotCancel()) );

    if ( type == PublicKeys )
        connect( job, SIGNAL(nextKey(GpgME::Key)),
                 q, SLOT(nextPublicKey(GpgME::Key)) );
    else
        connect( job, SIGNAL(nextKey(GpgME::Key)),
                 q, SLOT(nextSecretKey(GpgME::Key)) );
    
    const Error error = job->start( QStringList(), type == SecretKeys );
    
    if ( !error && !error.isCanceled() )
        ++m_jobsPending;
    return error;
}

#include "moc_keycache_p.cpp"
#include "moc_keycache.cpp"

