#pragma once

#include <QGraphicsView>

#include <unordered_map>
#include <vector>

class TechGraph;
struct Unit;
class UnitItem;
class ConnectionItem;

class TechGraphView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit TechGraphView(TechGraph *graph, QWidget *parent = nullptr);
    ~TechGraphView() override;

    std::vector<const Unit *> selectedUnits() const;

    TechGraph *graph() const { return m_graph; }

    ConnectionItem *addConnection(const Unit *source, const QPointF &pos);
    void connectToSink(ConnectionItem *connection, const QPointF &pos);

    QPointF sinkPosition(const Unit *unit) const;
    QPointF sourcePosition(const Unit *unit) const;

    void updateConnections(const Unit *unit);

signals:
    void selectionChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void removeConnections(const Unit *unit);
    void removeConnection(ConnectionItem *item);
    void removeUnitConnection(const Unit *unit, ConnectionItem *connection);
    void removeUnit(UnitItem *item);

    TechGraph *m_graph;
    std::unordered_map<const Unit *, UnitItem *> m_unitItems;
    std::unordered_map<const Unit *, std::vector<ConnectionItem *>> m_unitConnections;
};
