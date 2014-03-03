/*
 * articlematcher.cpp
 *
 * Copyright (c) 2004, 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "articlematcher.h"
#include "types.h"

#include <krss/item.h>

#include <kapplication.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <krandom.h>
#include <kurl.h>

#include <QList>
#include <QRegExp>


using namespace KRss;

namespace Akregator2 {
namespace Filters {

AbstractMatcher::AbstractMatcher() {}

AbstractMatcher::~AbstractMatcher() {}

QString Criterion::subjectToString(Subject subj)
{
    switch (subj)
    {
        case Title:
            return QString::fromLatin1("Title");
        case Link:
            return QString::fromLatin1("Link");
        case Description:
            return QString::fromLatin1("Description");
        case Status:
            return QString::fromLatin1("Status");
        case KeepFlag:
            return QString::fromLatin1("KeepFlag");
        default: // should never happen (TM)
            return QString::fromLatin1("Description");
    }
}

Criterion::Subject Criterion::stringToSubject(const QString& subjStr)
{
    if (subjStr == QString::fromLatin1("Title"))
        return Title;
    else if (subjStr == QString::fromLatin1("Link"))
        return Link;
    else if (subjStr == QString::fromLatin1("Description"))
        return Description;
    else if (subjStr == QString::fromLatin1("Status"))
        return Status;
    else if (subjStr == QString::fromLatin1("KeepFlag"))
        return KeepFlag;

    // hopefully never reached
    return Description;
}

QString Criterion::predicateToString(Predicate pred)
{
    switch (pred)
    {
        case Contains:
            return QString::fromLatin1("Contains");
        case Equals:
            return QString::fromLatin1("Equals");
        case Matches:
            return QString::fromLatin1("Matches");
        case Negation:
            return QString::fromLatin1("Negation");
        default:// hopefully never reached
            return QString::fromLatin1("Contains");
    }
}

Criterion::Predicate Criterion::stringToPredicate(const QString& predStr)
{
    if (predStr == QString::fromLatin1("Contains"))
        return Contains;
    else if (predStr == QString::fromLatin1("Equals"))
        return Equals;
    else if (predStr == QString::fromLatin1("Matches"))
        return Matches;
    else if (predStr == QString::fromLatin1("Negation"))
        return Negation;

    // hopefully never reached
    return Contains;
}

Criterion::Criterion()
{
}

Criterion::Criterion( Subject subject, Predicate predicate, const QVariant &object )
    : m_subject( subject )
    , m_predicate( predicate )
    , m_object( object )
{

}

void Criterion::writeConfig(KConfigGroup* config) const
{
    config->writeEntry(QString::fromLatin1("subject"), subjectToString(m_subject));

    config->writeEntry(QString::fromLatin1("predicate"), predicateToString(m_predicate));

    config->writeEntry(QString::fromLatin1("objectType"), QString(m_object.typeName()));

    config->writeEntry(QString::fromLatin1("objectValue"), m_object);
}

void Criterion::readConfig(KConfigGroup* config)
{
    m_subject = stringToSubject(config->readEntry(QString::fromLatin1("subject"), QString()));
    m_predicate = stringToPredicate(config->readEntry(QString::fromLatin1("predicate"), QString()));
    QVariant::Type type = QVariant::nameToType(config->readEntry(QString::fromLatin1("objType"), QString()).toAscii());

    if (type != QVariant::Invalid)
    {
        m_object = config->readEntry(QString::fromLatin1("objectValue"), QVariant(type) );
    }
}

static ArticleStatus convertStatus( const Akonadi::Item& item ) {
    if ( KRss::Item::isUnread( item ) )
        return Akregator2::Unread;
    else
        return Akregator2::Read;
}

bool Criterion::satisfiedBy( const Akonadi::Item& item ) const
{
    QVariant concreteSubject;
    const Item rssitem = item.payload<Item>();

    switch ( m_subject ) {
        case Title:
            concreteSubject = QVariant(rssitem.title());
            break;
        case Description:
            concreteSubject = QVariant(rssitem.description());
            break;
        case Link:
            // ### Maybe use prettyUrl here?
            concreteSubject = QVariant(rssitem.link());
            break;
        case Status:
            concreteSubject = QVariant(convertStatus(item));
            break;
        case KeepFlag:
            concreteSubject = QVariant(Item::isImportant(item));
        default:
            break;
    }

    bool satisfied = false;

    const Predicate predicateType = static_cast<Predicate>( m_predicate & ~Negation );
    QString subjectType=concreteSubject.typeName();

    switch ( predicateType ) {
        case Contains:
            satisfied = concreteSubject.toString().indexOf( m_object.toString(), 0, Qt::CaseInsensitive ) != -1;
            break;
        case Equals:
            if (subjectType=="int")
                satisfied = concreteSubject.toInt() == m_object.toInt();
            else
                satisfied = concreteSubject.toString() == m_object.toString();
            break;
        case Matches:
            satisfied = QRegExp( m_object.toString() ).indexIn( concreteSubject.toString() ) != -1;
            break;
        default:
            kDebug() <<"Internal inconsistency; predicateType should never be Negation";
            break;
    }

    if ( m_predicate & Negation ) {
        satisfied = !satisfied;
    }

    return satisfied;
}

Criterion::Subject Criterion::subject() const
{
    return m_subject;
}

Criterion::Predicate Criterion::predicate() const
{
    return m_predicate;
}

QVariant Criterion::object() const
{
    return m_object;
}

ArticleMatcher::ArticleMatcher()
    : m_association( None )
{
}

ArticleMatcher::~ArticleMatcher()
{
}

ArticleMatcher::ArticleMatcher( const QList<Criterion> &criteria, Association assoc)
    : m_criteria( criteria )
    , m_association( assoc )
{
}

bool ArticleMatcher::matches( const Akonadi::Item& item ) const
{
    switch ( m_association ) {
        case LogicalOr:
            return anyCriterionMatches( item );
        case LogicalAnd:
            return allCriteriaMatch( item );
        default:
            break;
    }
    return true;
}

void ArticleMatcher::writeConfig(KConfigGroup* config) const
{
    config->writeEntry(QString::fromLatin1("matcherAssociation"), associationToString(m_association));

    config->writeEntry(QString::fromLatin1("matcherCriteriaCount"), m_criteria.count());

    int index = 0;

    QString criterionGroupPrefix = config->name() + QString::fromLatin1("_Criterion");

    for (QList<Criterion>::ConstIterator it = m_criteria.begin(); it != m_criteria.end(); ++it)
    {
        config->changeGroup(criterionGroupPrefix + QString::number(index));
        (*it).writeConfig(config);
        ++index;
    }
}

void ArticleMatcher::readConfig(KConfigGroup* config)
{
    m_criteria.clear();
    m_association = stringToAssociation(config->readEntry(QString::fromLatin1("matcherAssociation"), QString()));

    int count =  config->readEntry(QString::fromLatin1("matcherCriteriaCount"), 0);

    QString criterionGroupPrefix = config->name() + QString::fromLatin1("_Criterion");

    for (int i = 0; i < count; ++i)
    {
        Criterion c;
        config->changeGroup(criterionGroupPrefix + QString::number(i));
        c.readConfig(config);
        m_criteria.append(c);
    }
}

bool ArticleMatcher::operator==(const AbstractMatcher& other) const
{
    AbstractMatcher* ptr = const_cast<AbstractMatcher*>(&other);
    ArticleMatcher* o = dynamic_cast<ArticleMatcher*>(ptr);
    if (!o)
        return false;
    else
        return m_association == o->m_association && m_criteria == o->m_criteria;
}
bool ArticleMatcher::operator!=(const AbstractMatcher& other) const
{
    return !(*this == other);
}

bool ArticleMatcher::anyCriterionMatches( const Akonadi::Item& item ) const
{
    if (m_criteria.count()==0)
        return true;
    QList<Criterion>::ConstIterator it = m_criteria.constBegin();
    QList<Criterion>::ConstIterator end = m_criteria.constEnd();
    for ( ; it != end; ++it ) {
        if ( ( *it ).satisfiedBy( item ) ) {
            return true;
        }
    }
    return false;
}

bool ArticleMatcher::allCriteriaMatch( const Akonadi::Item& item ) const
{
    if (m_criteria.count()==0)
        return true;
    QList<Criterion>::ConstIterator it = m_criteria.constBegin();
    QList<Criterion>::ConstIterator end = m_criteria.constEnd();
    for ( ; it != end; ++it ) {
        if ( !( *it ).satisfiedBy( item ) ) {
            return false;
        }
    }
    return true;
}

ArticleMatcher::Association ArticleMatcher::stringToAssociation(const QString& assocStr)
{
    if (assocStr == QString::fromLatin1("LogicalAnd"))
        return LogicalAnd;
    else if (assocStr == QString::fromLatin1("LogicalOr"))
        return LogicalOr;
    else
        return None;
}

QString ArticleMatcher::associationToString(Association association)
{
    switch (association)
    {
        case LogicalAnd:
            return QString::fromLatin1("LogicalAnd");
        case LogicalOr:
            return QString::fromLatin1("LogicalOr");
        default:
            return QString::fromLatin1("None");
    }
}


} //namespace Filters
} //namespace Akregator2