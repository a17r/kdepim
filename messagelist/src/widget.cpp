/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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
*/

#include "widget.h"

#include <collection.h>
#include <item.h>
#include <itemcopyjob.h>
#include <itemmovejob.h>

#include "storagemodel.h"
#include "core/messageitem.h"
#include "core/view.h"
#include <messagelistsettings.h>

#include <QTimer>
#include <QAction>
#include <QApplication>
#include <QDrag>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

#include "messagelist_debug.h"
#include <QIcon>
#include <KIconLoader>
#include <KLocalizedString>
#include <QMenu>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <QUrl>

#include "core/groupheaderitem.h"

#include <Monitor>
#include <Tag>
#include <TagFetchJob>
#include <TagFetchScope>
#include <TagAttribute>

namespace MessageList
{

class Q_DECL_HIDDEN Widget::Private
{
public:
    Private(Widget *owner)
        : q(owner), mLastSelectedMessage(-1), mXmlGuiClient(Q_NULLPTR), mMonitor(Q_NULLPTR) { }

    Akonadi::Item::List selectionAsItems() const;
    Akonadi::Item itemForRow(int row) const;
    KMime::Message::Ptr messageForRow(int row) const;

    Widget *const q;

    int mLastSelectedMessage;
    KXMLGUIClient *mXmlGuiClient;
    QModelIndex mGroupHeaderItemIndex;
    Akonadi::Monitor *mMonitor;
};

} // namespace MessageList

using namespace MessageList;
using namespace Akonadi;

Widget::Widget(QWidget *parent)
    : Core::Widget(parent), d(new Private(this))
{
    populateStatusFilterCombo();

    d->mMonitor = new Akonadi::Monitor(this);
    d->mMonitor->setTypeMonitored(Akonadi::Monitor::Tags);
    connect(d->mMonitor, &Akonadi::Monitor::tagAdded, this, &Widget::populateStatusFilterCombo);
    connect(d->mMonitor, &Akonadi::Monitor::tagRemoved, this, &Widget::populateStatusFilterCombo);
    connect(d->mMonitor, &Akonadi::Monitor::tagChanged, this, &Widget::populateStatusFilterCombo);
}

Widget::~Widget()
{
    delete d;
}

void Widget::setXmlGuiClient(KXMLGUIClient *xmlGuiClient)
{
    d->mXmlGuiClient = xmlGuiClient;
}

bool Widget::canAcceptDrag(const QDropEvent *e)
{
    if (e->source() == view()->viewport()) {
        return false;
    }

    Collection::List collections = static_cast<const StorageModel *>(storageModel())->displayedCollections();
    if (collections.size() != 1) {
        return false;    // no folder here or too many (in case we can't decide where the drop will end)
    }

    const Collection target = collections.first();

    if ((target.rights() & Collection::CanCreateItem) == 0) {
        return false;    // no way to drag into
    }

    const QList<QUrl> urls = e->mimeData()->urls();
    foreach (const QUrl &url, urls) {
        const Collection collection = Collection::fromUrl(url);
        if (collection.isValid()) {   // You're not supposed to drop collections here
            return false;
        } else { // Yay, this is an item!
            const QString type = url.queryItemValue(QStringLiteral("type"));
            if (!target.contentMimeTypes().contains(type)) {
                return false;
            }
        }
    }

    return true;
}

bool Widget::selectNextMessageItem(MessageList::Core::MessageTypeFilter messageTypeFilter,
                                   MessageList::Core::ExistingSelectionBehaviour existingSelectionBehaviour,
                                   bool centerItem,
                                   bool loop)
{
    return view()->selectNextMessageItem(messageTypeFilter, existingSelectionBehaviour, centerItem, loop);
}

bool Widget::selectPreviousMessageItem(MessageList::Core::MessageTypeFilter messageTypeFilter,
                                       MessageList::Core::ExistingSelectionBehaviour existingSelectionBehaviour,
                                       bool centerItem,
                                       bool loop)
{
    return view()->selectPreviousMessageItem(messageTypeFilter, existingSelectionBehaviour, centerItem, loop);
}

