#include "techgraph.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>
#include <QTransform>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/circle_layout.hpp>
#include <boost/graph/kamada_kawai_spring_layout.hpp>

#include <algorithm>
#include <deque>
#include <unordered_set>

QDebug operator<<(QDebug debug, const Cost &cost)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "(extropy=" << cost.extropy << ", energy=" << cost.energy << ", material=" << cost.material << ", carbon=" << cost.carbon << ")";
    return debug;
}

TechGraph::TechGraph(QObject *parent)
    : QObject(parent)
{
}

TechGraph::~TechGraph() = default;

const Unit *TechGraph::addUnit()
{
    auto unit = std::make_unique<Unit>();
    emit unitAboutToBeAdded(unit.get());
    m_units.push_back(std::move(unit));
    auto *newUnit = m_units.back().get();
    emit unitAdded(newUnit);
    return newUnit;
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

void TechGraph::setUnitCost(const Unit *unit, const Cost &cost)
{
    mutateUnit(unit, &Unit::cost, cost);
}

void TechGraph::setUnitYield(const Unit *unit, const Cost &yield)
{
    mutateUnit(unit, &Unit::yield, yield);
}

void TechGraph::setUnitType(const Unit *unit, Unit::Type type)
{
    mutateUnit(unit, &Unit::type, type);
}

void TechGraph::setUnitBoost(const Unit *unit, const Boost &boost)
{
    mutateUnit(unit, &Unit::boost, boost);
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
    emit unitRemoved();
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

int TechGraph::unitCount() const
{
    return static_cast<int>(m_units.size());
}

const Unit *TechGraph::unit(int index) const
{
    if (index < 0 || index >= m_units.size())
        return nullptr;
    return m_units[index].get();
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

QString typeKey()
{
    return QLatin1String("type");
}

QString positionKey()
{
    return QLatin1String("position");
}

QString dependenciesKey()
{
    return QLatin1String("dependencies");
}

QString costKey()
{
    return QLatin1String("cost");
}

QString yieldKey()
{
    return QLatin1String("yield");
}

QString boostKey()
{
    return QLatin1String("boost");
}

QString extropyKey()
{
    return QLatin1String("extropy");
}

QString energyKey()
{
    return QLatin1String("energy");
}

QString materialKey()
{
    return QLatin1String("material");
}

QString carbonKey()
{
    return QLatin1String("carbon");
}

QString factorKey()
{
    return QLatin1String("factor");
}

QString targetKey()
{
    return QLatin1String("target");
}

template<typename T>
QString enumToString(T value)
{
    auto metaEnum = QMetaEnum::fromType<T>();
    return metaEnum.valueToKey(static_cast<int>(value));
}

template<typename T>
T stringToEnum(const QString &key)
{
    auto metaEnum = QMetaEnum::fromType<T>();
    return static_cast<T>(metaEnum.keyToValue(key.toLatin1().data()));
}

Cost loadCost(const QJsonObject &settings)
{
    return {
        settings[extropyKey()].toDouble(),
        settings[energyKey()].toDouble(),
        settings[materialKey()].toDouble(),
        settings[carbonKey()].toDouble()
    };
}

QJsonObject saveCost(const Cost &cost)
{
    QJsonObject settings;
    settings[extropyKey()] = cost.extropy;
    settings[energyKey()] = cost.energy;
    settings[materialKey()] = cost.material;
    settings[carbonKey()] = cost.carbon;
    return settings;
}

} // namespace

QJsonObject TechGraph::save() const
{
    std::unordered_map<const Unit *, int> unitIndices;
    for (size_t i = 0, count = m_units.size(); i < count; ++i)
        unitIndices[m_units[i].get()] = i;
    const auto unitIndex = [&unitIndices](const Unit *unit) {
        if (unit == nullptr)
            return -1;
        auto it = unitIndices.find(unit);
        Q_ASSERT(it != unitIndices.end());
        return it->second;
    };

    QJsonObject settings;

    QJsonArray unitsArray;
    for (const auto &unit : m_units) {
        QJsonObject unitSettings;
        unitSettings[nameKey()] = unit->name;
        unitSettings[descriptionKey()] = unit->description;
        unitSettings[typeKey()] = enumToString(unit->type);
        QJsonArray positionArray;
        positionArray.append(unit->position.x());
        positionArray.append(unit->position.y());
        unitSettings[positionKey()] = positionArray;
        unitSettings[costKey()] = saveCost(unit->cost);
        unitSettings[yieldKey()] = saveCost(unit->yield);
        unitSettings[boostKey()] = [&unitIndices, unitIndex, boost = unit->boost] {
            QJsonObject settings;
            settings[factorKey()] = boost.factor;
            settings[targetKey()] = unitIndex(boost.target);
            return settings;
        }();
        QJsonArray dependenciesArray;
        for (auto *dependency : unit->dependencies)
            dependenciesArray.append(unitIndex(dependency));
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
    auto unitFromIndex = [this](int index) -> const Unit * {
        if (index == -1)
            return nullptr;
        Q_ASSERT(index >= 0 && index < m_units.size());
        return m_units[index].get();
    };

    for (size_t i = 0; i < unitsCount; ++i) {
        const auto &unitSettings = unitsArray[i].toObject();
        auto &unit = m_units[i];
        unit->name = unitSettings[nameKey()].toString();
        unit->description = unitSettings[descriptionKey()].toString();
        unit->type = stringToEnum<Unit::Type>(unitSettings[typeKey()].toString());
        const auto positionArray = unitSettings[positionKey()].toArray();
        unit->position = QPointF(positionArray[0].toDouble(), positionArray[1].toDouble());
        unit->cost = loadCost(unitSettings[costKey()].toObject());
        unit->yield = loadCost(unitSettings[yieldKey()].toObject());
        unit->boost = [this, unitFromIndex, settings = unitSettings[boostKey()].toObject()] {
            const auto factor = settings[factorKey()].toDouble();
            const auto targetIndex = settings[targetKey()].toInt();
            return Boost { factor, unitFromIndex(targetIndex) };
        }();
        const auto dependenciesArray = unitSettings[dependenciesKey()].toArray();
        std::transform(dependenciesArray.begin(), dependenciesArray.end(), std::back_inserter(unit->dependencies), [unitFromIndex](const QJsonValue &value) {
            return unitFromIndex(value.toInt());
        });
    }

    emit graphReset();
}

void TechGraph::clear()
{
    m_units.clear();
    emit graphReset();
}

void TechGraph::autoAdjustCosts(const Cost &leafCost, const Cost &leafYield, double secondsPerUnit, double bumpPerUnit)
{
    // find successors
    std::unordered_map<const Unit *, std::unordered_set<const Unit *>> successors;
    for (const auto &unit : m_units)
        successors[unit.get()] = {};
    for (const auto &unit : m_units) {
        for (auto *pred : unit->dependencies)
            successors[pred].insert(unit.get());
    }

    // breadth-first search starting from the leaves

    std::unordered_set<const Unit *> visited;

    std::deque<const Unit *> queue;
    for (const auto &unit : m_units) {
        if (unit->dependencies.empty())
            queue.push_back(unit.get());
    }

    Cost expectedYield;
    while (!queue.empty()) {
        const auto *unit = queue.front();
        queue.pop_front();
        if (visited.find(unit) != visited.end())
            continue;
        visited.insert(unit);

        Cost cost;
        if (unit->cost.energy > 0.0)
            cost.energy = expectedYield.energy > 0.0 ? secondsPerUnit * expectedYield.energy : leafCost.energy;
        if (unit->cost.material > 0.0)
            cost.material = expectedYield.material > 0.0 ? secondsPerUnit * expectedYield.material : leafCost.material;
        if (unit->cost.extropy > 0.0)
            cost.extropy = expectedYield.extropy > 0.0 ? secondsPerUnit * expectedYield.extropy : leafCost.extropy;

        Cost yield;
        if (unit->type == Unit::Type::Generator) {
            if (unit->yield.energy > 0.0)
                yield.energy = expectedYield.energy > 0.0 ? bumpPerUnit * expectedYield.energy : leafYield.energy;
            if (unit->yield.material > 0.0)
                yield.material = expectedYield.material > 0.0 ? bumpPerUnit * expectedYield.material : leafYield.material;
            if (unit->yield.extropy > 0.0)
                yield.extropy = expectedYield.extropy > 0.0 ? bumpPerUnit * expectedYield.extropy : leafYield.extropy;
        }

        setUnitCost(unit, cost);
        setUnitYield(unit, yield);

        constexpr const auto MinUnitCount = 3.0;
        if (unit->type == Unit::Type::Generator) {
            expectedYield += MinUnitCount * unit->yield;
        } else {
            const auto &boost = unit->boost;
            if (boost.target)
                expectedYield += (boost.factor - 1.0) * MinUnitCount * boost.target->yield;
        }

        qDebug() << unit->name << "cost=" << unit->cost << "yield=" << unit->yield;

        const auto &next = successors[unit];
        std::copy(next.begin(), next.end(), std::back_inserter(queue));
    }
}

namespace boost {
enum vertex_position_t { vertex_position };
enum vertex_unit_t { vertex_unit };
BOOST_INSTALL_PROPERTY(vertex, position);
BOOST_INSTALL_PROPERTY(vertex, unit);
} // namespace boost

void TechGraph::autoLayout(float sideLength, float tolerance, bool resetPositions)
{
    using Position = boost::square_topology<>::point_type;
    using VertexProperties =
            boost::property<boost::vertex_position_t, Position,
                            boost::property<boost::vertex_unit_t, const Unit *>>;
    using EdgeProperties = boost::property<boost::edge_weight_t, double>;
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexProperties, EdgeProperties>;

    Graph graph(m_units.size());

    // add edges
    {
        std::unordered_map<const Unit *, size_t> vertexIndex;
        {
            size_t index = 0;
            for (const auto &unit : m_units)
                vertexIndex[unit.get()] = index++;
        }
        for (const auto [unit, index] : vertexIndex) {
            for (auto *pred : unit->dependencies)
                boost::add_edge(vertexIndex[pred], index, graph);
        }
    }

    // initialize unit property
    {
        auto indexMap = boost::get(boost::vertex_index, graph);
        auto unitMap = boost::get(boost::vertex_unit, graph);
        for (auto [it, end] = boost::vertices(graph); it != end; ++it)
            unitMap[*it] = m_units[indexMap[*it]].get();
    }

    // initialize weights to 1
    {
        auto weightMap = boost::get(boost::edge_weight, graph);
        for (auto [it, end] = boost::edges(graph); it != end; ++it)
            weightMap[*it] = 1.0;
    }

    // initialize positions
    if (resetPositions) {
        boost::circle_graph_layout(graph, boost::get(boost::vertex_position, graph), 0.5 * sideLength);
    } else {
        auto unitMap = boost::get(boost::vertex_unit, graph);
        auto positionMap = boost::get(boost::vertex_position, graph);
        for (auto [it, end] = boost::vertices(graph); it != end; ++it) {
            auto *unit = unitMap[*it];
            Position p;
            p[0] = unit->position.x();
            p[1] = unit->position.y();
            positionMap[*it] = p;
        }
    }

    // do the thing
    boost::kamada_kawai_spring_layout(graph,
                                      boost::get(boost::vertex_position, graph),
                                      boost::get(boost::edge_weight, graph),
                                      boost::square_topology<>(sideLength),
                                      boost::side_length(sideLength),
                                      boost::layout_tolerance<double>(tolerance));

    auto positionMap = boost::get(boost::vertex_position, graph);
    auto unitMap = boost::get(boost::vertex_unit, graph);
    for (auto [it, end] = boost::vertices(graph); it != end; ++it) {
        const auto *unit = unitMap[*it];
        const auto p = positionMap[*it];
        setUnitPosition(unit, QPointF(p[0], p[1]));
    }
}

void TechGraph::rotateAroundCenter(float angle)
{
    auto center = std::accumulate(m_units.begin(), m_units.end(), QPointF(0, 0), [](const QPointF &p, const auto &unit) {
        return p + unit->position;
    });
    center *= 1.0f / m_units.size();
    QTransform transform;
    transform.rotate(angle);
    for (auto &unit : m_units) {
        const auto p = center + transform.map(unit->position - center);
        setUnitPosition(unit.get(), p);
    }
}
