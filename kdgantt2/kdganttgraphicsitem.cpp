/****************************************************************************
 ** Copyright (C) 2001-2006 Klarälvdalens Datakonsult AB.  All rights reserved.
 **
 ** This file is part of the KD Gantt library.
 **
 ** This file may be distributed and/or modified under the terms of the
 ** GNU General Public License version 2 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.
 **
 ** Licensees holding valid commercial KD Gantt licenses may use this file in
 ** accordance with the KD Gantt Commercial License Agreement provided with
 ** the Software.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** See http://www.kdab.net/kdgantt for
 **   information about KD Gantt Commercial License Agreements.
 **
 ** Contact info@kdab.net if any conditions of this
 ** licensing are not clear to you.
 **
 **********************************************************************/
#include "kdganttgraphicsitem.h"
#include "kdganttgraphicsscene.h"
#include "kdganttgraphicsview.h"
#include "kdganttitemdelegate.h"
#include "kdganttconstraintgraphicsitem.h"
#include "kdganttconstraintmodel.h"
#include "kdganttabstractgrid.h"
#include "kdganttabstractrowcontroller.h"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <iterator>

#include <QPainter>
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QItemSelectionModel>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>

#include "kdgantt_debug.h"

/*!\class KDGantt::GraphicsItem
 * \internal
 */

using namespace KDGantt;

typedef QGraphicsItem BASE;

namespace
{
class Updater
{
    bool *u_ptr;
    bool oldval;
public:
    Updater(bool *u) : u_ptr(u), oldval(*u)
    {
        *u = true;
    }
    ~Updater()
    {
        *u_ptr = oldval;
    }
};
}

GraphicsItem::GraphicsItem(QGraphicsItem *parent, GraphicsScene *scene)
    : BASE(parent),  m_isupdating(false)
{
    if (scene && !parent) {
        scene->addItem(this);
    }

    if (scene && parent) {
        if (scene != parent->scene()) {
            qDebug("%s: parent scene is different than given scene", Q_FUNC_INFO);
        }
    }

    init();
}

GraphicsItem::GraphicsItem(const QModelIndex &idx, QGraphicsItem *parent,
                           GraphicsScene *scene)
    : BASE(parent),  m_index(idx), m_isupdating(false)
{
    if (scene && !parent) {
        scene->addItem(this);
    }

    if (scene && parent) {
        if (scene != parent->scene()) {
            qDebug("%s: parent scene is different than given scene", Q_FUNC_INFO);
        }
    }

    init();
}

GraphicsItem::~GraphicsItem()
{
}

void GraphicsItem::init()
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setFlags(ItemIsMovable | ItemIsSelectable | ItemIsFocusable);
    setAcceptsHoverEvents(true);
    setHandlesChildEvents(true);
    setZValue(100.);
    m_dragline = 0;
}

int GraphicsItem::type() const
{
    return Type;
}

StyleOptionGanttItem GraphicsItem::getStyleOption() const
{
    StyleOptionGanttItem opt;
    opt.itemRect = rect();
    opt.boundingRect = boundingRect();
    QVariant tp = m_index.model()->data(m_index, TextPositionRole);
    if (tp.isValid()) {
        opt.displayPosition = static_cast<StyleOptionGanttItem::Position>(tp.toInt());
    } else {
#if 0
        qCDebug(KDGANTT_LOG) << "Item" << m_index.model()->data(m_index, Qt::DisplayRole).toString()
                 << ", ends=" << m_endConstraints.size() << ", starts=" << m_startConstraints.size();
#endif
        opt.displayPosition = m_endConstraints.size() < m_startConstraints.size() ? StyleOptionGanttItem::Left : StyleOptionGanttItem::Right;
#if 0
        qCDebug(KDGANTT_LOG) << "choosing" << opt.displayPosition;
#endif
    }
    QVariant da = m_index.model()->data(m_index, Qt::TextAlignmentRole);
    if (da.isValid()) {
        opt.displayAlignment = static_cast< Qt::Alignment >(da.toInt());
    } else {
        switch (opt.displayPosition) {
        case StyleOptionGanttItem::Left: opt.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter; break;
        case StyleOptionGanttItem::Right: opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter; break;
        case StyleOptionGanttItem::Center: opt.displayAlignment = Qt::AlignCenter; break;
        }
    }
    opt.grid = scene()->grid();
    opt.text = m_index.model()->data(m_index, Qt::DisplayRole).toString();
    if (isEnabled()) {
        opt.state  |= QStyle::State_Enabled;
    }
    if (isSelected()) {
        opt.state |= QStyle::State_Selected;
    }
    if (hasFocus()) {
        opt.state   |= QStyle::State_HasFocus;
    }
    return opt;
}

