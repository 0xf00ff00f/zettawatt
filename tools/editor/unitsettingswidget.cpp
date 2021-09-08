#include "unitsettingswidget.h"

#include "techgraph.h"

#include <QFormLayout>
#include <QLineEdit>

UnitSettingsWidget::UnitSettingsWidget(TechGraph *graph, QWidget *widget)
    : QWidget(widget)
    , m_graph(graph)
    , m_name(new QLineEdit(this))
    , m_description(new QLineEdit(this))
{
    auto *layout = new QFormLayout(this);
    layout->addRow(tr("Name"), m_name);
    layout->addRow(tr("Description"), m_description);

    connect(m_name, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_unit)
            m_graph->setUnitName(m_unit, text);
    });
    connect(m_description, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_unit)
            m_graph->setUnitDescription(m_unit, text);
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
    }
    setEnabled(unit != nullptr);
}
