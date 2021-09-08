#include <QMainWindow>

class TechGraph;
class TechGraphView;
class UnitSettingsWidget;

class EditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override;

private:
    TechGraph *m_graph;
    TechGraphView *m_graphView;
    UnitSettingsWidget *m_unitSettings;
};
