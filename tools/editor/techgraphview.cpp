#include "techgraphview.h"

#include "techgraph.h"

#include <QAction>
#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QStyleOptionGraphicsItem>

class UnitItem : public QGraphicsItem
{
public:
    UnitItem(TechGraphView *view, const Unit *unit, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    const Unit *unit() const { return m_unit; }

    QPointF sourcePosition() const;
    QPointF sinkPosition() const;

    enum { Type = UserType + 1 };

    int type() const override
    {
        return Type;
    }

    static constexpr auto Width = 100.0f;
    static constexpr auto Height = 50.0f;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    TechGraphView *m_view;
    const Unit *m_unit;
};

class ConnectorItem : public QGraphicsItem
{
public:
    enum class Direction {
        In,
        Out
    };

    explicit ConnectorItem(TechGraphView *view, UnitItem *unit, Direction direction, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    enum { Type = UserType + 2 };

    int type() const override
    {
        return Type;
    }

    static constexpr auto Radius = 6.0f;

private:
    TechGraphView *m_view;
    UnitItem *m_unit;
    Direction m_direction;
};

class ConnectionItem : public QGraphicsItem
{
public:
    ConnectionItem(TechGraphView *view, const Unit *source, const QPointF &pos, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void updateGeometry();

    void setSink(const Unit *sink);

    const Unit *source() const { return m_source; }
    const Unit *sink() const { return m_sink; }

    enum { Type = UserType + 3 };

    int type() const override
    {
        return Type;
    }

private:
    TechGraphView *m_view;

    QPointF m_from;
    QPointF m_to;

    const Unit *m_source = nullptr;
    const Unit *m_sink = nullptr;
};

ConnectionItem::ConnectionItem(TechGraphView *view, const Unit *source, const QPointF &pos, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_view(view)
    , m_source(source)
    , m_to(pos)
{
    setFlags(ItemIsSelectable);
    updateGeometry();
}

QRectF ConnectionItem::boundingRect() const
{
    const auto topLeft = QPointF(std::min(m_from.x(), m_to.x()), std::min(m_from.y(), m_to.y()));
    const auto bottomRight = QPointF(std::max(m_from.x(), m_to.x()), std::max(m_from.y(), m_to.y()));

    return QRectF(topLeft, bottomRight);
}

void ConnectionItem::updateGeometry()
{
    prepareGeometryChange();

    if (m_source)
        m_from = m_view->sourcePosition(m_source);

    if (m_sink)
        m_to = m_view->sinkPosition(m_sink);
}

void ConnectionItem::setSink(const Unit *sink)
{
    Q_ASSERT(m_sink == nullptr);
    m_sink = sink;
    updateGeometry();
}

void ConnectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * /* widget */)
{
    const QColor color = option->state & QStyle::State_Selected ? Qt::red : Qt::black;
    painter->setPen(color);
    painter->drawLine(m_from, m_to);
}

void ConnectionItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_sink == nullptr) {
        prepareGeometryChange();
        m_to = event->scenePos();
        return;
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void ConnectionItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_sink == nullptr) {
        ungrabMouse();
        m_view->connectToSink(this, event->scenePos());
        return;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

ConnectorItem::ConnectorItem(TechGraphView *view, UnitItem *unit, Direction direction, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_view(view)
    , m_unit(unit)
    , m_direction(direction)
{
}

QRectF ConnectorItem::boundingRect() const
{
    return QRectF(-Radius, -Radius, 2 * Radius, 2 * Radius);
}

void ConnectorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * /* widget */)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::red);
    painter->drawEllipse(boundingRect());
}

void ConnectorItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_direction == Direction::Out) {
        auto *connection = m_view->addConnection(m_unit->unit(), event->scenePos());
        connection->grabMouse();
    }
    event->accept();
}

UnitItem::UnitItem(TechGraphView *view, const Unit *unit, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_view(view)
    , m_unit(unit)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);

    auto *outConnector = new ConnectorItem(view, this, ConnectorItem::Direction::Out, this);
    outConnector->setPos(0, -25.0f);

    auto *inConnector = new ConnectorItem(view, this, ConnectorItem::Direction::In, this);
    inConnector->setPos(0, 25.0f);
}

QRectF UnitItem::boundingRect() const
{
    return QRectF(-0.5f * Width, -0.5f * Height, Width, Height);
}

void UnitItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * /* widget */)
{
    constexpr auto Border = 1.0f;

    painter->setPen(QPen(Qt::black, Border));

    QColor color = Qt::white;
    if (option->state & QStyle::State_Selected)
        color = color.darker(200);
    painter->setBrush(color);

    constexpr auto HalfBorder = 0.5f * Border;
    const auto rect = boundingRect().adjusted(HalfBorder, HalfBorder, -HalfBorder, -HalfBorder);
    painter->drawRect(rect);

    painter->drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, m_unit->name);
}

void UnitItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
}

QVariant UnitItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        QPointF position = value.toPointF();
        m_view->graph()->setUnitPosition(m_unit, position);
        m_view->updateConnections(m_unit);
    }
    return QGraphicsItem::itemChange(change, value);
}

QPointF UnitItem::sourcePosition() const
{
    return scenePos() + QPointF(0, -0.5f * Height);
}

QPointF UnitItem::sinkPosition() const
{
    return scenePos() + QPointF(0, 0.5f * Height);
}

