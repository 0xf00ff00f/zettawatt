#include "shadermanager.h"

#include "loadprogram.h"

#include <type_traits>

#include <spdlog/spdlog.h>

namespace GX {

namespace {

std::string shaderPath(std::string_view basename)
{
    return std::string("assets/shaders/") + std::string(basename);
}

std::unique_ptr<GL::ShaderProgram>
loadProgram(ShaderManager::Program id)
{
    struct ProgramSource {
        const char *vertexShader;
        const char *geometryShader;
        const char *fragmentShader;
    };
    static const ProgramSource programSources[] = {
        { "text.vert", nullptr, "text.frag" }, // Text
    };
    static_assert(std::extent_v<decltype(programSources)> == ShaderManager::NumPrograms, "expected number of programs to match");

    const auto &sources = programSources[id];
    return GX::loadProgram(sources.vertexShader, sources.geometryShader, sources.fragmentShader);
}

} // namespace

ShaderManager::~ShaderManager() = default;

void ShaderManager::useProgram(Program id)
{
    auto &cachedProgram = m_cachedPrograms[id];
    if (!cachedProgram) {
        cachedProgram.reset(new CachedProgram);
        cachedProgram->program = loadProgram(id);
        auto &uniforms = cachedProgram->uniformLocations;
        std::fill(uniforms.begin(), uniforms.end(), -1);
    }
    if (cachedProgram.get() == m_currentProgram) {
        return;
    }
    if (cachedProgram->program) {
        cachedProgram->program->bind();
    }
    m_currentProgram = cachedProgram.get();
}

int ShaderManager::uniformLocation(Uniform id)
{
    if (!m_currentProgram || !m_currentProgram->program) {
        return -1;
    }
    auto location = m_currentProgram->uniformLocations[id];
    if (location == -1) {
        static constexpr const char *uniformNames[] = {
            // clang-format off
            "modelViewProjection",
            "baseColorTexture",
            // clang-format on
        };
        static_assert(std::extent_v<decltype(uniformNames)> == NumUniforms, "expected number of uniforms to match");

        location = m_currentProgram->program->uniformLocation(uniformNames[id]);
        m_currentProgram->uniformLocations[id] = location;
    }
    return location;
}

} // namespace GX
