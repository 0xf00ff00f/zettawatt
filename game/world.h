#pragma once

#include "techgraph.h"

#include <memory>
#include <string>
#include <vector>

class UIPainter;

class World
{
public:
    World();
    ~World();

    void update(double elapsed);
    void paint(UIPainter *painter) const;

private:
    void paintState(UIPainter *painter) const;
    void paintGraph(UIPainter *painter) const;
    void updateStateDelta();

    StateVector m_state;
    StateVector m_stateDelta;
    std::unique_ptr<TechGraph> m_techGraph;
};
