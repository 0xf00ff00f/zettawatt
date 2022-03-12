#include <glwindow.h>

#include <shadermanager.h>

#include <GLFW/glfw3.h>

#include "techgraph.h"
#include "uipainter.h"
#include "world.h"

using namespace std::string_literals;

class GameWindow : public GX::GLWindow
{
public:
    GameWindow();
    ~GameWindow() override = default;

private:
    void initializeGL() override;
    void paintGL() override;
    void update(double elapsed) override;

    void mousePressEvent(int button, const glm::vec2 &pos) override;
    void mouseReleaseEvent(int button, const glm::vec2 &pos) override;
    void mouseMoveEvent(const glm::vec2 &pos) override;

    glm::vec2 mapToScene(const glm::vec2 &windowPos) const;

    std::unique_ptr<TechGraph> m_techGraph;
    std::unique_ptr<GX::ShaderManager> m_shaderManager;
    std::unique_ptr<UIPainter> m_painter;
    World m_world;
};

GameWindow::GameWindow()
    : m_techGraph(std::make_unique<TechGraph>())
{
    m_techGraph->load("assets/data/techgraph.json");
}

void GameWindow::initializeGL()
{
    m_shaderManager = std::make_unique<GX::ShaderManager>();
    m_painter = std::make_unique<UIPainter>(m_shaderManager.get());
    m_painter->resize(width(), height());

    m_world.setViewportSize(glm::vec2(width(), height()));
    m_world.initialize(m_techGraph.get());
}

void GameWindow::paintGL()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_painter->startPainting();
    m_world.paint(m_painter.get());
    m_painter->donePainting();
}

void GameWindow::update(double elapsed)
{
    m_world.update(elapsed);
}

void GameWindow::mousePressEvent(int button, const glm::vec2 &pos)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        m_world.mousePressEvent(mapToScene(pos));
}

void GameWindow::mouseReleaseEvent(int button, const glm::vec2 &pos)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        m_world.mouseReleaseEvent(mapToScene(pos));
}

void GameWindow::mouseMoveEvent(const glm::vec2 &pos)
{
    m_world.mouseMoveEvent(mapToScene(pos));
}

glm::vec2 GameWindow::mapToScene(const glm::vec2 &windowPos) const
{
    const GX::BoxF sceneBox = m_painter->sceneBox();
    const glm::vec2 p = windowPos / glm::vec2(width(), height());
    return sceneBox.min + p * (sceneBox.max - sceneBox.min);
}

int main()
{
    GameWindow w;
    w.initialize(1280, 720, "game");
    w.renderLoop();
}
