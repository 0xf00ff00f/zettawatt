#pragma once

#include "abstracttexture.h"
#include "texture.h"

namespace GX {

struct Pixmap;

class LazyTexture : public AbstractTexture
{
public:
    LazyTexture(const Pixmap *pixmap);

    void markDirty();

    void bind() const;

    const Pixmap *pixmap() const;

private:
    const Pixmap *m_pixmap;
    GL::Texture m_texture;
    mutable bool m_dirty;
};

} // namespace GX
