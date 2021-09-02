#pragma once

#include <noncopyable.h>
#include <shaderprogram.h>
#include <util.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace GX {
class TextureAtlas;
class FontCache;
class SpriteBatcher;
class ShaderManager;
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

    template<typename StringT>
    void drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const StringT &text);
    template<typename StringT>
    int horizontalAdvance(const StringT &text);

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
    GX::BoxF sceneBox() const { return m_sceneBox; }

private:
    void updateSceneBox(int width, int height);
    GX::BoxI textBoundingBox(const std::u32string &text);

    struct FontHasher {
        std::size_t operator()(const Font &font) const;
    };
    std::unordered_map<Font, std::unique_ptr<GX::FontCache>, FontHasher> m_fonts;
    std::unique_ptr<GX::SpriteBatcher> m_spriteBatcher;
    std::unique_ptr<GX::TextureAtlas> m_textureAtlas;
    GX::BoxF m_sceneBox = {};
    GX::FontCache *m_font = nullptr;
    glm::mat4 m_transform;
    std::vector<glm::mat4> m_transformStack;
};