TechGraphView::TechGraphView(TechGraph *graph, QWidget *parent)
    : QGraphicsView(parent)
    , m_graph(graph)
{
    setFocusPolicy(Qt::StrongFocus);
    setScene(new QGraphicsScene(QRectF(QPointF(-500, -500), QPointF(500, 500)), this));

    connect(scene(), &QGraphicsScene::selectionChanged, this, &TechGraphView::selectionChanged);

    connect(m_graph, &TechGraph::unitAdded, this, [this](const Unit *unit) {
        auto *item = new UnitItem(this, unit);
        scene()->addItem(item);
        m_unitItems[unit] = item;
    });

    connect(m_graph, &TechGraph::unitChanged, this, [this](const Unit *unit) {
        auto it = m_unitItems.find(unit);
        Q_ASSERT(it != m_unitItems.end());
        auto *item = it->second;
        if (unit->position != item->pos())
            item->setPos(unit->position);
        item->update();
    });

    connect(m_graph, &TechGraph::unitAboutToBeRemoved, this, [this](const Unit *unit) {
        removeConnections(unit);
        auto it = m_unitItems.find(unit);
        Q_ASSERT(it != m_unitItems.end());
        auto *item = it->second;
        delete item;
    });

    auto *deleteAction = new QAction(this);
    deleteAction->setShortcuts({ QKeySequence::Delete, Qt::Key_Backspace });
    connect(deleteAction, &QAction::triggered, this, [this] {
        // destroy connections
        for (auto *item : scene()->selectedItems()) {
            if (auto *connection = qgraphicsitem_cast<ConnectionItem *>(item)) {
                removeConnection(connection);
            }
        }
        // destroy units
        for (auto *item : scene()->selectedItems()) {
            if (auto *unit = qgraphicsitem_cast<UnitItem *>(item)) {
                removeUnit(unit);
            }
        }
    });
    addAction(deleteAction);
}

TechGraphView::~TechGraphView() = default;

std::vector<const Unit *> TechGraphView::selectedUnits() const
{
    const auto items = scene()->selectedItems();
    std::vector<const Unit *> units;
    for (const auto *item : items) {
        if (auto *unitItem = qgraphicsitem_cast<const UnitItem *>(item)) {
            units.push_back(unitItem->unit());
        }
    }
    return units;
}

ConnectionItem *TechGraphView::addConnection(const Unit *source, const QPointF &pos)
{
    auto *connection = new ConnectionItem(this, source, pos);
    connection->setZValue(-1);
    scene()->addItem(connection);
    m_unitConnections[source].push_back(connection);
    return connection;
}

QPointF TechGraphView::sinkPosition(const Unit *unit) const
{
    auto it = m_unitItems.find(unit);
    Q_ASSERT(it != m_unitItems.end());
    return it->second->sinkPosition();
}

QPointF TechGraphView::sourcePosition(const Unit *unit) const
{
    auto it = m_unitItems.find(unit);
    Q_ASSERT(it != m_unitItems.end());
    return it->second->sourcePosition();
}

void TechGraphView::connectToSink(ConnectionItem *connection, const QPointF &pos)
{
    auto it = std::find_if(m_unitItems.begin(), m_unitItems.end(), [&pos](auto &item) {
        return (item.second->sinkPosition() - pos).manhattanLength() < ConnectorItem::Radius;
    });
    if (it == m_unitItems.end()) {
        removeConnection(connection);
        return;
    }
    const auto *sink = it->first;
    connection->setSink(sink);
    m_unitConnections[sink].push_back(connection);
    m_graph->addDependency(sink, connection->source());
}

void TechGraphView::updateConnections(const Unit *unit)
{
    auto it = m_unitConnections.find(unit);
    if (it == m_unitConnections.end())
        return;
    auto &connections = it->second;
    for (auto *connection : connections)
        connection->updateGeometry();
}

void TechGraphView::removeConnections(const Unit *unit)
{
    auto it = m_unitConnections.find(unit);
    if (it == m_unitConnections.end())
        return;
    auto &connections = it->second;
    for (auto *connection : connections) {
        if (auto *source = connection->source(); source != unit)
            removeUnitConnection(source, connection);
        if (auto *sink = connection->sink(); sink != unit)
            removeUnitConnection(sink, connection);
    }
    qDeleteAll(connections);
    m_unitConnections.erase(it);
}

void TechGraphView::removeConnection(ConnectionItem *connection)
{
    removeUnitConnection(connection->source(), connection);

    if (connection->sink()) {
        removeUnitConnection(connection->sink(), connection);
        m_graph->removeDependency(connection->sink(), connection->source());
    }

    delete connection;
}

void TechGraphView::removeUnitConnection(const Unit *unit, ConnectionItem *connection)
{
    auto &connections = m_unitConnections[unit];
    auto it = std::find(connections.begin(), connections.end(), connection);
    Q_ASSERT(it != connections.end());
    connections.erase(it);
}

void TechGraphView::removeUnit(UnitItem *unit)
{
    m_graph->removeUnit(unit->unit());
}

void TechGraphView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier)) {
        auto *unit = m_graph->addUnit();
        m_graph->setUnitPosition(unit, mapToScene(event->pos()));
    }
    QGraphicsView::mousePressEvent(event);
}
