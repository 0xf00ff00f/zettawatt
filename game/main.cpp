#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <GL/glew.h>
#include <SDL/SDL.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>

#include "gamewindow.h"

using namespace std::string_literals;

namespace {
inline void panic(const char *fmt)
{
    std::printf("%s", fmt);
    std::abort();
}

template<typename... Args>
inline void panic(const char *fmt, const Args &...args)
{
    std::printf(fmt, args...);
    std::abort();
}

#ifndef __EMSCRIPTEN__
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
#endif

static std::unique_ptr<GameWindow> gameWindow;
} // namespace

static bool processEvents()
{
    SDL_Event event;
    const auto mouseButton = [&event] {
        switch (event.button.button) {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;
        case SDL_BUTTON_WHEELUP:
            return MouseButton::WheelUp;
        case SDL_BUTTON_WHEELDOWN:
            return MouseButton::WheelDown;
        default:
            return MouseButton::None;
        }
    };

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN: {
            gameWindow->mousePressEvent(mouseButton(), glm::vec2(event.button.x, event.button.y));
            break;
        }
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT)
                gameWindow->mouseReleaseEvent(mouseButton(), glm::vec2(event.button.x, event.button.y));
            break;
        case SDL_MOUSEMOTION:
            gameWindow->mouseMoveEvent(glm::vec2(event.motion.x, event.motion.y));
            break;
        case SDL_QUIT:
            return false;
        }
    }
    return true;
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("Video initialization failed: %s", SDL_GetError());
        return 1;
    }

    const SDL_VideoInfo *info = SDL_GetVideoInfo();
    if (!info) {
        panic("Video query failed: %s\n", SDL_GetError());
        return 1;
    }

    const int width = 1280;
    const int height = 720;
    const int bpp = info->vfmt->BitsPerPixel;
    const Uint32 flags = SDL_OPENGL;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    if (!SDL_SetVideoMode(width, height, bpp, flags)) {
        panic("Video mode set failed: %s\n", SDL_GetError());
        return 1;
    }

    auto error = glewInit();
    if (error != GLEW_OK) {
        panic("Failed to initialize GLEW: %s\n", glewGetErrorString(error));
        return 1;
    }

#ifndef __EMSCRIPTEN__
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
            [](GLenum source, GLenum type, GLuint /* id */, GLenum severity, GLsizei length, const GLchar *message,
               const void *user) {
                if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
                    spdlog::info("OpenGL [source: {}, type: {}, severity: {}]: {}",
                                 glDebugSource(source), glDebugType(type), glDebugSeverity(severity), message);
            },
            nullptr);
#endif

    gameWindow.reset(new GameWindow(width, height));

#ifdef __EMSCRIPTEN__
    emscripten_request_animation_frame_loop(
            [](double now, void *) -> EM_BOOL {
                const auto rv = processEvents();
                if (!rv)
                    return EM_FALSE;
                const auto elapsed = [now] {
                    static double last = 0;
                    const auto elapsed = last != 0 ? now - last : 0;
                    last = now;
                    return elapsed / 1000.0;
                }();
                gameWindow->update(elapsed);
                gameWindow->paintGL();
                return EM_TRUE;
            },
            nullptr);
#else
    while (processEvents()) {
        const auto elapsed = [] {
            static Uint32 last = 0;
            const Uint32 now = SDL_GetTicks();
            const Uint32 elapsed = last != 0 ? now - last : 0;
            last = now;
            return static_cast<double>(elapsed) / 1000.0;
        }();
        gameWindow->update(elapsed);
        gameWindow->paintGL();
        SDL_GL_SwapBuffers();
    }

    gameWindow.reset();

    SDL_Quit();
#endif
}
