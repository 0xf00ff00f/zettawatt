#pragma once

#include <QDialog>

#include "techgraph.h"

class CostWidget;
class QDoubleSpinBox;

class AutoAdjustDialog : public QDialog
{
public:
    explicit AutoAdjustDialog(QWidget *parent = nullptr);
    ~AutoAdjustDialog() override;

    Cost leafCost() const;
    Cost leafYield() const;
    double secondsPerUnit() const;
    double bumpPerUnit() const;

private:
    CostWidget *m_leafCost;
    CostWidget *m_leafYield;
    QDoubleSpinBox *m_secondsPerUnit;
    QDoubleSpinBox *m_bumpPerUnit;
};
