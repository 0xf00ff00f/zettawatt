#pragma once

#include <memory>
#include <string>
#include <vector>

using Real = long double;

class UIPainter;

struct StateVector {
    Real extropy = 0.0;
    Real energy = 0.0;
    Real materials = 0.0;
    Real carbon = 0.0;

    StateVector &operator+=(const StateVector &other)
    {
        extropy += other.extropy;
        energy += other.energy;
        materials += other.materials;
        carbon += other.carbon;
        return *this;
    }

    StateVector &operator*=(const StateVector &other)
    {
        extropy *= other.extropy;
        energy *= other.energy;
        materials *= other.materials;
        carbon *= other.carbon;
        return *this;
    }

    StateVector &operator*=(Real factor)
    {
        extropy *= factor;
        energy *= factor;
        materials *= factor;
        carbon *= factor;
        return *this;
    }
};

inline StateVector operator+(const StateVector &lhs, const StateVector &rhs)
{
    return { lhs.extropy + rhs.extropy, lhs.energy + rhs.energy, lhs.materials + rhs.materials, lhs.carbon + rhs.carbon };
}

inline StateVector operator*(const StateVector &lhs, const StateVector &rhs)
{
    return { lhs.extropy * rhs.extropy, lhs.energy * rhs.energy, lhs.materials * rhs.materials, lhs.carbon * rhs.carbon };
}

inline StateVector operator*(Real factor, const StateVector &rhs)
{
    return { factor * rhs.extropy, factor * rhs.energy, factor * rhs.materials, factor * rhs.carbon };
}

inline StateVector operator*(const StateVector &lhs, Real factor)
{
    return { lhs.extropy * factor, lhs.energy * factor, lhs.materials * factor, lhs.carbon * factor };
}

struct Booster {
    std::string name;
    std::string description;

    StateVector cost;
    StateVector boost;

    bool enabled = false;
};

struct Unit {
    std::string name;
    std::string description;

    StateVector cost;
    StateVector generated;

    struct Dependency {
        int count;
        const Unit *unit;
    };
    std::vector<Dependency> dependencies;
    std::vector<Booster> boosters;

    int count = 0;
};

class World
{
public:
    World();
    ~World();

    void update(double elapsed);
    void paint(UIPainter *painter) const;

private:
    void paintState(UIPainter *painter) const;
    void updateStateDelta();

    StateVector m_state;
    StateVector m_stateDelta;
    std::vector<std::unique_ptr<Unit>> m_techGraph;
};
