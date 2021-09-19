#include "costwidget.h"

#include "techgraph.h"

#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>

BigNumberWidget::BigNumberWidget(QWidget *parent)
    : QWidget(parent)
    , m_mantissa(new QDoubleSpinBox(this))
    , m_exponent(new QComboBox(this))
{
    m_mantissa->setSingleStep(0.01);
    m_mantissa->setDecimals(1);
    m_mantissa->setRange(0, 1000);

    m_exponent->addItem(QStringLiteral(" "), 1.0);
    m_exponent->addItem(QStringLiteral("k"), 1.0e3);
    m_exponent->addItem(QStringLiteral("M"), 1.0e6);
    m_exponent->addItem(QStringLiteral("G"), 1.0e9);
    m_exponent->addItem(QStringLiteral("T"), 1.0e12);
    m_exponent->addItem(QStringLiteral("P"), 1.0e15);
    m_exponent->addItem(QStringLiteral("E"), 1.0e18);
    m_exponent->addItem(QStringLiteral("Z"), 1.0e21);
    m_exponent->addItem(QStringLiteral("Y"), 1.0e24);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_mantissa);
    layout->addWidget(m_exponent);

    connect(m_mantissa, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &BigNumberWidget::valueChanged);
    connect(m_exponent, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BigNumberWidget::valueChanged);
}

BigNumberWidget::~BigNumberWidget() = default;

double BigNumberWidget::value() const
{
    return m_mantissa->value() * m_exponent->currentData().toDouble();
}

void BigNumberWidget::setValue(double value)
{
    if (qFuzzyCompare(this->value(), value))
        return;

    int power = 0;
    while (value >= 1000) {
        value /= 1000;
        ++power;
    }

    {
        const QSignalBlocker sb(m_mantissa);
        m_mantissa->setValue(value);
    }

    {
        const QSignalBlocker sb(m_exponent);
        m_exponent->setCurrentIndex(power);
    }

    emit valueChanged();
}

CostWidget::CostWidget(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
    , m_extropy(new BigNumberWidget(this))
    , m_energy(new BigNumberWidget(this))
    , m_material(new BigNumberWidget(this))
    , m_carbon(new BigNumberWidget(this))
{
    auto *layout = new QFormLayout(this);
    layout->addRow(tr("Extropy"), m_extropy);
    layout->addRow(tr("Energy"), m_energy);
    layout->addRow(tr("Material"), m_material);
    layout->addRow(tr("Carbon"), m_carbon);

    connect(m_extropy, &BigNumberWidget::valueChanged, this, &CostWidget::valueChanged);
    connect(m_energy, &BigNumberWidget::valueChanged, this, &CostWidget::valueChanged);
    connect(m_material, &BigNumberWidget::valueChanged, this, &CostWidget::valueChanged);
    connect(m_carbon, &BigNumberWidget::valueChanged, this, &CostWidget::valueChanged);
}

CostWidget::~CostWidget() = default;

Cost CostWidget::value() const
{
    return Cost { m_extropy->value(), m_energy->value(), m_material->value(), m_carbon->value() };
}

void CostWidget::setValue(const Cost &value)
{
    if (this->value() == value)
        return;

    {
        const QSignalBlocker sb(m_extropy);
        m_extropy->setValue(value.extropy);
    }

    {
        const QSignalBlocker sb(m_energy);
        m_energy->setValue(value.energy);
    }

    {
        const QSignalBlocker sb(m_material);
        m_material->setValue(value.material);
    }

    {
        const QSignalBlocker sb(m_carbon);
        m_carbon->setValue(value.carbon);
    }

    emit valueChanged();
}
