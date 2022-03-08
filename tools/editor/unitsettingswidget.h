#pragma once

#include <QWidget>

class TechGraph;
struct Unit;

class CostWidget;
class BoostWidget;

class QLineEdit;
class QComboBox;

class UnitSettingsWidget : public QWidget
{
public:
    explicit UnitSettingsWidget(TechGraph *graph, QWidget *parent = nullptr);
    ~UnitSettingsWidget();

    void setUnit(const Unit *unit);

private:
    TechGraph *m_graph;
    const Unit *m_unit = nullptr;
    QLineEdit *m_name;
    QLineEdit *m_description;
    QComboBox *m_type;
    CostWidget *m_cost;
    CostWidget *m_yield;
    BoostWidget *m_boost;
};
