#include "texture.h"
#include "pixmap.h"

#include <memory>

namespace GX {
namespace GL {

namespace {
constexpr GLenum Target = GL_TEXTURE_2D;
}

Texture::Texture(const Pixmap &pixmap)
    : Texture(pixmap.width, pixmap.height, pixmap.pixelType, pixmap.pixels.data())
{
}

Texture::Texture(int width, int height, PixelType pixelType, const unsigned char *data)
    : m_width(width)
    , m_height(height)
    , m_format(pixelType == PixelType::RGBA ? GL_RGBA : GL_RED)
    , m_internalFormat(pixelType == PixelType::RGBA ? GL_RGBA8 : GL_R8)
{
    glGenTextures(1, &m_id);

    bind();

    glTexParameteri(Target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(Target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(Target, 0, m_internalFormat, m_width, m_height, 0, m_format, GL_UNSIGNED_BYTE, data);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_id);
}

void Texture::setData(const unsigned char *data) const
{
    bind();
    glTexSubImage2D(Target, 0, 0, 0, m_width, m_height, m_format, GL_UNSIGNED_BYTE, data);
}

void Texture::bind() const
{
    glBindTexture(Target, m_id);
}

} // namespace GL
} // namespace GX
