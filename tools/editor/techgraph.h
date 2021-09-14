#pragma once

#include <QObject>
#include <QPointF>

#include <memory>

class QJsonObject;

struct Unit {
    QString name;
    QString description;
    QPointF position;
    std::vector<const Unit *> dependencies;
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

    void removeUnit(const Unit *unit);

    void addDependency(const Unit *unit, const Unit *dependency);
    void removeDependency(const Unit *unit, const Unit *dependency);

    std::vector<const Unit *> units() const;

    QJsonObject save() const;
    void load(const QJsonObject &settings);

    void clear();

signals:
    void unitAdded(const Unit *unit);
    void unitChanged(const Unit *unit);
    void unitAboutToBeRemoved(const Unit *unit);
    void dependencyAdded(const Unit *unit, const Unit *dependency);
    void dependencyAboutToBeRemoved(const Unit *unit, const Unit *dependency);
    void graphReset();

private:
    template<typename FieldPtrT, typename ValueT>
    void mutateUnit(const Unit *unit, FieldPtrT fieldPtr, const ValueT &value);

    std::vector<std::unique_ptr<Unit>> m_units;
};
