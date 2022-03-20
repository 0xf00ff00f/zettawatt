#pragma once

#include <QObject>
#include <QPointF>

#include <memory>

class QJsonObject;

struct Unit;

struct Cost {
    double extropy = 0.0;
    double energy = 0.0;
    double material = 0.0;
    double carbon = 0.0;

    bool operator==(const Cost &other) const
    {
        return qFuzzyCompare(extropy, other.extropy) && qFuzzyCompare(energy, other.energy) && qFuzzyCompare(material, other.material) && qFuzzyCompare(carbon, other.carbon);
    }

    Cost &operator+=(const Cost &other)
    {
        extropy += other.extropy;
        energy += other.energy;
        material += other.material;
        carbon += other.carbon;
        return *this;
    }
};

inline Cost operator+(const Cost &lhs, const Cost &rhs)
{
    return { lhs.extropy + rhs.extropy, lhs.energy + rhs.energy, lhs.material + rhs.material, lhs.carbon + rhs.carbon };
}

struct Boost {
    double factor = 1.0;
    const Unit *target = nullptr;

    bool operator==(const Boost &other) const
    {
        return qFuzzyCompare(factor, other.factor) && target == other.target;
    }
};

struct Unit {
    enum class Type {
        Generator,
        Booster
    };
    Q_ENUM(Type)

    QString name;
    QString description;
    Type type = Type::Generator;
    QPointF position;
    std::vector<const Unit *> dependencies;
    Cost cost;

    // if type == Type::Generator
    Cost yield;

    // if type == Type::Booster
    Boost boost;

    Q_GADGET
};

class TechGraph : public QObject
{
    Q_OBJECT
public:
    explicit TechGraph(QObject *parent = nullptr);
    ~TechGraph() override;

    const Unit *addUnit();

    void setUnitName(const Unit *unit, const QString &name);
    void setUnitDescription(const Unit *unit, const QString &name);
    void setUnitPosition(const Unit *unit, const QPointF &position);
    void setUnitCost(const Unit *unit, const Cost &cost);
    void setUnitYield(const Unit *unit, const Cost &cost);
    void setUnitType(const Unit *unit, Unit::Type type);
    void setUnitBoost(const Unit *unit, const Boost &boost);

    void removeUnit(const Unit *unit);

    void addDependency(const Unit *unit, const Unit *dependency);
    void removeDependency(const Unit *unit, const Unit *dependency);

    std::vector<const Unit *> units() const;
    const Unit *unit(int index) const;
    int unitCount() const;

    QJsonObject save() const;
    void load(const QJsonObject &settings);

    void clear();

    void autoAdjustCosts(const Cost &leafCost, const Cost &leafYield, double secondsPerUnit, double bumpPerUnit);

signals:
    void unitAboutToBeAdded(const Unit *unit);
    void unitAdded(const Unit *unit);
    void unitChanged(const Unit *unit);
    void unitAboutToBeRemoved(const Unit *unit);
    void unitRemoved();
    void dependencyAdded(const Unit *unit, const Unit *dependency);
    void dependencyAboutToBeRemoved(const Unit *unit, const Unit *dependency);
    void graphReset();

private:
    template<typename FieldPtrT, typename ValueT>
    void mutateUnit(const Unit *unit, FieldPtrT fieldPtr, const ValueT &value);

    std::vector<std::unique_ptr<Unit>> m_units;
};
