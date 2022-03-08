#include "boostwidget.h"

#include <QAbstractListModel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

#include "techgraph.h"

class UnitModel : public QAbstractListModel
{
public:
    static constexpr auto UnitRole = Qt::UserRole;

    explicit UnitModel(const TechGraph *graph, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_graph(graph)
    {
        auto unitRow = [this](const Unit *unit) {
            const auto units = m_graph->units();
            auto it = std::find(units.begin(), units.end(), unit);
            Q_ASSERT(it != units.end());
            return static_cast<int>(std::distance(units.begin(), it));
        };
        connect(graph, &TechGraph::unitAboutToBeAdded, this, [this] {
            const auto row = m_graph->unitCount();
            beginInsertRows({}, row, row);
        });
        connect(graph, &TechGraph::unitAdded, this, &UnitModel::endInsertRows);
        connect(graph, &TechGraph::unitAboutToBeRemoved, this, [this, unitRow](const Unit *unit) {
            const auto row = unitRow(unit);
            beginRemoveRows({}, row, row);
        });
        connect(graph, &TechGraph::unitRemoved, this, &UnitModel::endRemoveRows);
        connect(graph, &TechGraph::unitChanged, this, [this, unitRow](const Unit *unit) {
            const auto row = unitRow(unit);
            emit dataChanged(index(row, 0), index(row, 0));
        });
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return !parent.isValid() ? m_graph->unitCount() : 0;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!hasIndex(index.row(), index.column(), index.parent()))
            return {};
        switch (role) {
        case Qt::DisplayRole:
            return m_graph->unit(index.row())->name;
        case UnitRole:
            return QVariant::fromValue(m_graph->unit(index.row()));
        }
        return {};
    }

private:
    const TechGraph *m_graph;
};

BoostWidget::BoostWidget(const TechGraph *graph, QWidget *parent)
    : QGroupBox(tr("Boost"), parent)
    , m_factor(new QDoubleSpinBox(this))
    , m_target(new QComboBox(this))
{
    m_target->setModel(new UnitModel(graph, this));

    auto *layout = new QFormLayout(this);

    layout->addRow(tr("Factor"), m_factor);
    layout->addRow(tr("Target"), m_target);

    connect(m_factor, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &BoostWidget::valueChanged);
    connect(m_target, qOverload<int>(&QComboBox::currentIndexChanged), this, &BoostWidget::valueChanged);
}

BoostWidget::~BoostWidget() = default;

Boost BoostWidget::value() const
{
    return Boost { m_factor->value(), m_target->currentData(UnitModel::UnitRole).value<const Unit *>() };
}

void BoostWidget::setValue(const Boost &value)
{
    if (this->value() == value)
        return;

    {
        const QSignalBlocker sb(m_factor);
        m_factor->setValue(value.factor);
    }

    {
        const QSignalBlocker sb(m_target);
        m_target->setCurrentIndex(m_target->findData(QVariant::fromValue(value.target), UnitModel::UnitRole));
    }
}
