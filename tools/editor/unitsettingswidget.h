#pragma once

#include <QWidget>

class TechGraph;
struct Unit;

class CostWidget;

class QLineEdit;

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
    CostWidget *m_cost;
    CostWidget *m_yield;
};
