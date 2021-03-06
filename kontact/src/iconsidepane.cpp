/*
  This file is part of KDE Kontact.

  Copyright (C) 2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2008 Rafael Fernández López <ereslibre@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "iconsidepane.h"
#include "prefs.h"
using namespace Kontact;

#include <KontactInterface/Core>
#include <KontactInterface/Plugin>

#include <QAction>
#include <QIcon>
#include <KLocalizedString>
#include <KStringHandler>
#include <KIconLoader>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QCollator>
#include <QLayout>

namespace Kontact
{

class SelectionModel : public QItemSelectionModel
{
public:
    SelectionModel(QAbstractItemModel *model, QObject *parent)
        : QItemSelectionModel(model, parent)
    {
    }

public Q_SLOTS:
    void clear() Q_DECL_OVERRIDE {
        // Don't allow the current selection to be cleared. QListView doesn't call to this method
        // nowadays, but just to cover of future change of implementation, since QTreeView does call
        // to this one when clearing the selection.
    }

    void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) Q_DECL_OVERRIDE {
        // Don't allow the current selection to be cleared
        if (!index.isValid() && (command & QItemSelectionModel::Clear))
        {
            return;
        }
        QItemSelectionModel::select(index, command);
    }

    void select(const QItemSelection &selection,
                QItemSelectionModel::SelectionFlags command) Q_DECL_OVERRIDE {
        // Don't allow the current selection to be cleared
        if (!selection.count() && (command & QItemSelectionModel::Clear))
        {
            return;
        }
        QItemSelectionModel::select(selection, command);
    }
};

class Model : public QStringListModel
{
public:
    enum SpecialRoles {
        PluginName = Qt::UserRole
    };

    explicit Model(Navigator *parentNavigator = Q_NULLPTR)
        : QStringListModel(parentNavigator), mNavigator(parentNavigator)
    {
    }

    void emitReset()
    {
        //FIXME 
        //Q_EMIT reset();
    }

    void setPluginList(const QList<KontactInterface::Plugin *> &list)
    {
        pluginList = list;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        Qt::ItemFlags flags = QStringListModel::flags(index);

        flags &= ~Qt::ItemIsEditable;

        if (index.isValid()) {
            if (static_cast<KontactInterface::Plugin *>(index.internalPointer())->disabled()) {
                flags &= ~Qt::ItemIsEnabled;
                flags &= ~Qt::ItemIsSelectable;
                flags &= ~Qt::ItemIsDropEnabled;
            } else {
                flags |= Qt::ItemIsDropEnabled;
            }
        } else {
            flags &= ~Qt::ItemIsDropEnabled;
        }

        return flags;
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(parent);
        if (row < 0 || row >= pluginList.count()) {
            return QModelIndex();
        }
        return createIndex(row, column, pluginList[row]);
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE
    {
        if (!index.isValid() || !index.internalPointer()) {
            return QVariant();
        }

        if (role == Qt::DisplayRole) {
            if (!mNavigator->showText()) {
                return QVariant();
            }
            return static_cast<KontactInterface::Plugin *>(index.internalPointer())->title();
        } else if (role == Qt::DecorationRole) {
            if (!mNavigator->showIcons()) {
                return QVariant();
            }
            return QIcon::fromTheme(static_cast<KontactInterface::Plugin *>(index.internalPointer())->icon());
        } else if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        } else if (role == Qt::ToolTipRole) {
            if (!mNavigator->showText()) {
                return static_cast<KontactInterface::Plugin *>(index.internalPointer())->title();
            }
            return QVariant();
        } else if (role == PluginName) {
            return static_cast<KontactInterface::Plugin *>(index.internalPointer())->identifier();
        }
        return QStringListModel::data(index, role);
    }

private:
    QList<KontactInterface::Plugin *> pluginList;
    Navigator *mNavigator;
};

class SortFilterProxyModel
    : public QSortFilterProxyModel
{
public:
    explicit SortFilterProxyModel(QObject *parent = Q_NULLPTR): QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
        sort(0);
    }
protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE
    {
        KontactInterface::Plugin *leftPlugin =
            static_cast<KontactInterface::Plugin *>(left.internalPointer());
        KontactInterface::Plugin *rightPlugin =
            static_cast<KontactInterface::Plugin *>(right.internalPointer());

        if (leftPlugin->weight() == rightPlugin->weight()) {
            //Optimize it
            QCollator col;
            return col.compare(leftPlugin->title(), rightPlugin->title()) < 0;
        }

        return leftPlugin->weight() < rightPlugin->weight();
    }
};

class Delegate : public QStyledItemDelegate
{
public:
    explicit Delegate(Navigator *parentNavigator = Q_NULLPTR)
        : QStyledItemDelegate(parentNavigator), mNavigator(parentNavigator)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        if (!index.isValid() || !index.internalPointer()) {
            return;
        }

        QStyleOptionViewItem optionCopy(*static_cast<const QStyleOptionViewItem *>(&option));
        optionCopy.decorationPosition = QStyleOptionViewItem::Top;
        optionCopy.decorationSize = QSize(mNavigator->iconSize(), mNavigator->iconSize());
        optionCopy.textElideMode = Qt::ElideNone;
        QStyledItemDelegate::paint(painter, optionCopy, index);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        if (!index.isValid() || !index.internalPointer()) {
            return QSize();
        }

        QStyleOptionViewItem optionCopy(*static_cast<const QStyleOptionViewItem *>(&option));
        optionCopy.decorationPosition = QStyleOptionViewItem::Top;
        optionCopy.decorationSize =
            mNavigator->showIcons() ? QSize(mNavigator->iconSize(), mNavigator->iconSize()) : QSize();
        optionCopy.textElideMode = Qt::ElideNone;
        return QStyledItemDelegate::sizeHint(optionCopy, index);
    }

private:
    Navigator *mNavigator;
};

}

Navigator::Navigator(SidePaneBase *parent)
    : QListView(parent), mSidePane(parent)
{
    setViewport(new QWidget(this));

    setVerticalScrollMode(ScrollPerPixel);
    setHorizontalScrollMode(ScrollPerPixel);

    mIconSize = Prefs::self()->sidePaneIconSize();
    mShowIcons = Prefs::self()->sidePaneShowIcons();
    mShowText = Prefs::self()->sidePaneShowText();

    QActionGroup *viewMode = new QActionGroup(this);

    mShowIconsAction = new QAction(i18nc("@action:inmenu", "Show Icons Only"), this);
    mShowIconsAction->setCheckable(true);
    mShowIconsAction->setActionGroup(viewMode);
    mShowIconsAction->setChecked(!mShowText && mShowIcons);
    setHelpText(mShowIconsAction,
                i18nc("@info:status",
                      "Show sidebar items with icons and without text"));
    mShowIconsAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option if you want the sidebar items to have icons without text."));
    connect(mShowIconsAction, &QAction::triggered, this, &Navigator::slotActionTriggered);

    mShowTextAction = new QAction(i18nc("@action:inmenu", "Show Text Only"), this);
    mShowTextAction->setCheckable(true);
    mShowTextAction->setActionGroup(viewMode);
    mShowTextAction->setChecked(mShowText && !mShowIcons);
    setHelpText(mShowTextAction,
                i18nc("@info:status",
                      "Show sidebar items with text and without icons"));
    mShowTextAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option if you want the sidebar items to have text without icons."));
    connect(mShowTextAction, &QAction::triggered, this, &Navigator::slotActionTriggered);

    mShowBothAction = new QAction(i18nc("@action:inmenu", "Show Icons && Text"), this);
    mShowBothAction->setCheckable(true);
    mShowBothAction->setActionGroup(viewMode);
    mShowBothAction->setChecked(mShowText && mShowIcons);
    setHelpText(mShowBothAction,
                i18nc("@info:status",
                      "Show sidebar items with icons and text"));
    mShowBothAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option if you want the sidebar items to have icons and text."));
    connect(mShowBothAction, &QAction::triggered, this, &Navigator::slotActionTriggered);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);

    QActionGroup *iconSize = new QActionGroup(this);

    mBigIconsAction = new QAction(i18nc("@action:inmenu", "Big Icons"), this);
    mBigIconsAction->setCheckable(true);
    mBigIconsAction->setActionGroup(iconSize);
    mBigIconsAction->setChecked(mIconSize == KIconLoader::SizeLarge);
    setHelpText(mBigIconsAction,
                i18nc("@info:status",
                      "Show large size sidebar icons"));
    mBigIconsAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option if you want the sidebar icons to be extra big."));
    connect(mBigIconsAction, &QAction::triggered, this, &Navigator::slotActionTriggered);

    mNormalIconsAction = new QAction(i18nc("@action:inmenu", "Normal Icons"), this);
    mNormalIconsAction->setCheckable(true);
    mNormalIconsAction->setActionGroup(iconSize);
    mNormalIconsAction->setChecked(mIconSize == KIconLoader::SizeMedium);
    setHelpText(mNormalIconsAction,
                i18nc("@info:status",
                      "Show normal size sidebar icons"));
    mNormalIconsAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option if you want the sidebar icons to be normal size."));
    connect(mNormalIconsAction, &QAction::triggered, this, &Navigator::slotActionTriggered);

    mSmallIconsAction = new QAction(i18nc("@action:inmenu", "Small Icons"), this);
    mSmallIconsAction->setCheckable(true);
    mSmallIconsAction->setActionGroup(iconSize);
    mSmallIconsAction->setChecked(mIconSize == KIconLoader::SizeSmallMedium);
    setHelpText(mSmallIconsAction,
                i18nc("@info:status",
                      "Show small size sidebar icons"));
    mSmallIconsAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option if you want the sidebar icons to be extra small."));
    connect(mSmallIconsAction, &QAction::triggered, this, &Navigator::slotActionTriggered);

    QList<QAction *> actionList;
    actionList << mShowIconsAction << mShowTextAction << mShowBothAction << sep
               << mBigIconsAction << mNormalIconsAction << mSmallIconsAction;

    insertActions(Q_NULLPTR, actionList);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    setViewMode(ListMode);
    setItemDelegate(new Delegate(this));
    mModel = new Model(this);
    SortFilterProxyModel *sortFilterProxyModel = new SortFilterProxyModel;
    sortFilterProxyModel->setSourceModel(mModel);
    setModel(sortFilterProxyModel);
    setSelectionModel(new SelectionModel(sortFilterProxyModel, this));

    setDragDropMode(DropOnly);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            this, &Navigator::slotCurrentChanged);
}

void Navigator::updatePlugins(const QList<KontactInterface::Plugin *> &plugins_)
{
    QString currentPlugin;
    if (currentIndex().isValid()) {
        currentPlugin = currentIndex().model()->data(currentIndex(), Model::PluginName).toString();
    }

    QList<KontactInterface::Plugin *> pluginsToShow;
    foreach (KontactInterface::Plugin *plugin, plugins_) {
        if (plugin->showInSideBar()) {
            pluginsToShow << plugin;
        }
    }

    mModel->setPluginList(pluginsToShow);

    mModel->removeRows(0, mModel->rowCount());
    mModel->insertRows(0, pluginsToShow.count());

    // Restore the previous selected index, if any
    if (!currentPlugin.isEmpty()) {
        setCurrentPlugin(currentPlugin);
    }
}

void Navigator::setCurrentPlugin(const QString &plugin)
{
    const int numberOfRows(model()->rowCount());
    for (int i = 0; i < numberOfRows; ++i) {
        const QModelIndex index = model()->index(i, 0);
        const QString pluginName = model()->data(index, Model::PluginName).toString();

        if (plugin == pluginName) {
            selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
            break;
        }
    }
}

QSize Navigator::sizeHint() const
{
    //### TODO: We can cache this value, so this reply is faster. Since here we won't
    //          have too many elements, it is not that important. When caching this value
    //          make sure it is updated correctly when new rows have been added or
    //          removed. (ereslibre)

    int maxWidth = 0;
    const int numberOfRows(model()->rowCount());
    for (int i = 0; i < numberOfRows; ++i) {
        const QModelIndex index = model()->index(i, 0);
        maxWidth = qMax(maxWidth, sizeHintForIndex(index).width());
    }

    int viewHeight = QListView::sizeHint().height();

    QSize size(maxWidth + rect().width() - contentsRect().width(), viewHeight);
    return size;
}

void Navigator::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->proposedAction() == Qt::IgnoreAction) {
        return;
    }
    event->acceptProposedAction();
}

void Navigator::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->proposedAction() == Qt::IgnoreAction) {
        return;
    }

    const QModelIndex dropIndex = indexAt(event->pos());

    if (!dropIndex.isValid() ||
            !(dropIndex.model()->flags(dropIndex) & Qt::ItemIsEnabled)) {
        event->setAccepted(false);
        return;
    } else {
        const QModelIndex sourceIndex =
            static_cast<const QSortFilterProxyModel *>(model())->mapToSource(dropIndex);
        KontactInterface::Plugin *plugin =
            static_cast<KontactInterface::Plugin *>(sourceIndex.internalPointer());
        if (!plugin->canDecodeMimeData(event->mimeData())) {
            event->setAccepted(false);
            return;
        }
    }

    event->acceptProposedAction();
}

void Navigator::dropEvent(QDropEvent *event)
{
    if (event->proposedAction() == Qt::IgnoreAction) {
        return;
    }

    const QModelIndex dropIndex = indexAt(event->pos());

    if (!dropIndex.isValid()) {
        return;
    } else {
        const QModelIndex sourceIndex =
            static_cast<const QSortFilterProxyModel *>(model())->mapToSource(dropIndex);
        KontactInterface::Plugin *plugin =
            static_cast<KontactInterface::Plugin *>(sourceIndex.internalPointer());
        plugin->processDropEvent(event);
    }
}

void Navigator::setHelpText(QAction *act, const QString &text)
{
    act->setStatusTip(text);
    act->setToolTip(text);
    if (act->whatsThis().isEmpty()) {
        act->setWhatsThis(text);
    }
}

void Navigator::showEvent(QShowEvent *event)
{
    parentWidget()->setMaximumWidth(sizeHint().width());
    parentWidget()->setMinimumWidth(sizeHint().width());

    QListView::showEvent(event);
}

void Navigator::slotCurrentChanged(const QModelIndex &current)
{
    if (!current.isValid() || !current.internalPointer() ||
            !(current.model()->flags(current) & Qt::ItemIsEnabled)) {
        return;
    }

    QModelIndex source =
        static_cast<const QSortFilterProxyModel *>(current.model())->mapToSource(current);

    Q_EMIT pluginActivated(static_cast<KontactInterface::Plugin *>(source.internalPointer()));
}

void Navigator::slotActionTriggered(bool checked)
{
    QObject *object = sender();

    if (object == mShowIconsAction) {
        mShowIcons = checked;
        mShowText = !checked;
    } else if (object == mShowTextAction) {
        mShowIcons = !checked;
        mShowText = checked;
    } else if (object == mShowBothAction) {
        mShowIcons = checked;
        mShowText = checked;
    } else if (object == mBigIconsAction) {
        mIconSize = KIconLoader::SizeLarge;
    } else if (object == mNormalIconsAction) {
        mIconSize = KIconLoader::SizeMedium;
    } else if (object == mSmallIconsAction) {
        mIconSize = KIconLoader::SizeSmallMedium;
    }

    Prefs::self()->setSidePaneIconSize(mIconSize);
    Prefs::self()->setSidePaneShowIcons(mShowIcons);
    Prefs::self()->setSidePaneShowText(mShowText);

    mModel->emitReset();

    QTimer::singleShot(0, this, &Navigator::updateNavigatorSize);
}

void Navigator::updateNavigatorSize()
{
    parentWidget()->setMaximumWidth(sizeHint().width());
    parentWidget()->setMinimumWidth(sizeHint().width());
}

IconSidePane::IconSidePane(KontactInterface::Core *core, QWidget *parent)
    : SidePaneBase(core, parent)
{
    mNavigator = new Navigator(this);
    layout()->addWidget(mNavigator);
    mNavigator->setFocusPolicy(Qt::NoFocus);
    connect(mNavigator, &Navigator::pluginActivated, this, &IconSidePane::pluginSelected);
}

IconSidePane::~IconSidePane()
{
}

void IconSidePane::setCurrentPlugin(const QString &plugin)
{
    mNavigator->setCurrentPlugin(plugin);
}

void IconSidePane::updatePlugins()
{
    mNavigator->updatePlugins(core()->pluginList());
}

void IconSidePane::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    const int newWidth(mNavigator->sizeHint().width());
    setFixedWidth(newWidth);
    mNavigator->setFixedWidth(newWidth);
}

