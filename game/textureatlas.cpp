#include "textureatlas.h"
#include "pixmap.h"

#include <spdlog/spdlog.h>

namespace GX {

TextureAtlas::TextureAtlas(int pageWidth, int pageHeight, PixelType pixelType)
    : m_pageWidth(pageWidth)
    , m_pageHeight(pageHeight)
    , m_pixelType(pixelType)
{
}

TextureAtlas::~TextureAtlas() = default;

int TextureAtlas::pageWidth() const
{
    return m_pageWidth;
}

int TextureAtlas::pageHeight() const
{
    return m_pageHeight;
}

PixelType TextureAtlas::pixelType() const
{
    return m_pixelType;
}

std::optional<PackedPixmap> TextureAtlas::addPixmap(const Pixmap &pm)
{
    if (pm.pixelType != m_pixelType) {
        spdlog::warn("Invalid pixmap type for texture atlas");
        return std::nullopt;
    }

    if (pm.width > m_pageWidth || pm.height > m_pageHeight) {
        spdlog::warn("Pixmap too large for texture atlas");
        return std::nullopt;
    }

    std::optional<BoxF> textureCoords;
    LazyTexture *texture = nullptr;

    for (auto &entry : m_pages) {
        if ((textureCoords = entry->page.insert(pm))) {
            entry->texture.markDirty();
            texture = &entry->texture;
            break;
        }
    }

    if (!textureCoords) {
        m_pages.emplace_back(new PageTexture(m_pageWidth, m_pageHeight, m_pixelType));
        auto &entry = m_pages.back();
        textureCoords = entry->page.insert(pm);
        if (!textureCoords) {
            // shouldn't ever happen
            assert(false);
            return std::nullopt;
        }
        texture = &entry->texture;
    }

    PackedPixmap packedPixmap;
    packedPixmap.width = pm.width;
    packedPixmap.height = pm.height;
    packedPixmap.textureCoords = *textureCoords;
    packedPixmap.texture = texture;

    return packedPixmap;
}

int TextureAtlas::pageCount() const
{
    return m_pages.size();
}

const TextureAtlasPage &TextureAtlas::page(int index) const
{
    return m_pages[index]->page;
}

TextureAtlas::PageTexture::PageTexture(int width, int height, PixelType pixelType)
    : page(width, height, pixelType)
    , texture(page.pixmap())
{
}

} // namespace GX
