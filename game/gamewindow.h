#pragma once

#include "noncopyable.h"

#include <glm/glm.hpp>

#include <memory>

namespace GX {
class ShaderManager;
}

class UIPainter;
class TechGraph;
class Theme;
class World;

enum class MouseButton {
    Left,
    Middle,
    Right,
    WheelUp,
    WheelDown,
    None,
};

class GameWindow : private GX::NonCopyable
{
public:
    GameWindow(int width, int height);
    ~GameWindow();

    void paintGL();
    void update(double elapsed);

    void mousePressEvent(MouseButton button, const glm::vec2 &pos);
    void mouseReleaseEvent(MouseButton button, const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

private:
    void initializeGL();
    glm::vec2 mapToScene(const glm::vec2 &windowPos) const;

    int m_width;
    int m_height;
    std::unique_ptr<TechGraph> m_techGraph;
    std::unique_ptr<Theme> m_theme;
    std::unique_ptr<GX::ShaderManager> m_shaderManager;
    std::unique_ptr<UIPainter> m_painter;
    std::unique_ptr<World> m_world;
};
