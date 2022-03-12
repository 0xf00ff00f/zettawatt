#include "loadprogram.h"

#include "shaderprogram.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <vector>

namespace GX {

namespace {

std::string shaderPath(std::string_view basename)
{
    return std::string("assets/shaders/") + std::string(basename);
}

} // namespace

std::unique_ptr<GL::ShaderProgram>
loadProgram(const char *vertexShader, const char *fragmentShader)
{
    auto program = std::make_unique<GL::ShaderProgram>();
    if (!program->addShader(GL_VERTEX_SHADER, shaderPath(vertexShader))) {
        spdlog::warn("Failed to add vertex shader for program {}: {}", vertexShader, program->log());
        return {};
    }
    if (!program->addShader(GL_FRAGMENT_SHADER, shaderPath(fragmentShader))) {
        spdlog::warn("Failed to add fragment shader for program {}: {}", fragmentShader, program->log());
        return {};
    }
    if (!program->link()) {
        spdlog::warn("Failed to link program: {}", program->log());
        return {};
    }
    return program;
}

} // namespace GX
