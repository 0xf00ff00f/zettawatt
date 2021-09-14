#include "techgraph.h"

#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>

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
    for (auto &otherUnit : m_units) {
        if (otherUnit.get() == unit)
            continue;
        auto &dependencies = otherUnit->dependencies;
        dependencies.erase(std::remove(dependencies.begin(), dependencies.end(), unit), dependencies.end());
    }
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

namespace {
QString unitsKey()
{
    return QLatin1String("units");
}

QString nameKey()
{
    return QLatin1String("name");
}

QString descriptionKey()
{
    return QLatin1String("description");
}

QString positionKey()
{
    return QLatin1String("position");
}

QString dependenciesKey()
{
    return QLatin1String("dependencies");
}
} // namespace

QJsonObject TechGraph::save() const
{
    std::unordered_map<const Unit *, int> unitIndices;
    for (size_t i = 0, count = m_units.size(); i < count; ++i)
        unitIndices[m_units[i].get()] = i;

    QJsonObject settings;

    QJsonArray unitsArray;
    for (const auto &unit : m_units) {
        QJsonObject unitSettings;
        unitSettings[nameKey()] = unit->name;
        unitSettings[descriptionKey()] = unit->description;
        QJsonArray positionArray;
        positionArray.append(unit->position.x());
        positionArray.append(unit->position.y());
        unitSettings[positionKey()] = positionArray;
        QJsonArray dependenciesArray;
        for (auto *dependency : unit->dependencies)
            dependenciesArray.append(unitIndices[dependency]);
        unitSettings[dependenciesKey()] = dependenciesArray;
        unitsArray.append(unitSettings);
    }
    settings[unitsKey()] = unitsArray;

    return settings;
}

void TechGraph::load(const QJsonObject &settings)
{
    m_units.clear();

    const auto unitsArray = settings[unitsKey()].toArray();

    const auto unitsCount = unitsArray.size();

    m_units.reserve(unitsCount);
    std::generate_n(std::back_inserter(m_units), unitsCount, [] { return std::make_unique<Unit>(); });

    for (size_t i = 0; i < unitsCount; ++i) {
        const auto &unitSettings = unitsArray[i].toObject();
        auto &unit = m_units[i];
        unit->name = unitSettings[nameKey()].toString();
        unit->description = unitSettings[descriptionKey()].toString();
        const auto positionArray = unitSettings[positionKey()].toArray();
        unit->position = QPointF(positionArray[0].toDouble(), positionArray[1].toDouble());
        const auto dependenciesArray = unitSettings[dependenciesKey()].toArray();
        for (const auto &value : dependenciesArray) {
            const auto index = value.toInt();
            Q_ASSERT(index >= 0 && index < m_units.size());
            unit->dependencies.push_back(m_units[index].get());
        }
    }

    emit graphReset();
}

void TechGraph::clear()
{
    m_units.clear();
    emit graphReset();
}
