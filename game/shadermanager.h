#pragma once

#include "shaderprogram.h"

#include <array>
#include <memory>

namespace GX {

namespace GL {
class ShaderProgram;
};

class ShaderManager
{
public:
    ~ShaderManager();

    enum Program {
        Text,
        Circle,
        ThickLine,
        GlowCircle,
        NumPrograms
    };
    void useProgram(Program program);

    enum Uniform {
        ModelViewProjection,
        BaseColorTexture,
        NumUniforms
    };

    template<typename T>
    void setUniform(Uniform uniform, T &&value)
    {
        if (!m_currentProgram)
            return;
        const auto location = uniformLocation(uniform);
        if (location == -1)
            return;
        m_currentProgram->program->setUniform(location, std::forward<T>(value));
    }

private:
    int uniformLocation(Uniform uniform);

    struct CachedProgram {
        std::unique_ptr<GL::ShaderProgram> program;
        std::array<int, Uniform::NumUniforms> uniformLocations;
    };
    std::array<std::unique_ptr<CachedProgram>, Program::NumPrograms> m_cachedPrograms;
    CachedProgram *m_currentProgram = nullptr;
};

} // namespace GX
