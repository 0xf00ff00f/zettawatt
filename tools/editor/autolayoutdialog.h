#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QCheckBox;

class AutoLayoutDialog : public QDialog
{
public:
    explicit AutoLayoutDialog(QWidget *parent = nullptr);
    ~AutoLayoutDialog() override;

    double sideLength() const;
    double tolerance() const;
    bool resetPositions() const;

private:
    QDoubleSpinBox *m_sideLength;
    QDoubleSpinBox *m_tolerance;
    QCheckBox *m_resetPositions;
};
