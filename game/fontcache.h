#pragma once

#include "textureatlas.h"
#include "util.h"

#include <glm/glm.hpp>
#include <stb_truetype.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace GX {

struct Pixmap;

class FontCache
{
public:
    explicit FontCache(TextureAtlas *textureAtlas);
    ~FontCache();

    bool load(const std::string &ttfPath, int pixelHeight);

    struct Glyph {
        BoxI boundingBox;
        float advanceWidth;
        PackedPixmap pixmap;
    };
    const Glyph *getGlyph(int codepoint);

    template<typename StringT>
    int horizontalAdvance(const StringT &text);

private:
    std::unique_ptr<Glyph> initializeGlyph(int codepoint);
    Pixmap getCodepointPixmap(int codepoint) const;

    std::vector<unsigned char> m_ttfBuffer;
    stbtt_fontinfo m_font;
    TextureAtlas *m_textureAtlas;
    std::unordered_map<int, std::unique_ptr<Glyph>> m_glyphs;
    float m_scale = 0.0f;
    float m_ascent;
    float m_descent;
    float m_lineGap;
};

} // namespace GX
