#pragma once

#include <noncopyable.h>
#include <shaderprogram.h>
#include <textureatlas.h>
#include <util.h>

#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace GX {
class FontCache;
class SpriteBatcher;
class ShaderManager;
class AbstractTexture;
} // namespace GX

class UIPainter : private GX::NonCopyable
{
public:
    explicit UIPainter(GX::ShaderManager *shaderManager);
    ~UIPainter();

    void resize(int width, int height);

    void startPainting();
    void donePainting();

    struct Font {
        std::string name;
        int pixelHeight;
        bool operator==(const Font &other) const
        {
            return name == other.name && pixelHeight == other.pixelHeight;
        }
    };
    void setFont(const Font &font);

    GX::PackedPixmap getPixmap(const std::string &name);

    template<typename StringT>
    void drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const StringT &text);

    glm::vec2 drawTextBox(const GX::BoxF &box, const glm::vec4 &color, int depth, const std::string &text);

    template<typename StringT>
    float horizontalAdvance(const StringT &text) const;

    void drawCircle(const glm::vec2 &center, float radius, const glm::vec4 &fillColor, const glm::vec4 &outlineColor, float outlineSize, int depth);
    void drawRoundedRect(const GX::BoxF &box, float radius, const glm::vec4 &fillColor, const glm::vec4 &outlineColor, float outlineSize, int depth);
    void drawThickLine(const glm::vec2 &from, const glm::vec2 &to, float thickness, const glm::vec4 &fromColor, const glm::vec4 &toColor, int depth);
    void drawGlowCircle(const glm::vec2 &center, float radius, const glm::vec4 &color, int depth);
    void drawPixmap(const glm::vec2 &pos, const GX::PackedPixmap &pixmap, int depth);

    void resetTransform();
    void scale(const glm::vec2 &s);
    void scale(float sx, float sy);
    void scale(float s);
    void translate(float dx, float dy);
    void translate(const glm::vec2 &p);
    void rotate(float angle);
    void saveTransform();
    void restoreTransform();

    GX::SpriteBatcher *spriteBatcher() const { return m_spriteBatcher.get(); }
    const GX::FontCache *font() const { return m_font; }
    GX::BoxF sceneBox() const { return m_sceneBox; }

    enum class VerticalAlign {
        Top,
        Middle,
        Bottom
    };
    void setVerticalAlign(VerticalAlign align);

    enum class HorizontalAlign {
        Left,
        Center,
        Right
    };
    void setHorizontalAlign(HorizontalAlign align);

private:
    struct Vertex {
        glm::vec2 position;
        glm::vec2 textureCoords;
    };
    void addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, int depth);
    void addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &color, int depth);
    void addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, int depth);
    void addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, const glm::vec4 &size, int depth);
    void addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, int depth);
    void addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &color, int depth);
    void addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, int depth);
    void addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, const glm::vec4 &size, int depth);

    void updateSceneBox(int width, int height);

    struct TextRow {
        std::string_view text;
        float width;
    };
    std::vector<TextRow> breakTextLines(const std::string &text, float lineWidth) const;

    struct FontHasher {
        std::size_t operator()(const Font &font) const;
    };
    std::unordered_map<Font, std::unique_ptr<GX::FontCache>, FontHasher> m_fonts;
    std::unordered_map<std::string, GX::PackedPixmap> m_pixmaps;
    std::unique_ptr<GX::SpriteBatcher> m_spriteBatcher;
    std::unique_ptr<GX::TextureAtlas> m_grayscaleTextureAtlas;
    std::unique_ptr<GX::TextureAtlas> m_rgbaTextureAtlas;
    GX::BoxF m_sceneBox = {};
    GX::FontCache *m_font = nullptr;
    glm::mat4 m_transform;
    std::vector<glm::mat4> m_transformStack;
    VerticalAlign m_verticalAlign = VerticalAlign::Top;
    HorizontalAlign m_horizontalAlign = HorizontalAlign::Left;
};
