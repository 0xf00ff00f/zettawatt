#include "autoadjustdialog.h"

#include "costwidget.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

AutoAdjustDialog::AutoAdjustDialog(QWidget *parent)
    : QDialog(parent)
    , m_leafCost(new CostWidget(tr("Leaf cost"), this))
    , m_leafYield(new CostWidget(tr("Leaf yield"), this))
    , m_secondsPerUnit(new QDoubleSpinBox(this))
    , m_bumpPerUnit(new QDoubleSpinBox(this))
{
    auto *layout = new QFormLayout(this);

    layout->addWidget(m_leafCost);
    layout->addWidget(m_leafYield);
    layout->addRow(tr("Seconds per unit"), m_secondsPerUnit);
    layout->addRow(tr("Yield bump per unit"), m_bumpPerUnit);

    m_leafCost->setValue(Cost { 100, 100, 100, 100 });
    m_leafYield->setValue(Cost { 100, 100, 100, 100 });
    m_secondsPerUnit->setValue(5);
    m_bumpPerUnit->setValue(1.2);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AutoAdjustDialog::~AutoAdjustDialog() = default;

Cost AutoAdjustDialog::leafCost() const
{
    return m_leafCost->value();
}

Cost AutoAdjustDialog::leafYield() const
{
    return m_leafYield->value();
}

double AutoAdjustDialog::secondsPerUnit() const
{
    return m_secondsPerUnit->value();
}

double AutoAdjustDialog::bumpPerUnit() const
{
    return m_bumpPerUnit->value();
}
