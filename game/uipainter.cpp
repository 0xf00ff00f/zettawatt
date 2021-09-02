#include "uipainter.h"

#include <fontcache.h>
#include <loadprogram.h>
#include <shadermanager.h>
#include <spritebatcher.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

namespace {
constexpr auto TextureAtlasPageSize = 512;

std::string fontPath(std::string_view basename)
{
    return std::string("assets/fonts/") + std::string(basename);
}
} // namespace

UIPainter::UIPainter(GX::ShaderManager *shaderManager)
    : m_spriteBatcher(new GX::SpriteBatcher(shaderManager))
    , m_textureAtlas(new GX::TextureAtlas(TextureAtlasPageSize, TextureAtlasPageSize, GX::PixelType::Grayscale))
{
}

UIPainter::~UIPainter() = default;

void UIPainter::resize(int width, int height)
{
    updateSceneBox(width, height);

    const auto projectionMatrix = glm::ortho(m_sceneBox.min.x, m_sceneBox.max.x, m_sceneBox.max.y, m_sceneBox.min.y, -1.0f, 1.0f);
    m_spriteBatcher->setTransformMatrix(projectionMatrix);
}

void UIPainter::startPainting()
{
    m_transformStack.clear();
    resetTransform();
    m_font = nullptr;
    m_spriteBatcher->startBatch();
}

void UIPainter::donePainting()
{
    m_spriteBatcher->renderBatch();
}

void UIPainter::setFont(const Font &font)
{
    auto it = m_fonts.find(font);
    if (it == m_fonts.end()) {
        auto fontCache = std::make_unique<GX::FontCache>(m_textureAtlas.get());
        const auto path = fontPath(font.name);
        if (!fontCache->load(path, font.pixelHeight)) {
            spdlog::error("Failed to load font {}", path);
        }
        it = m_fonts.emplace(font, std::move(fontCache)).first;
    }
    m_font = it->second.get();
}

template<typename StringT>
void UIPainter::drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const StringT &text)
{
    if (!m_font) {
        spdlog::warn("No font set lol");
        return;
    }

    glm::vec2 glyphPosition = pos;

    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Text);

    for (auto ch : text) {
        const auto glyph = m_font->getGlyph(ch);
        if (!glyph)
            continue;
        const auto p0 = glyphPosition + glm::vec2(glyph->boundingBox.min);
        const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);

        const auto &pixmap = glyph->pixmap;

        const auto &textureCoords = pixmap.textureCoords;
        const auto &t0 = textureCoords.min;
        const auto &t1 = textureCoords.max;

        const auto v0 = glm::vec2(m_transform * glm::vec4(p0.x, p0.y, 0, 1));
        const auto v1 = glm::vec2(m_transform * glm::vec4(p1.x, p0.y, 0, 1));
        const auto v2 = glm::vec2(m_transform * glm::vec4(p1.x, p1.y, 0, 1));
        const auto v3 = glm::vec2(m_transform * glm::vec4(p0.x, p1.y, 0, 1));

        const GX::SpriteBatcher::QuadVerts verts = {
            { { v0, { t0.x, t0.y }, color, glm::vec4(0) },
              { v1, { t1.x, t0.y }, color, glm::vec4(0) },
              { v2, { t1.x, t1.y }, color, glm::vec4(0) },
              { v3, { t0.x, t1.y }, color, glm::vec4(0) } }
        };

        m_spriteBatcher->addSprite(pixmap.texture, verts, depth);

        glyphPosition += glm::vec2(glyph->advanceWidth, 0);
    }
}

template void UIPainter::drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const std::u32string &text);
template void UIPainter::drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const std::string &text);

template<typename StringT>
int UIPainter::horizontalAdvance(const StringT &text)
{
    return m_font->horizontalAdvance(text);
}

template int UIPainter::horizontalAdvance(const std::u32string &text);
template int UIPainter::horizontalAdvance(const std::string &text);

void UIPainter::drawCircle(const glm::vec2 &center, float radius, const glm::vec4 &color, int depth)
{
    const auto &p0 = center - glm::vec2(radius, radius);
    const auto &p1 = center + glm::vec2(radius, radius);

    const auto verts = GX::SpriteBatcher::QuadVerts {
        { { { p0.x, p0.y }, { 0.0f, 0.0f }, color, { 2.0f * radius, 0, 0, 0 } },
          { { p1.x, p0.y }, { 1.0f, 0.0f }, color, { 2.0f * radius, 0, 0, 0 } },
          { { p1.x, p1.y }, { 1.0f, 1.0f }, color, { 2.0f * radius, 0, 0, 0 } },
          { { p0.x, p1.y }, { 0.0f, 1.0f }, color, { 2.0f * radius, 0, 0, 0 } } }
    };

    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Circle);
    m_spriteBatcher->addSprite(nullptr, verts, depth);
}

