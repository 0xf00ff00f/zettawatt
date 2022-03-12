#include "unitsettingswidget.h"

#include "boostwidget.h"
#include "costwidget.h"
#include "techgraph.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QVBoxLayout>

UnitSettingsWidget::UnitSettingsWidget(TechGraph *graph, QWidget *parent)
    : QWidget(parent)
    , m_graph(graph)
    , m_name(new QLineEdit(this))
    , m_description(new QLineEdit(this))
    , m_type(new QComboBox(this))
    , m_cost(new CostWidget(tr("Cost"), this))
    , m_yield(new CostWidget(tr("Yield"), this))
    , m_boost(new BoostWidget(graph, this))
{
    auto *layout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout;
    layout->addLayout(formLayout);

    formLayout->addRow(tr("Name"), m_name);
    formLayout->addRow(tr("Description"), m_description);
    formLayout->addRow(tr("Type"), m_type);

    layout->addWidget(m_cost);
    layout->addWidget(m_yield);
    layout->addWidget(m_boost);
    layout->addStretch();

    m_type->addItem(tr("Generator"), QVariant::fromValue(Unit::Type::Generator));
    m_type->addItem(tr("Booster"), QVariant::fromValue(Unit::Type::Booster));

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
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_unit)
            m_graph->setUnitType(m_unit, m_type->itemData(index).value<Unit::Type>());
    });
    connect(m_boost, &BoostWidget::valueChanged, this, [this]() {
        if (m_unit)
            m_graph->setUnitBoost(m_unit, m_boost->value());
    });
    const auto updateUI = [this] {
        auto type = m_type->currentData().value<Unit::Type>();
        switch (type) {
        case Unit::Type::Generator:
            m_yield->setEnabled(true);
            m_boost->setEnabled(false);
            break;
        case Unit::Type::Booster:
            m_yield->setEnabled(false);
            m_boost->setEnabled(true);
            break;
        }
    };
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, updateUI);
    updateUI();

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
        m_boost->setValue(unit->boost);
        m_type->setCurrentIndex(m_type->findData(QVariant::fromValue(unit->type)));
    }
    setEnabled(unit != nullptr);
}