bool Widget::focusNextMessageItem(MessageList::Core::MessageTypeFilter messageTypeFilter, bool centerItem, bool loop)
{
    return view()->focusNextMessageItem(messageTypeFilter, centerItem, loop);
}

bool Widget::focusPreviousMessageItem(MessageList::Core::MessageTypeFilter messageTypeFilter, bool centerItem, bool loop)
{
    return view()->focusPreviousMessageItem(messageTypeFilter, centerItem, loop);
}

void Widget::selectFocusedMessageItem(bool centerItem)
{
    view()->selectFocusedMessageItem(centerItem);
}

bool Widget::selectFirstMessageItem(MessageList::Core::MessageTypeFilter messageTypeFilter, bool centerItem)
{
    return view()->selectFirstMessageItem(messageTypeFilter, centerItem);
}

bool Widget::selectLastMessageItem(Core::MessageTypeFilter messageTypeFilter, bool centerItem)
{
    return view()->selectLastMessageItem(messageTypeFilter, centerItem);
}

void Widget::selectAll()
{
    view()->setAllGroupsExpanded(true);
    view()->selectAll();
}

void Widget::setCurrentThreadExpanded(bool expand)
{
    view()->setCurrentThreadExpanded(expand);
}

void Widget::setAllThreadsExpanded(bool expand)
{
    view()->setAllThreadsExpanded(expand);
}

void Widget::setAllGroupsExpanded(bool expand)
{
    view()->setAllGroupsExpanded(expand);
}

void Widget::focusQuickSearch(const QString &selectedText)
{
    view()->focusQuickSearch(selectedText);
}

void Widget::setQuickSearchClickMessage(const QString &msg)
{
    view()->setQuickSearchClickMessage(msg);
}

void Widget::fillMessageTagCombo()
{
    Akonadi::TagFetchJob *fetchJob = new Akonadi::TagFetchJob(this);
    fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(fetchJob, &Akonadi::TagFetchJob::result, this, &Widget::slotTagsFetched);
}

void Widget::slotTagsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(MESSAGELIST_LOG) << "Failed to load tags " << job->errorString();
        return;
    }
    Akonadi::TagFetchJob *fetchJob = static_cast<Akonadi::TagFetchJob *>(job);

    KConfigGroup conf(MessageList::MessageListSettings::self()->config(), "MessageListView");
    const QString tagSelected = conf.readEntry(QStringLiteral("TagSelected"));
    if (tagSelected.isEmpty()) {
        setCurrentStatusFilterItem();
        return;
    }
    const QStringList tagSelectedLst = tagSelected.split(QLatin1Char(','));

    addMessageTagItem(QIcon::fromTheme(QStringLiteral("mail-flag")).pixmap(16, 16), i18nc("Item in list of Akonadi tags, to show all e-mails", "All"), QString());

    QStringList tagFound;
    foreach (const Akonadi::Tag &akonadiTag, fetchJob->tags()) {
        if (tagSelectedLst.contains(akonadiTag.url().url())) {
            tagFound.append(akonadiTag.url().url());
            QString iconName = QStringLiteral("mail-tagged");
            const QString label = akonadiTag.name();
            const QString id = akonadiTag.url().url();
            Akonadi::TagAttribute *attr = akonadiTag.attribute<Akonadi::TagAttribute>();
            if (attr) {
                iconName = attr->iconName();
            }
            addMessageTagItem(QIcon::fromTheme(iconName).pixmap(16, 16), label, QVariant(id));
        }
    }
    conf.writeEntry(QStringLiteral("TagSelected"), tagFound);
    conf.sync();

    setCurrentStatusFilterItem();
}

