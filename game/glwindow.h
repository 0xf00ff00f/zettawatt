#pragma once

#include "noncopyable.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string_view>
#include <vector>

struct GLFWwindow;

namespace GX {

class GLWindow : private NonCopyable
{
public:
    GLWindow();
    virtual ~GLWindow();

    bool initialize(int width, int height, const char *title);
    void renderLoop();

    int width() const { return m_width; }
    int height() const { return m_height; }

    glm::vec2 cursorPosition() const;

    // minimumSeverity can be GL_DEBUG_SEVERITY_(NOTIFICATION|LOW|MEDIUM|HIGH)
    void enableGLDebugging(GLenum minimumSeverity = GL_DEBUG_SEVERITY_NOTIFICATION);
    void handleGLDebugMessage(GLenum source, GLenum type, GLenum severity, const std::string_view message) const;

protected:
    virtual void initializeGL() = 0;
    virtual void paintGL() = 0;
    virtual void update(double elapsed) = 0;
    virtual void mousePressEvent();
    virtual void mouseReleaseEvent();
    virtual void mouseMoveEvent(const glm::vec2 &position);
    virtual void keyPressEvent(int key);
    virtual void keyReleaseEvent(int key);

private:
    void initResources();

    static void sizeCallback(GLFWwindow *window, int width, int height);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPositionCallback(GLFWwindow *window, double x, double y);

    void sizeEvent(int width, int height);
    void keyEvent(int key, int scancode, int action, int mods);
    void mouseButtonEvent(int button, int action, int mods);
    void cursorPositionEvent(double x, double y);

    GLFWwindow *m_window = nullptr;
    int m_width;
    int m_height;
    bool m_initialized = false;
    int m_glDebugMinimumSeverity = 0;
};

} // namespace GX
