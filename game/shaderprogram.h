#pragma once

#include "noncopyable.h"

#include <GL/glew.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

namespace GX {
namespace GL {

class ShaderProgram : private NonCopyable
{
public:
    ShaderProgram();
    ~ShaderProgram();

    bool addShader(GLenum type, const std::string &path);
    bool addShaderSource(GLenum type, const GLchar *source);
    bool link();
    const std::string &log() const;

    void bind() const;

    int uniformLocation(std::string_view name) const;

    void setUniform(int location, int v) const;
    void setUniform(int location, float v) const;
    void setUniform(int location, const glm::vec2 &v) const;
    void setUniform(int location, const glm::vec3 &v) const;
    void setUniform(int location, const glm::vec4 &v) const;

    void setUniform(int location, const std::vector<float> &v) const;
    void setUniform(int location, const std::vector<glm::vec2> &v) const;
    void setUniform(int location, const std::vector<glm::vec3> &v) const;
    void setUniform(int location, const std::vector<glm::vec4> &v) const;

    void setUniform(int location, const glm::mat3 &mat) const;
    void setUniform(int location, const glm::mat4 &mat) const;

    template<typename T>
    void setUniform(std::string_view name, T &&value) const
    {
        setUniform(uniformLocation(name), value);
    }

private:
    GLuint m_id;
    std::string m_log;
};

} // namespace GL
} // namespace GX