void Widget::viewMessageSelected(MessageList::Core::MessageItem *msg)
{
    int row = -1;
    if (msg) {
        row = msg->currentModelIndexRow();
    }

    if (!msg || !msg->isValid() || !storageModel()) {
        d->mLastSelectedMessage = -1;
        Q_EMIT messageSelected(Item());
        return;
    }

    Q_ASSERT(row >= 0);

    d->mLastSelectedMessage = row;

    Q_EMIT messageSelected(d->itemForRow(row));     // this MAY be null
}

void Widget::viewMessageActivated(MessageList::Core::MessageItem *msg)
{
    Q_ASSERT(msg);   // must not be null
    Q_ASSERT(storageModel());

    if (!msg->isValid()) {
        return;
    }

    int row = msg->currentModelIndexRow();
    Q_ASSERT(row >= 0);

    // The assert below may fail when quickly opening and closing a non-selected thread.
    // This will actually activate the item without selecting it...
    //Q_ASSERT( d->mLastSelectedMessage == row );

    if (d->mLastSelectedMessage != row) {
        // Very ugly. We are activating a non selected message.
        // This is very likely a double click on the plus sign near a thread leader.
        // Dealing with mLastSelectedMessage here would be expensive: it would involve releasing the last selected,
        // emitting signals, handling recursion... ugly.
        // We choose a very simple solution: double clicking on the plus sign near a thread leader does
        // NOT activate the message (i.e open it in a toplevel window) if it isn't previously selected.
        return;
    }

    Q_EMIT messageActivated(d->itemForRow(row));     // this MAY be null
}

void Widget::viewSelectionChanged()
{
    Q_EMIT selectionChanged();
    if (!currentMessageItem()) {
        Q_EMIT messageSelected(Item());
    }
}

void Widget::viewMessageListContextPopupRequest(const QList< MessageList::Core::MessageItem * > &selectedItems,
        const QPoint &globalPos)
{
    Q_UNUSED(selectedItems);

    if (!d->mXmlGuiClient) {
        return;
    }

    QMenu *popup = static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(
                                            QStringLiteral("akonadi_messagelist_contextmenu"),
                                            d->mXmlGuiClient));
    if (popup) {
        popup->exec(globalPos);
    }
}

void Widget::viewMessageStatusChangeRequest(MessageList::Core::MessageItem *msg, Akonadi::MessageStatus set, Akonadi::MessageStatus clear)
{
    Q_ASSERT(msg);   // must not be null
    Q_ASSERT(storageModel());

    if (!msg->isValid()) {
        return;
    }

    int row = msg->currentModelIndexRow();
    Q_ASSERT(row >= 0);

    Item item = d->itemForRow(row);
    Q_ASSERT(item.isValid());

    Q_EMIT messageStatusChangeRequest(item, set, clear);
}

void Widget::viewGroupHeaderContextPopupRequest(MessageList::Core::GroupHeaderItem *ghi, const QPoint &globalPos)
{
    Q_UNUSED(ghi);

    QMenu menu(this);

    QAction *act;

    QModelIndex index = view()->model()->index(ghi, 0);
    d->mGroupHeaderItemIndex = index;

    if (view()->isExpanded(index)) {
        act = menu.addAction(i18n("Collapse Group"));
        connect(act, &QAction::triggered, this, &Widget::slotCollapseItem);
    } else {
        act = menu.addAction(i18n("Expand Group"));
        connect(act, &QAction::triggered, this, &Widget::slotExpandItem);
    }

    menu.addSeparator();

    act = menu.addAction(i18n("Expand All Groups"));
    connect(act, &QAction::triggered,
            view(), &Core::View::slotExpandAllGroups);

    act = menu.addAction(i18n("Collapse All Groups"));
    connect(act, &QAction::triggered,
            view(), &Core::View::slotCollapseAllGroups);

    menu.exec(globalPos);
}

void Widget::viewDragEnterEvent(QDragEnterEvent *e)
{
    if (!canAcceptDrag(e)) {
        e->ignore();
        return;
    }

    e->accept();
}

