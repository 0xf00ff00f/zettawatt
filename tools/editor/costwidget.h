#pragma once

#include <QGroupBox>

struct Cost;

class QDoubleSpinBox;
class QComboBox;

class BigNumberWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BigNumberWidget(QWidget *parent = nullptr);
    ~BigNumberWidget() override;

    double value() const;
    void setValue(double value);

signals:
    void valueChanged();

private:
    QDoubleSpinBox *m_mantissa;
    QComboBox *m_exponent;
};

class CostWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit CostWidget(const QString &title, QWidget *parent = nullptr);
    ~CostWidget() override;

    Cost value() const;
    void setValue(const Cost &value);

signals:
    void valueChanged();

private:
    BigNumberWidget *m_extropy;
    BigNumberWidget *m_energy;
    BigNumberWidget *m_material;
    BigNumberWidget *m_carbon;
};
