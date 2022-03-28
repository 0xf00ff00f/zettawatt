#include "autolayoutdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

AutoLayoutDialog::AutoLayoutDialog(QWidget *parent)
    : QDialog(parent)
    , m_sideLength(new QDoubleSpinBox(this))
    , m_tolerance(new QDoubleSpinBox(this))
    , m_resetPositions(new QCheckBox(tr("Reset positions?"), this))
{
    auto *layout = new QFormLayout(this);
    layout->addRow(tr("Side length"), m_sideLength);
    layout->addRow(tr("Tolerance"), m_tolerance);
    layout->addWidget(m_resetPositions);

    m_sideLength->setMinimum(100);
    m_sideLength->setMaximum(50000);
    m_sideLength->setValue(2000.0);
    m_tolerance->setDecimals(5);
    m_tolerance->setValue(1e-3);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AutoLayoutDialog::~AutoLayoutDialog() = default;

double AutoLayoutDialog::sideLength() const
{
    return m_sideLength->value();
}

double AutoLayoutDialog::tolerance() const
{
    return m_tolerance->value();
}

bool AutoLayoutDialog::resetPositions() const
{
    return m_resetPositions->isChecked();
}
