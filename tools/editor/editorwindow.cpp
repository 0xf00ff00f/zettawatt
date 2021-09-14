#include "editorwindow.h"

#include "techgraph.h"
#include "techgraphview.h"
#include "unitsettingswidget.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_graph(new TechGraph(this))
    , m_graphView(new TechGraphView(m_graph, this))
    , m_unitSettings(new UnitSettingsWidget(m_graph, this))
{
    setCentralWidget(m_graphView);

    auto *dock = new QDockWidget(tr("Unit Settings"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(m_unitSettings);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(m_graphView, &TechGraphView::selectionChanged, this, [this] {
        const auto selected = m_graphView->selectedUnits();
        m_unitSettings->setUnit(selected.empty() ? nullptr : selected.front());
    });

    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *newAction = new QAction(tr("&New"), this);
    fileMenu->addAction(newAction);
    connect(newAction, &QAction::triggered, this, [this] {
        m_graph->clear();
    });

    auto *openAction = new QAction(tr("&Open..."), this);
    fileMenu->addAction(openAction);
    connect(openAction, &QAction::triggered, this, [this] {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Graph"));
        if (fileName.isEmpty())
            return;
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            const auto message = tr("Failed to open %1\n%2").arg(fileName, file.errorString());
            QMessageBox::warning(this, tr("Error"), message);
            return;
        }
        QJsonParseError error;
        const auto settings = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            const auto message = tr("Failed to parse %1").arg(fileName);
            QMessageBox::warning(this, tr("Error"), message);
            return;
        }
        m_graph->load(settings.object());
    });

    auto *saveAction = new QAction(tr("&Save..."), this);
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, [this] {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Graph"));
        if (fileName.isEmpty())
            return;
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly)) {
            const auto message = tr("Failed to open %1\n%2").arg(fileName, file.errorString());
            QMessageBox::warning(this, tr("Error"), message);
            return;
        }
        file.write(QJsonDocument(m_graph->save()).toJson(QJsonDocument::Indented));
        statusBar()->showMessage(tr("Saved %1").arg(fileName), 2000);
    });
}

EditorWindow::~EditorWindow() = default;