GraphicsScene *GraphicsItem::scene() const
{
    return qobject_cast<GraphicsScene *>(QGraphicsItem::scene());
}

void GraphicsItem::setRect(const QRectF &r)
{
#if 0
    qCDebug(KDGANTT_LOG) << "GraphicsItem::setRect(" << r << "), txt=" << m_index.model()->data(m_index, Qt::DisplayRole).toString();
    if (m_index.model()->data(m_index, Qt::DisplayRole).toString() == QLatin1String("Code Freeze")) {
        qCDebug(KDGANTT_LOG) << "gotcha";
    }
#endif

    prepareGeometryChange();
    m_rect = r;
    updateConstraintItems();
    update();
}

void GraphicsItem::setBoundingRect(const QRectF &r)
{
    prepareGeometryChange();
    m_boundingrect = r;
    update();
}

bool GraphicsItem::isEditable() const
{
    return !scene()->isReadOnly() && m_index.model()->flags(m_index) & Qt::ItemIsEditable;
}

void GraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
    Q_UNUSED(widget);
    if (boundingRect().isValid() && scene()) {
        StyleOptionGanttItem opt = getStyleOption();
        *static_cast<QStyleOption *>(&opt) = *static_cast<const QStyleOption *>(option);
        //opt.fontMetrics = painter->fontMetrics();
        scene()->itemDelegate()->paintGanttItem(painter, opt, index());
    }
}

void GraphicsItem::setIndex(const QPersistentModelIndex &idx)
{
    m_index = idx;
    update();
}

QString GraphicsItem::ganttToolTip() const
{
    // TODO: Make delegate handle this
    const QAbstractItemModel *model = index().model();
    if (!model) {
        return QString();
    }
#if 0
    QString dbgstr;
    QDebug(&dbgstr) << m_index;
    return dbgstr;
#endif
    QString tip = model->data(index(), Qt::ToolTipRole).toString();
    if (!tip.isNull()) {
        return tip;
    } else return GraphicsScene::tr("%1 -> %2: %3")
                      .arg(model->data(index(), StartTimeRole).toString())
                      .arg(model->data(index(), EndTimeRole).toString())
                      .arg(model->data(index(), Qt::DisplayRole).toString());
}

QRectF GraphicsItem::boundingRect() const
{
    return m_boundingrect;
}

QPointF GraphicsItem::startConnector() const
{
    return mapToScene(m_rect.right(), m_rect.top() + m_rect.height() / 2.);
}

QPointF GraphicsItem::endConnector() const
{
    return mapToScene(m_rect.left(), m_rect.top() + m_rect.height() / 2.);
}

void GraphicsItem::constraintsChanged()
{
    if (!scene() || !scene()->itemDelegate()) {
        return;
    }
    const Span bs = scene()->itemDelegate()->itemBoundingSpan(getStyleOption(), index());
    const QRectF br = boundingRect();
    setBoundingRect(QRectF(bs.start(), 0., bs.length(), br.height()));
}

void GraphicsItem::addStartConstraint(ConstraintGraphicsItem *item)
{
    assert(item);
    m_startConstraints << item;
    item->setStart(startConnector());
    constraintsChanged();
}

void GraphicsItem::addEndConstraint(ConstraintGraphicsItem *item)
{
    assert(item);
    m_endConstraints << item;
    item->setEnd(endConnector());
    constraintsChanged();
}

void GraphicsItem::removeStartConstraint(ConstraintGraphicsItem *item)
{
    assert(item);
    m_startConstraints.removeAll(item);
    constraintsChanged();
}

void GraphicsItem::removeEndConstraint(ConstraintGraphicsItem *item)
{
    assert(item);
    m_endConstraints.removeAll(item);
    constraintsChanged();
}

void GraphicsItem::updateConstraintItems()
{
    QPointF s = startConnector();
    QPointF e = endConnector();
    {
        // Workaround for multiple definition error with MSVC6
        Q_FOREACH (ConstraintGraphicsItem *item, m_startConstraints) {
            item->setStart(s);
        }
    }
    {
        // Workaround for multiple definition error with MSVC6
        Q_FOREACH (ConstraintGraphicsItem *item, m_endConstraints) {
            item->setEnd(e);
        }
    }
}