void Widget::viewDragMoveEvent(QDragMoveEvent *e)
{
    if (!canAcceptDrag(e)) {
        e->ignore();
        return;
    }

    e->accept();
}

enum DragMode {
    DragCopy,
    DragMove,
    DragCancel
};

void Widget::viewDropEvent(QDropEvent *e)
{
    if (!canAcceptDrag(e)) {
        e->ignore();
        return;
    }

    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) {
        qCWarning(MESSAGELIST_LOG) << "Could not decode drag data!";
        e->ignore();
        return;
    }

    e->accept();

    int action;
    if ((e->possibleActions() & Qt::MoveAction) == 0) {     // We can't move anyway
        action = DragCopy;
    } else {
        action = DragCancel;
        int keybstate = QApplication::keyboardModifiers();
        if (keybstate & Qt::CTRL) {
            action = DragCopy;

        } else if (keybstate & Qt::SHIFT) {
            action = DragMove;

        } else {
            QMenu menu;
            QAction *moveAction = menu.addAction(QIcon::fromTheme(QStringLiteral("go-jump")), i18n("&Move Here"));
            QAction *copyAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("&Copy Here"));
            menu.addSeparator();
            menu.addAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), i18n("C&ancel"));

            QAction *menuChoice = menu.exec(QCursor::pos());
            if (menuChoice == moveAction) {
                action = DragMove;
            } else if (menuChoice == copyAction) {
                action = DragCopy;
            } else {
                action = DragCancel;
            }
        }
    }
    if (action == DragCancel) {
        return;
    }

    Collection::List collections = static_cast<const StorageModel *>(storageModel())->displayedCollections();
    Collection target = collections.at(0);
    Item::List items;
    items.reserve(urls.count());
    foreach (const QUrl &url, urls) {
        items << Item::fromUrl(url);
    }

    if (action == DragCopy) {
        new ItemCopyJob(items, target, this);
    } else if (action == DragMove) {
        new ItemMoveJob(items, target, this);
    }
}

