#pragma once

#include "techgraph.h"

#include <util.h>

#include <memory>
#include <string>
#include <vector>

class UIPainter;
struct Unit;

class World
{
public:
    World();
    ~World();

    void update(double elapsed);
    void paint(UIPainter *painter) const;

    void mousePressEvent(const glm::vec2 &pos);
    void mouseReleaseEvent(const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

private:
    void paintState(UIPainter *painter) const;
    void paintGraph(UIPainter *painter) const;
    void updateStateDelta();

    void unitClicked(Unit *unit);
    bool canAcquire(const Unit *unit) const;

    StateVector m_state;
    StateVector m_stateDelta;
    std::unique_ptr<TechGraph> m_techGraph;
};
