#include "glwindow.h"

#include <GLFW/glfw3.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include <array>

namespace GX {

namespace {

const char *glDebugSource(GLenum source)
{
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "APPLICATION";
    case GL_DEBUG_SOURCE_OTHER:
        return "OTHER";
    default:
        return "?";
    }
}

const char *glDebugType(GLenum type)
{
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER:
        return "MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "POP_GROUP";
    case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
    default:
        return "?";
    }
}

const char *glDebugSeverity(GLenum severity)
{
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
        return "LOW";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "MEDIUM";
    case GL_DEBUG_SEVERITY_HIGH:
        return "HIGH";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "NOTIFICATION";
    default:
        return "?";
    }
}

int glDebugSeverityIndex(GLenum severity)
{
    static const std::array severities = {
        GL_DEBUG_SEVERITY_NOTIFICATION,
        GL_DEBUG_SEVERITY_LOW,
        GL_DEBUG_SEVERITY_MEDIUM,
        GL_DEBUG_SEVERITY_HIGH,
    };
    auto it = std::find(severities.begin(), severities.end(), severity);
    if (it == severities.end())
        return -1;
    return std::distance(severities.begin(), it);
}

} // namespace

GLWindow::GLWindow()
{
    spdlog::cfg::load_env_levels();
}

GLWindow::~GLWindow()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool GLWindow::initialize(int width, int height, const char *title)
{
    m_width = width;
    m_height = height;

    glfwInit();
    glfwSetErrorCallback([](int error, const char *description) {
        spdlog::error("GLFW error %08x: %s\n", error, description);
    });

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 16);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, GLWindow::keyCallback);
    glfwSetMouseButtonCallback(m_window, GLWindow::mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, GLWindow::cursorPositionCallback);
    glfwSetWindowSizeCallback(m_window, GLWindow::sizeCallback);

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    auto error = glewInit();
    if (error != GLEW_OK) {
        spdlog::error("Failed to initialize GLEW: {}", glewGetErrorString(error));
        return false;
    }

    if (!glewIsSupported("GL_VERSION_4_2")) {
        spdlog::error("OpenGL 4.2 not supported");
        return false;
    }

    initializeGL();

    m_initialized = true;

    return true;
}

void GLWindow::enableGLDebugging(GLenum minimumSeverity)
{
    glEnable(GL_DEBUG_OUTPUT);
    m_glDebugMinimumSeverity = glDebugSeverityIndex(minimumSeverity);
    glDebugMessageCallback(
            [](GLenum source, GLenum type, GLuint /* id */, GLenum severity, GLsizei length, const GLchar *message,
               const void *user) {
                reinterpret_cast<const GLWindow *>(user)->handleGLDebugMessage(source, type, severity, std::string_view(message, length));
            },
            this);
}

void GLWindow::handleGLDebugMessage(GLenum source, GLenum type, GLenum severity, const std::string_view message) const
{
    if (glDebugSeverityIndex(severity) < m_glDebugMinimumSeverity)
        return;
    spdlog::info("OpenGL [source: {}, type: {}, severity: {}]: {}",
                 glDebugSource(source), glDebugType(type), glDebugSeverity(severity), message);
}

glm::vec2 GLWindow::cursorPosition() const
{
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    return { x, y };
}

void GLWindow::renderLoop()
{
    double curTime = glfwGetTime();
    while (!glfwWindowShouldClose(m_window)) {
        const auto now = glfwGetTime();
        const auto elapsed = now - curTime;
        curTime = now;

        update(elapsed);

        glViewport(0, 0, m_width, m_height);
        paintGL();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void GLWindow::mousePressEvent(int /* button */, const glm::vec2 & /* position */)
{
}

void GLWindow::mouseReleaseEvent(int /* button */, const glm::vec2 & /* position */)
{
}

void GLWindow::mouseMoveEvent(const glm::vec2 & /*position*/)
{
}

void GLWindow::keyPressEvent(int /* key */)
{
}

void GLWindow::keyReleaseEvent(int /* key */)
{
}

void GLWindow::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *gameWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
    gameWindow->keyEvent(key, scancode, action, mods);
}

void GLWindow::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    auto *gameWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
    gameWindow->mouseButtonEvent(button, action, mods);
}

void GLWindow::cursorPositionCallback(GLFWwindow *window, double x, double y)
{
    auto *gameWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
    gameWindow->cursorPositionEvent(x, y);
}

void GLWindow::sizeCallback(GLFWwindow *window, int width, int height)
{
    auto *gameWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
    gameWindow->sizeEvent(width, height);
}

void GLWindow::keyEvent(int key, int /*scancode*/, int action, int /*mods*/)
{
    switch (action) {
    case GLFW_PRESS:
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(m_window, 1);
        } else {
            keyPressEvent(key);
        }
        break;
    case GLFW_RELEASE:
        keyReleaseEvent(key);
        break;
    }
}

void GLWindow::mouseButtonEvent(int button, int action, int /*mods*/)
{
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);

    switch (action) {
    case GLFW_PRESS: {
        mousePressEvent(button, glm::vec2(x, y));
        break;
    }
    case GLFW_RELEASE: {
        mouseReleaseEvent(button, glm::vec2(x, y));
    }
    }
}

void GLWindow::cursorPositionEvent(double x, double y)
{
    mouseMoveEvent(glm::vec2(x, y));
}

void GLWindow::sizeEvent(int width, int height)
{
    m_width = width;
    m_height = height;
}

} // namespace GX
