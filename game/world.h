#pragma once

#include "techgraph.h"

#include <util.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class UIPainter;
struct Unit;

class World;

class GraphItem
{
public:
    explicit GraphItem(World *world);
    virtual ~GraphItem();

    virtual glm::vec2 position() const = 0;
    virtual float radius() const = 0;
    virtual void update(double elapsed) = 0;
    virtual void paint(UIPainter *painter) const = 0;
    virtual bool contains(const glm::vec2 &pos) const = 0;

    void mousePressEvent(const glm::vec2 &pos);
    void mouseReleaseEvent(const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

    virtual void handleMousePress();
    virtual void handleMouseRelease();

protected:
    World *m_world;
    bool m_hovered = false;
};

class World
{
public:
    World();
    ~World();

    void initialize(TechGraph *techGraph);
    void reset();

    void update(double elapsed);
    void paint(UIPainter *painter) const;

    void mousePressEvent(const glm::vec2 &pos);
    void mouseReleaseEvent(const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

    bool unitClicked(Unit *unit);

private:
    void paintState(UIPainter *painter) const;
    void paintGraph(UIPainter *painter) const;
    void updateStateDelta();

    bool canAcquire(const Unit *unit) const;

    StateVector m_state;
    StateVector m_stateDelta;
    TechGraph *m_techGraph;
    std::vector<std::unique_ptr<GraphItem>> m_graphItems;
    std::unordered_map<const Unit *, const GraphItem *> m_unitItems;
    struct Edge {
        const GraphItem *from;
        const GraphItem *to;
    };
    std::vector<Edge> m_edges;
};
