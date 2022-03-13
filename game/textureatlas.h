#pragma once

#include "lazytexture.h"
#include "pixeltype.h"
#include "textureatlaspage.h"
#include "util.h"

#include <optional>
#include <vector>

namespace GX {

struct Pixmap;

struct PackedPixmap {
    int width = 0;
    int height = 0;
    BoxF textureCoords;
    const AbstractTexture *texture = nullptr;
};

class TextureAtlas
{
public:
    TextureAtlas(int pageWidth, int pageHeight, PixelType pixelType);
    ~TextureAtlas();

    int pageWidth() const;
    int pageHeight() const;
    PixelType pixelType() const;

    std::optional<PackedPixmap> addPixmap(const Pixmap &pixmap);

    int pageCount() const;
    const TextureAtlasPage &page(int index) const;

private:
    struct PageTexture {
        PageTexture(int width, int height, PixelType pixelType);
        TextureAtlasPage page;
        LazyTexture texture;
    };
    int m_pageWidth;
    int m_pageHeight;
    PixelType m_pixelType;
    std::vector<std::unique_ptr<PageTexture>> m_pages;
};

} // namespace GX
