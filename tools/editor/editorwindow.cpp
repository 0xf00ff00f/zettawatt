#include "editorwindow.h"

#include "techgraph.h"
#include "techgraphview.h"
#include "unitsettingswidget.h"

#include <QDockWidget>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_graph(new TechGraph(this))
    , m_graphView(new TechGraphView(m_graph, this))
    , m_unitSettings(new UnitSettingsWidget(m_graph, this))
{
    {
        const auto *unit = m_graph->addUnit();
        m_graph->setUnitName(unit, QStringLiteral("The quick brown fox"));
    }

    {
        const auto *unit = m_graph->addUnit();
        m_graph->setUnitName(unit, QStringLiteral("Sphinx of black quartz"));
    }

    setCentralWidget(m_graphView);

    auto *dock = new QDockWidget(tr("Unit Settings"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(m_unitSettings);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(m_graphView, &TechGraphView::selectionChanged, this, [this] {
        const auto selected = m_graphView->selectedUnits();
        m_unitSettings->setUnit(selected.empty() ? nullptr : selected.front());
    });
}

EditorWindow::~EditorWindow() = default;
