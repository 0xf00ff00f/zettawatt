#include <glwindow.h>

#include <shadermanager.h>

#include "uipainter.h"
#include "world.h"

class GameWindow : public GX::GLWindow
{
public:
    GameWindow() = default;
    ~GameWindow() override = default;

private:
    void initializeGL() override;
    void paintGL() override;
    void update(double elapsed) override;

    std::unique_ptr<GX::ShaderManager> m_shaderManager;
    std::unique_ptr<UIPainter> m_painter;
    World m_world;
};

void GameWindow::initializeGL()
{
    m_shaderManager = std::make_unique<GX::ShaderManager>();
    m_painter = std::make_unique<UIPainter>(m_shaderManager.get());
    m_painter->resize(width(), height());
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

int main()
{
    GameWindow w;
    w.initialize(1280, 720, "game");
    w.renderLoop();
}
