#include "unitsettingswidget.h"

#include "costwidget.h"
#include "techgraph.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QVBoxLayout>

UnitSettingsWidget::UnitSettingsWidget(TechGraph *graph, QWidget *parent)
    : QWidget(parent)
    , m_graph(graph)
    , m_name(new QLineEdit(this))
    , m_description(new QLineEdit(this))
    , m_cost(new CostWidget(tr("Cost"), this))
    , m_yield(new CostWidget(tr("Yield"), this))
{
    auto *layout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout;
    layout->addLayout(formLayout);

    formLayout->addRow(tr("Name"), m_name);
    formLayout->addRow(tr("Description"), m_description);

    layout->addWidget(m_cost);
    layout->addWidget(m_yield);
    layout->addStretch();

    connect(m_name, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_unit)
            m_graph->setUnitName(m_unit, text);
    });
    connect(m_description, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_unit)
            m_graph->setUnitDescription(m_unit, text);
    });
    connect(m_cost, &CostWidget::valueChanged, this, [this]() {
        if (m_unit)
            m_graph->setUnitCost(m_unit, m_cost->value());
    });
    connect(m_yield, &CostWidget::valueChanged, this, [this]() {
        if (m_unit)
            m_graph->setUnitYield(m_unit, m_yield->value());
    });

    setEnabled(false);
}

UnitSettingsWidget::~UnitSettingsWidget() = default;

void UnitSettingsWidget::setUnit(const Unit *unit)
{
    m_unit = unit;
    if (unit) {
        m_name->setText(unit->name);
        m_description->setText(unit->description);
        m_cost->setValue(unit->cost);
        m_yield->setValue(unit->yield);
    }
    setEnabled(unit != nullptr);
}
