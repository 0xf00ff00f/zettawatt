#include "gamewindow.h"

#include "shadermanager.h"
#include "uipainter.h"

GameWindow::GameWindow(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_techGraph(std::make_unique<TechGraph>())
{
    m_techGraph->load("assets/data/techgraph.json");
    initializeGL();
}

GameWindow::~GameWindow() = default;

void GameWindow::initializeGL()
{
    m_shaderManager = std::make_unique<GX::ShaderManager>();
    m_painter = std::make_unique<UIPainter>(m_shaderManager.get());
    m_painter->resize(m_width, m_height);

    m_world.initialize(m_painter.get(), m_techGraph.get());
}

void GameWindow::paintGL()
{
    glViewport(0, 0, m_width, m_height);

    glClearColor(0.15, 0.15, 0.15, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_painter->startPainting();
    m_world.paint();
    m_painter->donePainting();
}

void GameWindow::update(double elapsed)
{
    m_world.update(elapsed);
}

void GameWindow::mousePressEvent(const glm::vec2 &pos)
{
    m_world.mousePressEvent(mapToScene(pos));
}

void GameWindow::mouseReleaseEvent(const glm::vec2 &pos)
{
    m_world.mouseReleaseEvent(mapToScene(pos));
}

void GameWindow::mouseMoveEvent(const glm::vec2 &pos)
{
    m_world.mouseMoveEvent(mapToScene(pos));
}

glm::vec2 GameWindow::mapToScene(const glm::vec2 &windowPos) const
{
    const GX::BoxF sceneBox = m_painter->sceneBox();
    const glm::vec2 p = windowPos / glm::vec2(m_width, m_height);
    return sceneBox.min + p * (sceneBox.max - sceneBox.min);
}