void UIPainter::drawRoundedRect(const glm::vec2 &min, const glm::vec2 &max, float radius, const glm::vec4 &color, int depth)
{
    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Circle);

    const auto drawPatch = [this, radius, &color, depth](const glm::vec2 &p0, const glm::vec2 &p1, const glm::vec2 &t0, const glm::vec2 &t1) {
        const auto verts = GX::SpriteBatcher::QuadVerts {
            { { { p0.x, p0.y }, { t0.x, t0.y }, color, { 2.0f * radius, 0, 0, 0 } },
              { { p1.x, p0.y }, { t1.x, t0.y }, color, { 2.0f * radius, 0, 0, 0 } },
              { { p1.x, p1.y }, { t1.x, t1.y }, color, { 2.0f * radius, 0, 0, 0 } },
              { { p0.x, p1.y }, { t0.x, t1.y }, color, { 2.0f * radius, 0, 0, 0 } } }
        };
        m_spriteBatcher->addSprite(nullptr, verts, depth);
    };

    const auto x0 = min.x;
    const auto x1 = min.x + radius;
    const auto x2 = max.x - radius;
    const auto x3 = max.x;

    const auto y0 = min.y;
    const auto y1 = min.y + radius;
    const auto y2 = max.y - radius;
    const auto y3 = max.y;

    drawPatch(glm::vec2(x0, y0), glm::vec2(x1, y1), glm::vec2(0.0, 0.0), glm::vec2(0.5, 0.5));
    drawPatch(glm::vec2(x1, y0), glm::vec2(x2, y1), glm::vec2(0.5, 0.0), glm::vec2(0.5, 0.5));
    drawPatch(glm::vec2(x2, y0), glm::vec2(x3, y1), glm::vec2(0.5, 0.0), glm::vec2(1.0, 0.5));

    drawPatch(glm::vec2(x0, y1), glm::vec2(x1, y2), glm::vec2(0.0, 0.5), glm::vec2(0.5, 0.5));
    drawPatch(glm::vec2(x1, y1), glm::vec2(x2, y2), glm::vec2(0.5, 0.5), glm::vec2(0.5, 0.5));
    drawPatch(glm::vec2(x2, y1), glm::vec2(x3, y2), glm::vec2(0.5, 0.5), glm::vec2(1.0, 0.5));

    drawPatch(glm::vec2(x0, y2), glm::vec2(x1, y3), glm::vec2(0.0, 0.5), glm::vec2(0.5, 1.0));
    drawPatch(glm::vec2(x1, y2), glm::vec2(x2, y3), glm::vec2(0.5, 0.5), glm::vec2(0.5, 1.0));
    drawPatch(glm::vec2(x2, y2), glm::vec2(x3, y3), glm::vec2(0.5, 0.5), glm::vec2(1.0, 1.0));
}

void UIPainter::updateSceneBox(int width, int height)
{
    static constexpr auto PreferredSceneSize = glm::vec2(1280, 720);
    static constexpr auto PreferredAspectRatio = PreferredSceneSize.x / PreferredSceneSize.y;

    const auto aspectRatio = static_cast<float>(width) / height;
    const auto sceneSize = [aspectRatio]() -> glm::vec2 {
        if (aspectRatio > PreferredAspectRatio) {
            const auto height = PreferredSceneSize.y;
            return { height * aspectRatio, height };
        } else {
            const auto width = PreferredSceneSize.x;
            return { width, width / aspectRatio };
        }
    }();

    const auto left = -0.5f * sceneSize.x;
    const auto right = 0.5f * sceneSize.x;
    const auto top = 0.5f * sceneSize.y;
    const auto bottom = -0.5f * sceneSize.y;
    m_sceneBox = GX::BoxF { { left, bottom }, { right, top } };
}

void UIPainter::resetTransform()
{
    m_transform = glm::mat4(1);
}

void UIPainter::scale(const glm::vec2 &s)
{
    m_transform = glm::scale(m_transform, glm::vec3(s, 1));
}

void UIPainter::scale(float sx, float sy)
{
    scale(glm::vec2(sx, sy));
}

void UIPainter::scale(float s)
{
    scale(s, s);
}

void UIPainter::translate(const glm::vec2 &p)
{
    m_transform = glm::translate(m_transform, glm::vec3(p, 0));
}

void UIPainter::translate(float dx, float dy)
{
    translate(glm::vec2(dx, dy));
}

void UIPainter::rotate(float angle)
{
    m_transform = glm::rotate(m_transform, angle, glm::vec3(0, 0, 1));
}

void UIPainter::saveTransform()
{
    m_transformStack.push_back(m_transform);
}

void UIPainter::restoreTransform()
{
    if (m_transformStack.empty()) {
        spdlog::warn("Transform stack underflow lol");
        return;
    }
    m_transform = m_transformStack.back();
    m_transformStack.pop_back();
}

std::size_t UIPainter::FontHasher::operator()(const Font &font) const
{
    std::size_t hash = 17;
    hash = hash * 31 + static_cast<std::size_t>(font.pixelHeight);
    hash = hash * 31 + std::hash<std::string>()(font.name);
    return hash;
}
