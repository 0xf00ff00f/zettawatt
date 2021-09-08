#include "techgraph.h"

TechGraph::TechGraph(QObject *parent)
    : QObject(parent)
{
}

TechGraph::~TechGraph() = default;

const Unit *TechGraph::addUnit()
{
    m_units.emplace_back(new Unit);
    auto *unit = m_units.back().get();
    emit unitAdded(unit);
    return unit;
}

template<typename FieldPtrT, typename ValueT>
void TechGraph::mutateUnit(const Unit *unit, FieldPtrT fieldPtr, const ValueT &value)
{
    Q_ASSERT(std::any_of(m_units.begin(), m_units.end(), [unit](const auto &item) {
        return item.get() == unit;
    }));
    if (unit->*fieldPtr == value)
        return;
    const_cast<Unit *>(unit)->*fieldPtr = value;
    emit unitChanged(unit);
}

void TechGraph::setUnitName(const Unit *unit, const QString &name)
{
    mutateUnit(unit, &Unit::name, name);
}

void TechGraph::setUnitDescription(const Unit *unit, const QString &description)
{
    mutateUnit(unit, &Unit::description, description);
}

void TechGraph::setUnitPosition(const Unit *unit, const QPointF &position)
{
    mutateUnit(unit, &Unit::position, position);
}

void TechGraph::removeUnit(const Unit *unit)
{
    auto it = std::find_if(m_units.begin(), m_units.end(), [unit](const auto &item) {
        return item.get() == unit;
    });
    Q_ASSERT(it != m_units.end());
    emit unitAboutToBeRemoved(unit);
    m_units.erase(it);
}

void TechGraph::addDependency(const Unit *unit, const Unit *dependency)
{
    Q_ASSERT(std::any_of(m_units.begin(), m_units.end(), [unit](const auto &item) {
        return item.get() == unit;
    }));
    Q_ASSERT(std::any_of(m_units.begin(), m_units.end(), [dependency](const auto &item) {
        return item.get() == dependency;
    }));
    auto &dependencies = const_cast<Unit *>(unit)->dependencies;
    Q_ASSERT(std::find(dependencies.begin(), dependencies.end(), dependency) == dependencies.end());
    dependencies.push_back(dependency);
    emit dependencyAdded(unit, dependency);
}

void TechGraph::removeDependency(const Unit *unit, const Unit *dependency)
{
    Q_ASSERT(std::any_of(m_units.begin(), m_units.end(), [unit](const auto &item) {
        return item.get() == unit;
    }));
    Q_ASSERT(std::any_of(m_units.begin(), m_units.end(), [dependency](const auto &item) {
        return item.get() == dependency;
    }));
    auto &dependencies = const_cast<Unit *>(unit)->dependencies;
    auto it = std::find(dependencies.begin(), dependencies.end(), dependency);
    Q_ASSERT(it != dependencies.end());
    emit dependencyAboutToBeRemoved(unit, dependency);
    dependencies.erase(it);
}

std::vector<const Unit *> TechGraph::units() const
{
    std::vector<const Unit *> result;
    result.reserve(m_units.size());
    std::transform(m_units.begin(), m_units.end(), std::back_inserter(result), [](const auto &item) {
        return item.get();
    });
    return result;
}
