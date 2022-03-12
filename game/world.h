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
    virtual glm::vec4 color() const = 0;
    virtual bool isVisible() const = 0;

    bool mousePressEvent(const glm::vec2 &pos);
    void mouseReleaseEvent(const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

    virtual bool handleMousePress();
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

    void setViewportSize(const glm::vec2 &viewportSize);
    void initialize(TechGraph *techGraph);
    void reset();

    void update(double elapsed);
    void paint(UIPainter *painter) const;

    void mousePressEvent(const glm::vec2 &pos);
    void mouseReleaseEvent(const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

    bool unitClicked(Unit *unit);

    bool canAcquire(const Unit *unit) const;

private:
    void paintState(UIPainter *painter) const;
    void paintGraph(UIPainter *painter) const;
    void updateStateDelta();

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
    glm::vec2 m_lastMousePosition;
    bool m_panningView = false;
    glm::vec2 m_viewOffset;
    glm::vec2 m_viewportSize;
};
