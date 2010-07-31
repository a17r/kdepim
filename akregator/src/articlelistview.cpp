/*
    This file is part of Akregator.

    Copyright (C) 2004 Stanislav Karchebny <Stanislav.Karchebny@kdemail.net>
                  2005 Frank Osterfeld <frank.osterfeld at kdemail.net>
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

#include "akregatorconfig.h"
#include "actionmanager.h"
#include "articlelistview.h"
#include "article.h"
#include "articlefilter.h"
#include "dragobjects.h"
#include "feed.h"
#include "treenode.h"
#include "treenodevisitor.h"

#include <kstandarddirs.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kcharsets.h>
#include <kurl.h>

#include <tqdatetime.h>
#include <tqpixmap.h>
#include <tqpopupmenu.h>
#include <tqptrlist.h>
#include <tqvaluelist.h>
#include <tqwhatsthis.h>
#include <tqheader.h>
#include <tqdragobject.h>
#include <tqsimplerichtext.h>
#include <tqpainter.h>
#include <tqapplication.h>

#include <ctime>

namespace Akregator {

class ArticleListView::ArticleListViewPrivate
{
    public:

    ArticleListViewPrivate(ArticleListView* parent) : m_parent(parent) { }

    void ensureCurrentItemVisible()
    {
        if (m_parent->currentItem())
        {
            m_parent->center( m_parent->contentsX(), m_parent->itemPos(m_parent->currentItem()), 0, 9.0 );
        }
    }

    ArticleListView* m_parent;

    /** maps article to article item */
    TQMap<Article, ArticleItem*> articleMap;
    TreeNode* node;
    Akregator::Filters::ArticleMatcher textFilter;
    Akregator::Filters::ArticleMatcher statusFilter;
    enum ColumnMode { groupMode, feedMode };
    ColumnMode columnMode;
    int feedWidth;
    bool noneSelected;
    
    ColumnLayoutVisitor* columnLayoutVisitor;
};
  
class ArticleListView::ColumnLayoutVisitor : public TreeNodeVisitor
{
    public:
        ColumnLayoutVisitor(ArticleListView* view) : m_view(view) {}

        virtual bool visitTagNode(TagNode* /*node*/)
        {
            if (m_view->d->columnMode == ArticleListViewPrivate::feedMode)
            {
                m_view->setColumnWidth(1, m_view->d->feedWidth);
                m_view->d->columnMode = ArticleListViewPrivate::groupMode;
            }
            return true;
        }
        
        virtual bool visitFolder(Folder* /*node*/)
        {
            if (m_view->d->columnMode == ArticleListViewPrivate::feedMode)
            {
                m_view->setColumnWidth(1, m_view->d->feedWidth);
                m_view->d->columnMode = ArticleListViewPrivate::groupMode;
            }
            return true;
        }
        
        virtual bool visitFeed(Feed* /*node*/)
        {
            if (m_view->d->columnMode == ArticleListViewPrivate::groupMode)
            {    
                m_view->d->feedWidth = m_view->columnWidth(1);
                m_view->hideColumn(1);
                m_view->d->columnMode = ArticleListViewPrivate::feedMode;
            }
            return true;
        }
    private:

        ArticleListView* m_view;
    
};

class ArticleListView::ArticleItem : public KListViewItem
    {
        friend class ArticleListView;

        public:
            ArticleItem( TQListView *parent, const Article& a);
            ~ArticleItem();

            Article& article();

            void paintCell ( TQPainter * p, const TQColorGroup & cg, int column, int width, int align );
            virtual int compare(TQListViewItem *i, int col, bool ascending) const;

            void updateItem(const Article& article);

            virtual ArticleItem* itemAbove() { return static_cast<ArticleItem*>(KListViewItem::itemAbove()); }
            
            virtual ArticleItem* nextSibling() { return static_cast<ArticleItem*>(KListViewItem::nextSibling()); }

        private:
            Article m_article;
            time_t m_pubDate;
            static TQPixmap keepFlag() {
                   static TQPixmap s_keepFlag = TQPixmap(locate("data", "akregator/pics/akregator_flag.png"));
                   return s_keepFlag;
	    }
};

