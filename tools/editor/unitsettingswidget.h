#pragma once

#include <QWidget>

class TechGraph;
struct Unit;

class QLineEdit;

class UnitSettingsWidget : public QWidget
{
public:
    explicit UnitSettingsWidget(TechGraph *graph, QWidget *widget);
    ~UnitSettingsWidget();

    void setUnit(const Unit *unit);

private:
    TechGraph *m_graph;
    const Unit *m_unit = nullptr;
    QLineEdit *m_name;
    QLineEdit *m_description;
};
