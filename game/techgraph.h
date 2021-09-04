#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

using Real = long double;

struct StateVector {
    Real extropy = 0.0;
    Real energy = 0.0;
    Real material = 0.0;
    Real carbon = 0.0;

    StateVector &operator+=(const StateVector &other)
    {
        extropy += other.extropy;
        energy += other.energy;
        material += other.material;
        carbon += other.carbon;
        return *this;
    }

    StateVector &operator-=(const StateVector &other)
    {
        extropy -= other.extropy;
        energy -= other.energy;
        material -= other.material;
        carbon -= other.carbon;
        return *this;
    }

    StateVector &operator*=(const StateVector &other)
    {
        extropy *= other.extropy;
        energy *= other.energy;
        material *= other.material;
        carbon *= other.carbon;
        return *this;
    }

    StateVector &operator*=(Real factor)
    {
        extropy *= factor;
        energy *= factor;
        material *= factor;
        carbon *= factor;
        return *this;
    }
};

inline StateVector operator+(const StateVector &lhs, const StateVector &rhs)
{
    return { lhs.extropy + rhs.extropy, lhs.energy + rhs.energy, lhs.material + rhs.material, lhs.carbon + rhs.carbon };
}

inline StateVector operator-(const StateVector &lhs, const StateVector &rhs)
{
    return { lhs.extropy - rhs.extropy, lhs.energy - rhs.energy, lhs.material - rhs.material, lhs.carbon - rhs.carbon };
}

inline StateVector operator*(const StateVector &lhs, const StateVector &rhs)
{
    return { lhs.extropy * rhs.extropy, lhs.energy * rhs.energy, lhs.material * rhs.material, lhs.carbon * rhs.carbon };
}

inline StateVector operator*(Real factor, const StateVector &rhs)
{
    return { factor * rhs.extropy, factor * rhs.energy, factor * rhs.material, factor * rhs.carbon };
}

inline StateVector operator*(const StateVector &lhs, Real factor)
{
    return { lhs.extropy * factor, lhs.energy * factor, lhs.material * factor, lhs.carbon * factor };
}

struct Unit;

struct Project {
    std::string name;
    std::string description;

    glm::vec2 position;

    StateVector cost;
    StateVector boost;

    const Unit *parent;

    bool enabled = false;
};

struct Unit {
    std::string name;
    std::string description;

    glm::vec2 position;

    StateVector cost;
    StateVector yield;

    struct RequiredUnit {
        int count;
        const Unit *unit;
    };
    std::vector<RequiredUnit> requiredUnits;
    std::vector<const Project *> requiredProjects;

    int count = 0;
};

struct TechGraph {
    std::vector<std::unique_ptr<Unit>> units;
    std::vector<std::unique_ptr<Project>> projects;
};