// FIXME: Remove resolveEntities for KDE 4.0, it's now done in the parser
ArticleListView::ArticleItem::ArticleItem( TQListView *parent, const Article& a)
    : KListViewItem( parent, KCharsets::resolveEntities(a.title()), a.feed()->title(), KGlobal::locale()->formatDateTime(a.pubDate(), true, false) ), m_article(a), m_pubDate(a.pubDate().toTime_t())
{
    if (a.keep())
        setPixmap(0, keepFlag());
}
 
ArticleListView::ArticleItem::~ArticleItem()
{
}

Article& ArticleListView::ArticleItem::article()
{
    return m_article;
}

// paint ze peons
void ArticleListView::ArticleItem::paintCell ( TQPainter * p, const TQColorGroup & cg, int column, int width, int align )
{
    if (article().status() == Article::Read)
        KListViewItem::paintCell( p, cg, column, width, align );
    else
    {
        // if article status is unread or new, we change the color: FIXME: make colors configurable
        TQColorGroup cg2(cg);
    
        if (article().status() == Article::Unread)
            cg2.setColor(TQColorGroup::Text, Qt::blue);
        else // New
            cg2.setColor(TQColorGroup::Text, Qt::red);
    
        KListViewItem::paintCell( p, cg2, column, width, align );
    }

}

void ArticleListView::ArticleItem::updateItem(const Article& article)
{
    m_article = article;
    setPixmap(0, m_article.keep() ? keepFlag() : TQPixmap());
    setText(0, KCharsets::resolveEntities(m_article.title()));
    setText(1, m_article.feed()->title());
    setText(2, KGlobal::locale()->formatDateTime(m_article.pubDate(), true, false));
}

int ArticleListView::ArticleItem::compare(TQListViewItem *i, int col, bool ascending) const {
    if (col == 2)
    {
        ArticleItem* item = static_cast<ArticleItem*>(i);
        if (m_pubDate == item->m_pubDate)
            return 0;
        return (m_pubDate > item->m_pubDate) ? 1 : -1;
    }
    return KListViewItem::compare(i, col, ascending);
}

/* ==================================================================================== */

ArticleListView::ArticleListView(TQWidget *parent, const char *name)
    : KListView(parent, name)
{
    d = new ArticleListViewPrivate(this);
    d->noneSelected = true;
    d->node = 0;
    d->columnMode = ArticleListViewPrivate::feedMode;

    d->columnLayoutVisitor = new ColumnLayoutVisitor(this);
    setMinimumSize(250, 150);
    addColumn(i18n("Article"));
    addColumn(i18n("Feed"));
    addColumn(i18n("Date"));
    setSelectionMode(TQListView::Extended);
    setColumnWidthMode(2, TQListView::Maximum);
    setColumnWidthMode(1, TQListView::Manual);
    setColumnWidthMode(0, TQListView::Manual);
    setRootIsDecorated(false);
    setItemsRenameable(false);
    setItemsMovable(false);
    setAllColumnsShowFocus(true);
    setDragEnabled(true); // FIXME before we implement dragging between archived feeds??
    setAcceptDrops(false); // FIXME before we implement dragging between archived feeds??
    setFullWidth(false);
    
    setShowSortIndicator(true);
    setDragAutoScroll(true);
    setDropHighlighter(false);

    int c = Settings::sortColumn();
    setSorting((c >= 0 && c <= 2) ? c : 2, Settings::sortAscending());

    int w;
    w = Settings::titleWidth();
    if (w > 0) {
        setColumnWidth(0, w);
    }
    
    w = Settings::feedWidth();
    if (w > 0) {
        setColumnWidth(1, w);
    }
    
    w = Settings::dateWidth();
    if (w > 0) {
        setColumnWidth(2, w);
    }
    
    d->feedWidth = columnWidth(1);
    hideColumn(1);

    header()->setStretchEnabled(true, 0);

    TQWhatsThis::add(this, i18n("<h2>Article list</h2>"
        "Here you can browse articles from the currently selected feed. "
        "You can also manage articles, as marking them as persistent (\"Keep Article\") or delete them, using the right mouse button menu."
        "To view the web page of the article, you can open the article internally in a tab or in an external browser window."));

    connect(this, TQT_SIGNAL(currentChanged(TQListViewItem*)), this, TQT_SLOT(slotCurrentChanged(TQListViewItem* )));
    connect(this, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(slotSelectionChanged()));
    connect(this, TQT_SIGNAL(doubleClicked(TQListViewItem*, const TQPoint&, int)),  this, TQT_SLOT(slotDoubleClicked(TQListViewItem*, const TQPoint&, int)) );
    connect(this, TQT_SIGNAL(contextMenu(KListView*, TQListViewItem*, const TQPoint&)),
            this, TQT_SLOT(slotContextMenu(KListView*, TQListViewItem*, const TQPoint&)));

    connect(this, TQT_SIGNAL(mouseButtonPressed(int, TQListViewItem *, const TQPoint &, int)), this, TQT_SLOT(slotMouseButtonPressed(int, TQListViewItem *, const TQPoint &, int)));
}

