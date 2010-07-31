/*
    This file is part of Akregator.

    Copyright (C) 2005 Frank Osterfeld <frank.osterfeld at kdemail.net>

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

#include "folder.h"
#include "nodelist.h"
#include "treenode.h"
#include "treenodevisitor.h"

#include <kapplication.h>

#include <tqmap.h>
#include <tqstring.h>
#include <tqvaluelist.h>

namespace Akregator {

class NodeList::NodeListPrivate
{
    public:
    TQValueList<TreeNode*> flatList;
    Folder* rootNode;
    TQString title;
    TQMap<int, TreeNode*> idMap;
    AddNodeVisitor* addNodeVisitor;
    RemoveNodeVisitor* removeNodeVisitor;
};


class NodeList::AddNodeVisitor : public TreeNodeVisitor
{
    public:
        AddNodeVisitor(NodeList* list) : m_list(list) {}


        virtual bool visitTreeNode(TreeNode* node)
        {
            if (!m_preserveID)
            	node->setId(m_list->generateID());
            m_list->d->idMap[node->id()] = node;
            m_list->d->flatList.append(node);
            
            connect(node, TQT_SIGNAL(signalDestroyed(TreeNode*)), m_list, TQT_SLOT(slotNodeDestroyed(TreeNode*) ));
            m_list->signalNodeAdded(node); // emit

            return true;
        }
        virtual bool visitFolder(Folder* node)
        {
            connect(node, TQT_SIGNAL(signalChildAdded(TreeNode*)), m_list, TQT_SLOT(slotNodeAdded(TreeNode*) ));
            connect(node, TQT_SIGNAL(signalChildRemoved(Folder*, TreeNode*)), m_list, TQT_SLOT(slotNodeRemoved(Folder*, TreeNode*) ));

            visitTreeNode(node);

            for (TreeNode* i = node->firstChild(); i && i != node; i = i->next() )
                m_list->slotNodeAdded(i);

            return true;
        }

        virtual void visit(TreeNode* node, bool preserveID)
        {
            m_preserveID = preserveID;
            TreeNodeVisitor::visit(node);
        }

    private:
        NodeList* m_list;
        bool m_preserveID;
};

class NodeList::RemoveNodeVisitor : public TreeNodeVisitor
{
    public:
        RemoveNodeVisitor(NodeList* list) : m_list(list) {}

        virtual bool visitTreeNode(TreeNode* node)
        {
            m_list->d->idMap.remove(node->id());
            m_list->d->flatList.remove(node);

            disconnect(node, TQT_SIGNAL(signalDestroyed(TreeNode*)), m_list, TQT_SLOT(slotNodeDestroyed(TreeNode*) ));
            m_list->signalNodeRemoved(node); // emit signal
            
            return true;
        }

        virtual bool visitFolder(Folder* node)
        {
            
            disconnect(node, TQT_SIGNAL(signalChildAdded(TreeNode*)), m_list, TQT_SLOT(slotNodeAdded(TreeNode*) ));
            disconnect(node, TQT_SIGNAL(signalChildRemoved(Folder*, TreeNode*)), m_list, TQT_SLOT(slotNodeRemoved(Folder*, TreeNode*) ));
            visitTreeNode(node);
          
            return true;
        }
    private:
        NodeList* m_list;
};

NodeList::NodeList(TQObject *parent, const char *name) : d(new NodeListPrivate)
{
    d->rootNode = 0;
    d->addNodeVisitor = new AddNodeVisitor(this);
    d->removeNodeVisitor = new RemoveNodeVisitor(this);
    
}

const TQString& NodeList::title() const
{
    return d->title;
}

TreeNode* NodeList::findByID(int id) const
{
    return d->idMap[id];
}

void NodeList::setTitle(const TQString& title)
{
    d->title = title;
}

Folder* NodeList::rootNode() const
{
    return d->rootNode;
}

const TQValueList<TreeNode*>& NodeList::asFlatList() const
{
    return d->flatList;
}

bool NodeList::isEmpty() const
{
    return d->rootNode->firstChild() == 0;
}

TQValueList<TreeNode*>* NodeList::flatList() const
{
    return &(d->flatList);
}

void NodeList::clear()
{
    Q_ASSERT(rootNode());
    
    TQValueList<TreeNode*> children = rootNode()->children();

    for (TQValueList<TreeNode*>::ConstIterator it = children.begin(); it != children.end(); ++it)
        delete *it; // emits signal "emitSignalDestroyed"
}

TQMap<int, TreeNode*>* NodeList::idMap() const
{
    return &(d->idMap);
}

void NodeList::setRootNode(Folder* folder)
{
    delete d->rootNode;
    d->rootNode = folder;

    if (d->rootNode)
    {
        d->rootNode->setOpen(true);
        connect(d->rootNode, TQT_SIGNAL(signalChildAdded(TreeNode*)), this, TQT_SLOT(slotNodeAdded(TreeNode*)));
        connect(d->rootNode, TQT_SIGNAL(signalChildRemoved(Folder*, TreeNode*)), this, TQT_SLOT(slotNodeRemoved(Folder*, TreeNode*)));
    }
}

void NodeList::addNode(TreeNode* node, bool preserveID)
{
    d->addNodeVisitor->visit(node, preserveID);
}

void NodeList::removeNode(TreeNode* node)
{
   d->removeNodeVisitor->visit(node);
}

NodeList::~NodeList()
{
    emit signalDestroyed(this);
    delete d->addNodeVisitor;
    delete d->removeNodeVisitor;
    delete d;
    d = 0;
}

int NodeList::generateID()
{
    return KApplication::random();
}

void NodeList::slotNodeAdded(TreeNode* node)
{
    Folder* parent = node->parent();
    if ( !node || !d->flatList.contains(parent) || d->flatList.contains(node) )
        return;

    addNode(node, false);
}

void NodeList::slotNodeDestroyed(TreeNode* node)
{
    if ( !node || !d->flatList.contains(node) )
        return;

    removeNode(node);
}

void NodeList::slotNodeRemoved(Folder* /*parent*/, TreeNode* node)
{
    if ( !node || !d->flatList.contains(node) )
        return;

    removeNode(node);
}

}

#include "nodelist.moc"