void Widget::viewStartDragRequest()
{
    Collection::List collections = static_cast<const StorageModel *>(storageModel())->displayedCollections();

    if (collections.isEmpty()) {
        return;    // no folder here
    }

    QList<Core::MessageItem *> selection = view()->selectionAsMessageItemList();
    if (selection.isEmpty()) {
        return;
    }

    bool readOnly = false;

    foreach (const Collection &c, collections) {
        // We won't be able to remove items from this collection
        if ((c.rights() & Collection::CanDeleteItem) == 0) {
            // So the drag will be read-only
            readOnly = true;
            break;
        }
    }

    QList<QUrl> urls;
    urls.reserve(selection.count());
    Q_FOREACH (Core::MessageItem *mi, selection) {
        const Item i = d->itemForRow(mi->currentModelIndexRow());
        QUrl url = i.url(Item::Item::Item::UrlWithMimeType);
        url.addQueryItem(QStringLiteral("parent"), QString::number(mi->parentCollectionId()));
        urls << url;
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls(urls);

    QDrag *drag = new QDrag(view()->viewport());
    drag->setMimeData(mimeData);

    // Set pixmap
    QPixmap pixmap;
    if (selection.size() == 1) {
        pixmap = QPixmap(DesktopIcon(QStringLiteral("mail-message"), KIconLoader::SizeSmall));
    } else {
        pixmap = QPixmap(DesktopIcon(QStringLiteral("document-multiple"), KIconLoader::SizeSmall));
    }

    // Calculate hotspot (as in Konqueror)
    if (!pixmap.isNull()) {
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
        drag->setPixmap(pixmap);
    }

    if (readOnly) {
        drag->exec(Qt::CopyAction);
    } else {
        drag->exec(Qt::CopyAction | Qt::MoveAction);
    }
}

Item::List Widget::Private::selectionAsItems() const
{
    Item::List res;
    QList<Core::MessageItem *> selection = q->view()->selectionAsMessageItemList();
    res.reserve(selection.count());

    foreach (Core::MessageItem *mi, selection) {
        Item i = itemForRow(mi->currentModelIndexRow());
        Q_ASSERT(i.isValid());
        res << i;
    }

    return res;
}

Item Widget::Private::itemForRow(int row) const
{
    return static_cast<const StorageModel *>(q->storageModel())->itemForRow(row);
}

KMime::Message::Ptr Widget::Private::messageForRow(int row) const
{
    return static_cast<const StorageModel *>(q->storageModel())->messageForRow(row);
}

Item Widget::currentItem() const
{
    Core::MessageItem *mi = view()->currentMessageItem();

    if (mi == Q_NULLPTR) {
        return Item();
    }

    return d->itemForRow(mi->currentModelIndexRow());
}

KMime::Message::Ptr Widget::currentMessage() const
{
    Core::MessageItem *mi = view()->currentMessageItem();

    if (mi == Q_NULLPTR) {
        return KMime::Message::Ptr();
    }

    return d->messageForRow(mi->currentModelIndexRow());
}

QList<KMime::Message::Ptr > Widget::selectionAsMessageList(bool includeCollapsedChildren) const
{
    QList<KMime::Message::Ptr> lstMiPtr;
    QList<Core::MessageItem *> lstMi = view()->selectionAsMessageItemList(includeCollapsedChildren);
    if (lstMi.isEmpty()) {
        return lstMiPtr;
    }
    lstMiPtr.reserve(lstMi.count());
    foreach (Core::MessageItem *it, lstMi) {
        lstMiPtr.append(d->messageForRow(it->currentModelIndexRow()));
    }
    return lstMiPtr;
}

Akonadi::Item::List Widget::selectionAsMessageItemList(bool includeCollapsedChildren) const
{
    Akonadi::Item::List lstMiPtr;
    QList<Core::MessageItem *> lstMi = view()->selectionAsMessageItemList(includeCollapsedChildren);
    if (lstMi.isEmpty()) {
        return lstMiPtr;
    }
    lstMiPtr.reserve(lstMi.count());
    foreach (Core::MessageItem *it, lstMi) {
        lstMiPtr.append(d->itemForRow(it->currentModelIndexRow()));
    }
    return lstMiPtr;
}

QVector<qlonglong> Widget::selectionAsMessageItemListId(bool includeCollapsedChildren) const
{
    QVector<qlonglong> lstMiPtr;
    QList<Core::MessageItem *> lstMi = view()->selectionAsMessageItemList(includeCollapsedChildren);
    if (lstMi.isEmpty()) {
        return lstMiPtr;
    }
    lstMiPtr.reserve(lstMi.count());
    foreach (Core::MessageItem *it, lstMi) {
        lstMiPtr.append(d->itemForRow(it->currentModelIndexRow()).id());
    }
    return lstMiPtr;
}

QList<Akonadi::Item::Id> Widget::selectionAsListMessageId(bool includeCollapsedChildren) const
{
    QList<qlonglong> lstMiPtr;
    QList<Core::MessageItem *> lstMi = view()->selectionAsMessageItemList(includeCollapsedChildren);
    if (lstMi.isEmpty()) {
        return lstMiPtr;
    }
    lstMiPtr.reserve(lstMi.count());
    foreach (Core::MessageItem *it, lstMi) {
        lstMiPtr.append(d->itemForRow(it->currentModelIndexRow()).id());
    }
    return lstMiPtr;
}

Akonadi::Item::List Widget::currentThreadAsMessageList() const
{
    Akonadi::Item::List lstMiPtr;
    QList<Core::MessageItem *> lstMi = view()->currentThreadAsMessageItemList();
    if (lstMi.isEmpty()) {
        return lstMiPtr;
    }
    lstMiPtr.reserve(lstMi.count());
    foreach (Core::MessageItem *it, lstMi) {
        lstMiPtr.append(d->itemForRow(it->currentModelIndexRow()));
    }
    return lstMiPtr;
}

MessageList::Core::QuickSearchLine::SearchOptions Widget::currentOptions() const
{
    return view()->currentOptions();
}

QList<Akonadi::MessageStatus> Widget::currentFilterStatus() const
{
    return view()->currentFilterStatus();
}

QString Widget::currentFilterSearchString() const
{
    return view()->currentFilterSearchString();
}

bool Widget::isThreaded() const
{
    return view()->isThreaded();
}

bool Widget::selectionEmpty() const
{
    return view()->selectionEmpty();
}

bool Widget::getSelectionStats(
    Akonadi::Item::List &selectedItems,
    Akonadi::Item::List &selectedVisibleItems,
    bool *allSelectedBelongToSameThread,
    bool includeCollapsedChildren) const
{
    if (!storageModel()) {
        return false;
    }

    selectedItems.clear();
    selectedVisibleItems.clear();

    QList< Core::MessageItem * > selected = view()->selectionAsMessageItemList(includeCollapsedChildren);

    Core::MessageItem *topmost = Q_NULLPTR;

    *allSelectedBelongToSameThread = true;

    foreach (Core::MessageItem *it, selected) {
        const Item item = d->itemForRow(it->currentModelIndexRow());
        selectedItems.append(item);
        if (view()->isDisplayedWithParentsExpanded(it)) {
            selectedVisibleItems.append(item);
        }
        if (topmost == Q_NULLPTR) {
            topmost = (*it).topmostMessage();
        } else {
            if (topmost != (*it).topmostMessage()) {
                *allSelectedBelongToSameThread = false;
            }
        }
    }
    return true;
}

void Widget::deletePersistentSet(MessageList::Core::MessageItemSetReference ref)
{
    view()->deletePersistentSet(ref);
}

void Widget::markMessageItemsAsAboutToBeRemoved(MessageList::Core::MessageItemSetReference ref, bool bMark)
{
    QList< Core::MessageItem * > lstPersistent = view()->persistentSetCurrentMessageItemList(ref);
    if (!lstPersistent.isEmpty()) {
        view()->markMessageItemsAsAboutToBeRemoved(lstPersistent, bMark);
    }
}

Akonadi::Item::List Widget::itemListFromPersistentSet(MessageList::Core::MessageItemSetReference ref)
{
    Akonadi::Item::List lstItem;
    QList< Core::MessageItem * > refList = view()->persistentSetCurrentMessageItemList(ref);
    if (!refList.isEmpty()) {
        lstItem.reserve(refList.count());
        foreach (Core::MessageItem *it, refList) {
            lstItem.append(d->itemForRow(it->currentModelIndexRow()));
        }
    }
    return lstItem;
}

MessageList::Core::MessageItemSetReference Widget::selectionAsPersistentSet(bool includeCollapsedChildren) const
{
    QList<Core::MessageItem *> lstMi = view()->selectionAsMessageItemList(includeCollapsedChildren);
    if (lstMi.isEmpty()) {
        return -1;
    }
    return view()->createPersistentSet(lstMi);
}

MessageList::Core::MessageItemSetReference Widget::currentThreadAsPersistentSet() const
{
    QList<Core::MessageItem *> lstMi = view()->currentThreadAsMessageItemList();
    if (lstMi.isEmpty()) {
        return -1;
    }
    return view()->createPersistentSet(lstMi);
}

Akonadi::Collection Widget::currentCollection() const
{
    Collection::List collections = static_cast<const StorageModel *>(storageModel())->displayedCollections();
    if (collections.size() != 1) {
        return Akonadi::Collection();    // no folder here or too many (in case we can't decide where the drop will end)
    }
    return collections.first();
}

void Widget::slotCollapseItem()
{
    view()->setCollapseItem(d->mGroupHeaderItemIndex);
}

void Widget::slotExpandItem()
{
    view()->setExpandItem(d->mGroupHeaderItemIndex);
}