Article ArticleListView::currentArticle() const
{
    ArticleItem* ci = dynamic_cast<ArticleItem*>(KListView::currentItem());
    return (ci && !selectedItems().isEmpty()) ? ci->article() : Article();
}

void ArticleListView::slotSetFilter(const Akregator::Filters::ArticleMatcher& textFilter, const Akregator::Filters::ArticleMatcher& statusFilter)
{
    if ( (textFilter != d->textFilter) || (statusFilter != d->statusFilter) )
    {
        d->textFilter = textFilter;
        d->statusFilter = statusFilter;
               
        applyFilters();
    }
}

void ArticleListView::slotShowNode(TreeNode* node)
{
    if (node == d->node)
        return;

    slotClear();

    if (!node)
        return;

    d->node = node;
    connectToNode(node);

    d->columnLayoutVisitor->visit(node);

    setUpdatesEnabled(false);

    TQValueList<Article> articles = d->node->articles();

    TQValueList<Article>::ConstIterator end = articles.end();
    TQValueList<Article>::ConstIterator it = articles.begin();
    
    for (; it != end; ++it)
    {
        if (!(*it).isNull() && !(*it).isDeleted())
        {
            ArticleItem* ali = new ArticleItem(this, *it);
            d->articleMap.insert(*it, ali);
        }
    }

    sort();
    applyFilters();
    d->noneSelected = true;
    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::slotClear()
{
    if (d->node)
        disconnectFromNode(d->node);
        
    d->node = 0;
    d->articleMap.clear();
    clear();
}

void ArticleListView::slotArticlesAdded(TreeNode* /*node*/, const TQValueList<Article>& list)
{
    setUpdatesEnabled(false);
    
    bool statusActive = !(d->statusFilter.matchesAll());
    bool textActive = !(d->textFilter.matchesAll());
    
    for (TQValueList<Article>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if (!d->articleMap.contains(*it))
        {
            if (!(*it).isNull() && !(*it).isDeleted())
            {
                ArticleItem* ali = new ArticleItem(this, *it);
                ali->setVisible( (!statusActive ||  d->statusFilter.matches( ali->article()))
                        && (!textActive || d->textFilter.matches( ali->article())) );
                d->articleMap.insert(*it, ali);
            }
        }
    }
    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::slotArticlesUpdated(TreeNode* /*node*/, const TQValueList<Article>& list)
{
    setUpdatesEnabled(false);

    // if only one item is selected and this selected item
    // is deleted, we will select the next item in the list
    bool singleSelected = selectedArticles().count() == 1;
    
    bool statusActive = !(d->statusFilter.matchesAll());
    bool textActive = !(d->textFilter.matchesAll());
    
    TQListViewItem* next = 0; // the item to select if a selected item is deleted
    
    for (TQValueList<Article>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        
        if (!(*it).isNull() && d->articleMap.contains(*it))
        {
            ArticleItem* ali = d->articleMap[*it];

            if (ali)
            {
                if ((*it).isDeleted()) // if article was set to deleted, delete item
                {
                    if (singleSelected && ali->isSelected())
                    {
                        if (ali->itemBelow())
                            next = ali->itemBelow();
                        else if (ali->itemAbove())
                            next = ali->itemAbove();
                    }
                    
                    d->articleMap.remove(*it);
                    delete ali;
                }
                else
                {
                    ali->updateItem(*it);
                    // if the updated article matches the filters after the update,
                    // make visible. If it matched them before but not after update,
                    // they should stay visible (to not confuse users)
                    if ((!statusActive || d->statusFilter.matches(ali->article()))
                        && (!textActive || d->textFilter.matches( ali->article())) )
                        ali->setVisible(true);
                }
            } // if ali
        }
    }

    // if the only selected item was deleted, select
    // an item next to it
    if (singleSelected && next != 0)
    {
        setSelected(next, true);
        setCurrentItem(next);
    }
    else
    {
        d->noneSelected = true;
    }
    

    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::slotArticlesRemoved(TreeNode* /*node*/, const TQValueList<Article>& list)
{
    // if only one item is selected and this selected item
    // is deleted, we will select the next item in the list
    bool singleSelected = selectedArticles().count() == 1;

    TQListViewItem* next = 0; // the item to select if a selected item is deleted
    
    setUpdatesEnabled(false);
    
    for (TQValueList<Article>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if (d->articleMap.contains(*it))
        {
            ArticleItem* ali = d->articleMap[*it];
            d->articleMap.remove(*it);
            
            if (singleSelected && ali->isSelected())
            {
                if (ali->itemBelow())
                    next = ali->itemBelow();
                else if (ali->itemAbove())
                    next = ali->itemAbove();
            }
            
            delete ali;
        }
    }
    
    // if the only selected item was deleted, select
    // an item next to it
    if (singleSelected && next != 0)
    {
        setSelected(next, true);
        setCurrentItem(next);
    }
    else
    {
        d->noneSelected = true;
    }
     
    setUpdatesEnabled(true);
    triggerUpdate();
}
            
void ArticleListView::connectToNode(TreeNode* node)
{
    connect(node, TQT_SIGNAL(signalDestroyed(TreeNode*)), this, TQT_SLOT(slotClear()) );
    connect(node, TQT_SIGNAL(signalArticlesAdded(TreeNode*, const TQValueList<Article>&)), this, TQT_SLOT(slotArticlesAdded(TreeNode*, const TQValueList<Article>&)) );
    connect(node, TQT_SIGNAL(signalArticlesUpdated(TreeNode*, const TQValueList<Article>&)), this, TQT_SLOT(slotArticlesUpdated(TreeNode*, const TQValueList<Article>&)) );
    connect(node, TQT_SIGNAL(signalArticlesRemoved(TreeNode*, const TQValueList<Article>&)), this, TQT_SLOT(slotArticlesRemoved(TreeNode*, const TQValueList<Article>&)) );
}

void ArticleListView::disconnectFromNode(TreeNode* node)
{
    disconnect(node, TQT_SIGNAL(signalDestroyed(TreeNode*)), this, TQT_SLOT(slotClear()) );
    disconnect(node, TQT_SIGNAL(signalArticlesAdded(TreeNode*, const TQValueList<Article>&)), this, TQT_SLOT(slotArticlesAdded(TreeNode*, const TQValueList<Article>&)) );
    disconnect(node, TQT_SIGNAL(signalArticlesUpdated(TreeNode*, const TQValueList<Article>&)), this, TQT_SLOT(slotArticlesUpdated(TreeNode*, const TQValueList<Article>&)) );
    disconnect(node, TQT_SIGNAL(signalArticlesRemoved(TreeNode*, const TQValueList<Article>&)), this, TQT_SLOT(slotArticlesRemoved(TreeNode*, const TQValueList<Article>&)) );
}

void ArticleListView::applyFilters()
{
    bool statusActive = !(d->statusFilter.matchesAll());
    bool textActive = !(d->textFilter.matchesAll());
    
    ArticleItem* ali = 0;
    
    if (!statusActive && !textActive)
    {
        for (TQListViewItemIterator it(this); it.current(); ++it)
        {
            (static_cast<ArticleItem*> (it.current()))->setVisible(true);
        }
    }
    else if (statusActive && !textActive)
    {
        for (TQListViewItemIterator it(this); it.current(); ++it)
        {
            ali = static_cast<ArticleItem*> (it.current());
            ali->setVisible( d->statusFilter.matches( ali->article()) );
        }
    }
    else if (!statusActive && textActive)
    {
        for (TQListViewItemIterator it(this); it.current(); ++it)
        {
            ali = static_cast<ArticleItem*> (it.current());
            ali->setVisible( d->textFilter.matches( ali->article()) );
        }
    }
    else // both true
    {
        for (TQListViewItemIterator it(this); it.current(); ++it)
        {
            ali = static_cast<ArticleItem*> (it.current());
            ali->setVisible( d->statusFilter.matches( ali->article()) 
                    && d->textFilter.matches( ali->article()) );
        }
    }

}

int ArticleListView::visibleArticles()
{
    int visible = 0;
    ArticleItem* ali = 0;
    for (TQListViewItemIterator it(this); it.current(); ++it) {
        ali = static_cast<ArticleItem*> (it.current());
        visible += ali->isVisible() ? 1 : 0;
    }
    return visible;
}

// from amarok :)
void ArticleListView::paintInfoBox(const TQString &message)
{
    TQPainter p( viewport() );
    TQSimpleRichText t( message, TQApplication::font() );

    if ( t.width()+30 >= viewport()->width() || t.height()+30 >= viewport()->height() )
            //too big, giving up
        return;

    const uint w = t.width();
    const uint h = t.height();
    const uint x = (viewport()->width() - w - 30) / 2 ;
    const uint y = (viewport()->height() - h - 30) / 2 ;

    p.setBrush( colorGroup().background() );
    p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
    t.draw( &p, x+15, y+15, TQRect(), colorGroup() );
}

void ArticleListView::viewportPaintEvent(TQPaintEvent *e)
{

    KListView::viewportPaintEvent(e);
    
    if(!e)
        return;
        
    TQString message = TQString::null;
    
    //kdDebug() << "visible articles: " << visibleArticles() << endl;
    
    if(childCount() != 0) // article list is not empty
    {
        if (visibleArticles() == 0)
        {
        message = i18n("<div align=center>"
                        "<h3>No matches</h3>"
                        "Filter does not match any articles, "
                        "please change your criteria and try again."
                        "</div>");
        }
        
    }
    else // article list is empty
    {
        if (!d->node) // no node selected
        {
            message = i18n("<div align=center>"
                       "<h3>No feed selected</h3>"
                       "This area is article list. "
                       "Select a feed from the feed list "
                       "and you will see its articles here."
                       "</div>");
        }
        else // empty node
        {
            // TODO: we could display message like "empty node, choose "fetch" to update it" 
        }
    }
    
    if (!message.isNull())
        paintInfoBox(message);
}

TQDragObject *ArticleListView::dragObject()
{
    TQDragObject* d = 0;
    TQValueList<Article> articles = selectedArticles();
    if (!articles.isEmpty())
    {
        d = new ArticleDrag(articles, this);
    }
    return d;
}

void ArticleListView::slotPreviousArticle()
{
    ArticleItem* ali = 0;
    if (!currentItem() || selectedItems().isEmpty())
        ali = dynamic_cast<ArticleItem*>(lastChild());
    else
        ali = dynamic_cast<ArticleItem*>(currentItem()->itemAbove());
    
    if (ali)
    {
        Article a = ali->article();
        setCurrentItem(d->articleMap[a]);
        clearSelection();
        setSelected(d->articleMap[a], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::slotNextArticle()
{
    ArticleItem* ali = 0;
    if (!currentItem() || selectedItems().isEmpty())
        ali = dynamic_cast<ArticleItem*>(firstChild());
    else
        ali = dynamic_cast<ArticleItem*>(currentItem()->itemBelow());
    
    if (ali)
    {
        Article a = ali->article();
        setCurrentItem(d->articleMap[a]);
        clearSelection();
        setSelected(d->articleMap[a], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::slotNextUnreadArticle()
{
    ArticleItem* start = 0L;
    if (!currentItem() || selectedItems().isEmpty())
        start = dynamic_cast<ArticleItem*>(firstChild());
    else
        start = dynamic_cast<ArticleItem*>(currentItem()->itemBelow() ? currentItem()->itemBelow() : firstChild());

    ArticleItem* i = start;
    ArticleItem* unread = 0L;
    
    do
    {
        if (i == 0L)
            i = static_cast<ArticleItem*>(firstChild());
        else
        {
            if (i->article().status() != Article::Read)
                unread = i;
            else 
                i = static_cast<ArticleItem*>(i && i->itemBelow() ? i->itemBelow() : firstChild());
        }
    }
    while (!unread && i != start);

    if (unread)
    {
        Article a = unread->article();
        setCurrentItem(d->articleMap[a]);
        clearSelection();
        setSelected(d->articleMap[a], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::slotPreviousUnreadArticle()
{
    ArticleItem* start = 0L;
    if (!currentItem() || selectedItems().isEmpty())
        start = dynamic_cast<ArticleItem*>(lastChild());
    else
        start = dynamic_cast<ArticleItem*>(currentItem()->itemAbove() ? currentItem()->itemAbove() : firstChild());

    ArticleItem* i = start;
    ArticleItem* unread = 0L;
    
    do
    {
        if (i == 0L)
            i = static_cast<ArticleItem*>(lastChild());
        else
        {
            if (i->article().status() != Article::Read)
                unread = i;
            else 
                i = static_cast<ArticleItem*>(i->itemAbove() ? i->itemAbove() : lastChild());
        }
    }
    while ( !(unread != 0L || i == start) );

    if (unread)
    {
        Article a = unread->article();
        setCurrentItem(d->articleMap[a]);
        clearSelection();
        setSelected(d->articleMap[a], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::keyPressEvent(TQKeyEvent* e)
{
    e->ignore();
}

void ArticleListView::slotSelectionChanged()
{
    // if there is only one article in the list, currentItem is set initially to 
    // that article item, although the user hasn't selected it. If the user selects
    // the article, selection changes, but currentItem does not.
    // executed. So we have to handle this case by observing selection changes.
    
    if (d->noneSelected)
    {
        d->noneSelected = false;
        slotCurrentChanged(currentItem());
    }
}

void ArticleListView::slotCurrentChanged(TQListViewItem* item)
{
    ArticleItem* ai = dynamic_cast<ArticleItem*> (item);
    if (ai)
        emit signalArticleChosen( ai->article() );
    else
    {
        d->noneSelected = true;
        emit signalArticleChosen( Article() );
    }
} 


void ArticleListView::slotDoubleClicked(TQListViewItem* item, const TQPoint& p, int i)
{
    ArticleItem* ali = dynamic_cast<ArticleItem*>(item);
    if (ali)
        emit signalDoubleClicked(ali->article(), p, i);
}

void ArticleListView::slotContextMenu(KListView* /*list*/, TQListViewItem* /*item*/, const TQPoint& p)
{
    TQWidget* w = ActionManager::getInstance()->container("article_popup");
    TQPopupMenu* popup = static_cast<TQPopupMenu *>(w);
    if (popup)
        popup->exec(p);
}

void ArticleListView::slotMouseButtonPressed(int button, TQListViewItem* item, const TQPoint& p, int column)
{
    ArticleItem* ali = dynamic_cast<ArticleItem*>(item);
    if (ali)
        emit signalMouseButtonPressed(button, ali->article(), p, column);
}

ArticleListView::~ArticleListView()
{
    Settings::setTitleWidth(columnWidth(0));
    Settings::setFeedWidth(columnWidth(1) > 0 ? columnWidth(1) : d->feedWidth);
    Settings::setSortColumn(sortColumn());
    Settings::setSortAscending(sortOrder() == Ascending);
    Settings::writeConfig();
    delete d->columnLayoutVisitor;
    delete d;
    d = 0;
}

TQValueList<Article> ArticleListView::selectedArticles() const
{
    TQValueList<Article> ret;
    TQPtrList<TQListViewItem> items = selectedItems(false);
    for (TQListViewItem* i = items.first(); i; i = items.next() )
        ret.append((static_cast<ArticleItem*>(i))->article());
    return ret;
}

} // namespace Akregator

#include "articlelistview.moc"
// vim: ts=4 sw=4 et
