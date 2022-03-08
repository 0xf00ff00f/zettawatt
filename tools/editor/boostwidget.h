#pragma once

#include <QGroupBox>

struct Boost;
class TechGraph;

class QDoubleSpinBox;
class QComboBox;

class BoostWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit BoostWidget(const TechGraph *graph, QWidget *parent = nullptr);
    ~BoostWidget() override;

    Boost value() const;
    void setValue(const Boost &value);

signals:
    void valueChanged();

private:
    QDoubleSpinBox *m_factor;
    QComboBox *m_target;
};
