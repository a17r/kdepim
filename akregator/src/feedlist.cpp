/*
    This file is part of Akregator.

    Copyright (C) 2004 Frank Osterfeld <frank.osterfeld at kdemail.net>

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
#include "feedlist.h"

#include <tqdatetime.h>
#include <tqdom.h>
#include <tqmap.h>
#include <tqvaluelist.h>

#include <kdebug.h>
#include <klocale.h>

#include "article.h"
#include "feed.h"
#include "folder.h"
#include "treenode.h"
#include "treenodevisitor.h"

namespace Akregator {

class FeedList::FeedListPrivate
{
    public:

        TQMap<TQString, TQValueList<Feed*> > urlMap;
        AddNodeVisitor* addNodeVisitor;
        RemoveNodeVisitor* removeNodeVisitor;
};

class FeedList::AddNodeVisitor : public TreeNodeVisitor
{
    public:
        AddNodeVisitor(FeedList* list) : m_list(list) {}


        virtual bool visitFeed(Feed* node)
        {
            m_list->idMap()->insert(node->id(), node);
            m_list->flatList()->append(node);
            return true;
        }

    private:
        FeedList* m_list;
};

class FeedList::RemoveNodeVisitor : public TreeNodeVisitor
{
    public:
        RemoveNodeVisitor(FeedList* list) : m_list(list) {}

        virtual bool visitFeed(Feed* node)
        {
            m_list->d->urlMap[node->xmlUrl()].remove(node);
            return true;
        }

    private:
        FeedList* m_list;
};

FeedList::FeedList(TQObject *parent, const char *name)
    : NodeList(parent, name), d(new FeedListPrivate)
{
    d->addNodeVisitor = new AddNodeVisitor(this);
    d->removeNodeVisitor = new RemoveNodeVisitor(this);

    Folder* rootNode = new Folder(i18n("All Feeds"));
    rootNode->setId(1);
    setRootNode(rootNode);
    addNode(rootNode, true);
}

void FeedList::addNode(TreeNode* node, bool preserveID)
{
    NodeList::addNode(node, preserveID);
    d->addNodeVisitor->visit(node);
}

void FeedList::removeNode(TreeNode* node)
{
   NodeList::removeNode(node);
   d->removeNodeVisitor->visit(node);
}

void FeedList::parseChildNodes(TQDomNode &node, Folder* parent)
{
    TQDomElement e = node.toElement(); // try to convert the node to an element.

    if( !e.isNull() )
    {
        TQString title = e.hasAttribute("text") ? e.attribute("text") : e.attribute("title");

        if (e.hasAttribute("xmlUrl") || e.hasAttribute("xmlurl") || e.hasAttribute("xmlURL") )
        {
            Feed* feed = Feed::fromOPML(e);
            if (feed)
            {
                if (!d->urlMap[feed->xmlUrl()].contains(feed))
                    d->urlMap[feed->xmlUrl()].append(feed);
                parent->appendChild(feed);
            }
        }
        else
        {
            Folder* fg = Folder::fromOPML(e);
            parent->appendChild(fg);

            if (e.hasChildNodes())
            {
                TQDomNode child = e.firstChild();
                while(!child.isNull())
                {
                    parseChildNodes(child, fg);
                    child = child.nextSibling();
                }
            }
        }
    }
}

bool FeedList::readFromXML(const TQDomDocument& doc)
{
    TQDomElement root = doc.documentElement();

    kdDebug() << "loading OPML feed " << root.tagName().lower() << endl;

    kdDebug() << "measuring startup time: START" << endl;
    TQTime spent;
    spent.start();

    if (root.tagName().lower() != "opml")
    {
        return false;
    }
    TQDomNode bodyNode = root.firstChild();

    while (!bodyNode.isNull() && bodyNode.toElement().tagName().lower() != "body")
        bodyNode = bodyNode.nextSibling();


    if (bodyNode.isNull())
    {
        kdDebug() << "Failed to acquire body node, markup broken?" << endl;
        return false;
    }

    TQDomElement body = bodyNode.toElement();

    TQDomNode i = body.firstChild();

    while( !i.isNull() )
    {
        parseChildNodes(i, rootNode());
        i = i.nextSibling();
    }

    for (TreeNode* i = rootNode()->firstChild(); i && i != rootNode(); i = i->next() )
        if (i->id() == 0)
    {
            uint id = generateID();
            i->setId(id);
            idMap()->insert(id, i);
    }

    kdDebug() << "measuring startup time: STOP, " << spent.elapsed() << "ms" << endl;
    kdDebug() << "Number of articles loaded: " << rootNode()->totalCount() << endl;
    return true;
}

FeedList::~FeedList()
{
    emit signalDestroyed(this);
    setRootNode(0);
    delete d->addNodeVisitor;
    delete d->removeNodeVisitor;
    delete d;
    d = 0;
}

Feed* FeedList::findByURL(const TQString& feedURL) const
{
    if (d->urlMap[feedURL].isEmpty())
        return 0;
    else
        return *(d->urlMap[feedURL].begin());
}

Article FeedList::findArticle(const TQString& feedURL, const TQString& guid) const
{
    Feed* feed = findByURL(feedURL);

    return feed ? feed->findArticle(guid) : Article();
}

void FeedList::append(FeedList* list, Folder* parent, TreeNode* after)
{
    if ( list == this )
        return;

    if ( !flatList()->contains(parent) )
        parent = rootNode();

    TQValueList<TreeNode*> children = list->rootNode()->children();

    TQValueList<TreeNode*>::ConstIterator end(  children.end() );
    for (TQValueList<TreeNode*>::ConstIterator it = children.begin(); it != end; ++it)
    {
        list->rootNode()->removeChild(*it);
        parent->insertChild(*it, after);
        after = *it;
    }
}

TQDomDocument FeedList::toXML() const
{
    TQDomDocument doc;
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );

    TQDomElement root = doc.createElement( "opml" );
    root.setAttribute( "version", "1.0" );
    doc.appendChild( root );

    TQDomElement head = doc.createElement( "head" );
    root.appendChild( head );

    TQDomElement ti = doc.createElement( "text" );
    head.appendChild( ti );

    TQDomText t = doc.createTextNode( title() );
    ti.appendChild( t );

    TQDomElement body = doc.createElement( "body" );
    root.appendChild( body );

    TQValueList<TreeNode*> children = rootNode()->children();

    TQValueList<TreeNode*>::ConstIterator end(  children.end() );

    for (TQValueList<TreeNode*>::ConstIterator it = children.begin(); it != end; ++it)
        body.appendChild( (*it)->toOPML(body, doc) );

    return doc;
}

} // namespace Akregator
#include "feedlist.moc"
