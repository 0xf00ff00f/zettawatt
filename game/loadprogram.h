#pragma once

#include <memory>

namespace GX::GL {
class ShaderProgram;
}

std::unique_ptr<GX::GL::ShaderProgram> loadProgram(const char *vertexShader, const char *geometryShader, const char *fragmentShader);
