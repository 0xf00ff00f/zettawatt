#pragma once

#include <memory>

namespace GX {

namespace GL {
class ShaderProgram;
}

std::unique_ptr<GL::ShaderProgram> loadProgram(const char *vertexShader, const char *fragmentShader);

} // namespace GX