void GraphicsItem::updateItem(const Span &rowGeometry, const QPersistentModelIndex &idx)
{
    //qCDebug(KDGANTT_LOG) << "GraphicsItem::updateItem("<<rowGeometry<<idx<<")";
    Updater updater(&m_isupdating);
    if (!idx.isValid() || idx.data(ItemTypeRole) == TypeMulti) {
        setRect(QRectF());
        hide();
        return;
    }

    const Span s = scene()->grid()->mapToChart(idx);
    setPos(QPointF(s.start(), rowGeometry.start()));
    setRect(QRectF(0., 0., s.length(), rowGeometry.length()));
    setIndex(idx);
    const Span bs = scene()->itemDelegate()->itemBoundingSpan(getStyleOption(), index());
    //qCDebug(KDGANTT_LOG) << "boundingSpan for" << getStyleOption().text << rect() << "is" << bs;
    setBoundingRect(QRectF(bs.start(), 0., bs.length(), rowGeometry.length()));
    const int maxh = scene()->rowController()->maximumItemHeight();
    if (maxh < rowGeometry.length()) {
        QRectF r = rect();
        const Qt::Alignment align = getStyleOption().displayAlignment;
        if (align & Qt::AlignTop) {
            // Do nothing
        } else if (align & Qt::AlignBottom) {
            r.setY(rowGeometry.length() - maxh);
        } else {
            // Center
            r.setY((rowGeometry.length() - maxh) / 2.);
        }
        r.setHeight(maxh);
        setRect(r);
    }

    //scene()->setSceneRect( scene()->sceneRect().united( mapToScene( boundingRect() ).boundingRect() ) );
    //updateConstraintItems();
}

QVariant GraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (!isUpdating() && change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        if (isEditable()) {
            newPos.setY(pos().y());
            return newPos;
        } else {
            return pos();
        }
    } else if (change == QGraphicsItem::ItemSelectedChange) {
        if (index().isValid() && !(index().model()->flags(index()) & Qt::ItemIsSelectable)) {
            // Reject selection attempt
            return qVariantFromValue(false);
        }

        if (value.toBool()) {
            scene()->selectionModel()->select(index(), QItemSelectionModel::Select);
        } else {
            scene()->selectionModel()->select(index(), QItemSelectionModel::Deselect);
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void GraphicsItem::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    scene()->selectionModel()->select(index(), QItemSelectionModel::SelectCurrent);
}

void GraphicsItem::updateModel()
{
    //qCDebug(KDGANTT_LOG) << "GraphicsItem::updateModel()";
    if (isEditable()) {
        QAbstractItemModel *model = const_cast<QAbstractItemModel *>(index().model());
        ConstraintModel *cmodel = scene()->constraintModel();
        assert(model);
        assert(cmodel);
        Q_UNUSED(cmodel);
        if (model) {
            //ItemType typ = static_cast<ItemType>( model->data( index(),
            //                                                   ItemTypeRole ).toInt() );
            const QModelIndex sourceIdx = scene()->summaryHandlingModel()->mapToSource(index());
            Q_UNUSED(sourceIdx);
            QList<Constraint> constraints;
            for (QList<ConstraintGraphicsItem *>::iterator it1 = m_startConstraints.begin() ;
                    it1 != m_startConstraints.end() ;
                    ++it1) {
                constraints.push_back((*it1)->proxyConstraint());
            }
            for (QList<ConstraintGraphicsItem *>::iterator it2 = m_endConstraints.begin() ;
                    it2 != m_endConstraints.end() ;
                    ++it2) {
                constraints.push_back((*it2)->proxyConstraint());
            }
            if (scene()->grid()->mapFromChart(Span(scenePos().x(), rect().width()),
                                              index(),
                                              constraints)) {
                scene()->updateRow(index().parent());
            }
        }
    }
}

void GraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (!isEditable()) {
        return;
    }
    StyleOptionGanttItem opt = getStyleOption();
    ItemDelegate::InteractionState istate = scene()->itemDelegate()->interactionStateFor(event->pos(), opt, index());
    switch (istate) {
    case ItemDelegate::State_ExtendLeft:
#ifndef QT_NO_CURSOR
        setCursor(Qt::SizeHorCursor);
#endif
        scene()->itemEntered(index());
        break;
    case ItemDelegate::State_ExtendRight:
#ifndef QT_NO_CURSOR
        setCursor(Qt::SizeHorCursor);
#endif
        scene()->itemEntered(index());
        break;
    case ItemDelegate::State_Move:
#ifndef QT_NO_CURSOR
        setCursor(Qt::SplitHCursor);
#endif
        scene()->itemEntered(index());
        break;
    default:
#ifndef QT_NO_CURSOR
        unsetCursor();
#endif
        break;
    };
}

void GraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
#ifndef QT_NO_CURSOR
    unsetCursor();
#endif
}

void GraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qCDebug(KDGANTT_LOG) << "GraphicsItem::mousePressEvent("<<event<<")";
    StyleOptionGanttItem opt = getStyleOption();
    m_istate = scene()->itemDelegate()->interactionStateFor(event->pos(), opt, index());
    m_presspos = event->pos();
    m_pressscenepos = event->scenePos();
    scene()->itemPressed(index());

    switch (m_istate) {
    case ItemDelegate::State_ExtendLeft:
    case ItemDelegate::State_ExtendRight:
    default: /* None and Move */
        BASE::mousePressEvent(event);
        break;
    }
}

void GraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    //qCDebug(KDGANTT_LOG) << "GraphicsItem::mouseReleaseEvent("<<event << ")";
    if (!m_presspos.isNull()) {
        scene()->itemClicked(index());
    }
    delete m_dragline; m_dragline = 0;
    if (scene()->dragSource()) {
        // Create a new constraint
        GraphicsItem *other = qgraphicsitem_cast<GraphicsItem *>(scene()->itemAt(event->scenePos()));
        if (other && scene()->dragSource() != other &&
                other->mapToScene(other->rect()).boundingRect().contains(event->scenePos())) {
            GraphicsView *view = qobject_cast<GraphicsView *>(event->widget()->parentWidget());
            if (view) {
                view->addConstraint(scene()->summaryHandlingModel()->mapToSource(scene()->dragSource()->index()),
                                    scene()->summaryHandlingModel()->mapToSource(other->index()), event->modifiers());
            }
        }
        scene()->setDragSource(0);
        //scene()->update();
    } else {
        if (isEditable()) {
            updateItemFromMouse(event->scenePos());

            // It is important to set m_presspos to null here because
            // when the sceneRect updates because we move the item
            // a MouseMoveEvent will be delivered, and we have to
            // protect against that
            m_presspos = QPointF();
            updateModel();
            // without this command we sometimes get a white area at the left side of a task item
            // after we moved that item right-ways into a grey weekend section of the scene:
            scene()->update();
        }
    }

    m_presspos = QPointF();
    BASE::mouseReleaseEvent(event);
}

void GraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    StyleOptionGanttItem opt = getStyleOption();
    ItemDelegate::InteractionState istate = scene()->itemDelegate()->interactionStateFor(event->pos(), opt, index());
    if (istate != ItemDelegate::State_None) {
        scene()->itemDoubleClicked(index());
    }
    BASE::mouseDoubleClickEvent(event);
}

void GraphicsItem::updateItemFromMouse(const QPointF &scenepos)
{
    //qCDebug(KDGANTT_LOG) << "GraphicsItem::updateItemFromMouse("<<scenepos<<")";
    const QPointF p = scenepos - m_presspos;
    QRectF r = rect();
    QRectF br = boundingRect();
    switch (m_istate) {
    case ItemDelegate::State_Move:
        setPos(p.x(), pos().y());
        break;
    case ItemDelegate::State_ExtendLeft: {
        const qreal brr = br.right();
        const qreal rr = r.right();
        const qreal delta = pos().x() - p.x();
        setPos(p.x(), QGraphicsItem::pos().y());
        br.setRight(brr + delta);
        r.setRight(rr + delta);
        break;
    }
    case ItemDelegate::State_ExtendRight: {
        const qreal rr = r.right();
        r.setRight(scenepos.x() - pos().x());
        br.setWidth(br.width() + r.right() - rr);
        break;
    }
    default: return;
    }
    setRect(r);
    setBoundingRect(br);
}

void GraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!isEditable()) {
        return;
    }
    if (m_presspos.isNull()) {
        return;
    }

    //qCDebug(KDGANTT_LOG) << "GraphicsItem::mouseMoveEvent("<<event<<"), m_istate="<< static_cast<ItemDelegate::InteractionState>( m_istate );
    //QPointF pos = event->pos() - m_presspos;
    switch (m_istate) {
    case ItemDelegate::State_ExtendLeft:
    case ItemDelegate::State_ExtendRight:
    case ItemDelegate::State_Move:
        // Check for constraint drag
        if (qAbs(m_pressscenepos.x() - event->scenePos().x()) < 10.
                && qAbs(m_pressscenepos.y() - event->scenePos().y()) > 5.) {
            m_istate = ItemDelegate::State_DragConstraint;
            m_dragline = new QGraphicsLineItem(this);
            m_dragline->setPen(QPen(Qt::DashLine));
            m_dragline->setLine(QLineF(rect().center(), event->pos()));
            scene()->addItem(m_dragline);
            scene()->setDragSource(this);
            break;
        }

        scene()->selectionModel()->setCurrentIndex(index(), QItemSelectionModel::Current);
        updateItemFromMouse(event->scenePos());
        //BASE::mouseMoveEvent(event);
        break;
    case ItemDelegate::State_DragConstraint: {
        QLineF line = m_dragline->line();
        m_dragline->setLine(QLineF(line.p1(), event->pos()));
        //QGraphicsItem* item = scene()->itemAt( event->scenePos() );
        break;
    }
    }
}
