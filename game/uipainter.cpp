#include "uipainter.h"

#include <fontcache.h>
#include <loadprogram.h>
#include <shadermanager.h>
#include <spritebatcher.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <numeric>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

namespace {
constexpr auto TextureAtlasPageSize = 512;

std::string fontPath(std::string_view basename)
{
    return std::string("assets/fonts/") + std::string(basename);
}

std::string pixmapPath(std::string_view basename)
{
    return std::string("assets/images/") + std::string(basename);
}
} // namespace

UIPainter::UIPainter(GX::ShaderManager *shaderManager)
    : m_spriteBatcher(new GX::SpriteBatcher(shaderManager))
    , m_grayscaleTextureAtlas(new GX::TextureAtlas(TextureAtlasPageSize, TextureAtlasPageSize, GX::PixelType::Grayscale))
    , m_rgbaTextureAtlas(new GX::TextureAtlas(TextureAtlasPageSize, TextureAtlasPageSize, GX::PixelType::RGBA))
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
        auto fontCache = std::make_unique<GX::FontCache>(m_grayscaleTextureAtlas.get());
        const auto path = fontPath(font.name);
        if (!fontCache->load(path, font.pixelHeight)) {
            spdlog::error("Failed to load font {}", path);
        }
        it = m_fonts.emplace(font, std::move(fontCache)).first;
    }
    m_font = it->second.get();
}

GX::PackedPixmap UIPainter::getPixmap(const std::string &name)
{
    auto it = m_pixmaps.find(name);
    if (it == m_pixmaps.end()) {
        auto pm = GX::loadPixmap(pixmapPath(name));
        if (!pm) {
            spdlog::warn("Failed to load pixmap {}", name);
            return {};
        }
        auto packed = [this, &pm]() -> std::optional<GX::PackedPixmap> {
            switch (pm.pixelType) {
            case GX::PixelType::Grayscale:
                return m_grayscaleTextureAtlas->addPixmap(pm);
            case GX::PixelType::RGBA:
                return m_rgbaTextureAtlas->addPixmap(pm);
            default:
                return {};
            }
        }();
        if (!packed)
            return {};
        it = m_pixmaps.emplace(name, std::move(*packed)).first;
    }
    return it->second;
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

        addQuad(pixmap.texture,
                { { p0.x, p0.y }, { t0.x, t0.y } },
                { { p1.x, p0.y }, { t1.x, t0.y } },
                { { p1.x, p1.y }, { t1.x, t1.y } },
                { { p0.x, p1.y }, { t0.x, t1.y } },
                color, depth);

        glyphPosition += glm::vec2(glyph->advanceWidth, 0);
    }
}

template void UIPainter::drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const std::u32string &text);
template void UIPainter::drawText(const glm::vec2 &pos, const glm::vec4 &color, int depth, const std::string &text);

template<typename StringT>
float UIPainter::horizontalAdvance(const StringT &text) const
{
    return std::accumulate(text.begin(), text.end(), 0.0f, [this](float advance, auto ch) {
        const auto *g = m_font->getGlyph(ch);
        return advance + g->advanceWidth;
    });
}

template float UIPainter::horizontalAdvance(const std::u32string &text) const;
template float UIPainter::horizontalAdvance(const std::string &text) const;

glm::vec2 UIPainter::textBoxSize(float maxWidth, const std::u32string &text) const
{
    if (!m_font) {
        spdlog::warn("No font set lol");
        return {};
    }

    const auto rows = breakTextLines(text, maxWidth);
    const auto rowCount = rows.size();
    const auto textHeight = rowCount > 0 ? rowCount * (m_font->ascent() - m_font->descent()) + (rowCount - 1) * m_font->lineGap() : 0.0f;
    float textWidth = 0.0f;
    for (const auto &row : rows) {
        assert(std::abs(horizontalAdvance(row.text) - row.width) < 1e-3);
        textWidth = std::max(textWidth, row.width);
    }

    return glm::vec2(textWidth, textHeight);
}

glm::vec2 UIPainter::drawTextBox(const GX::BoxF &box, const glm::vec4 &color, int depth, const std::u32string &text)
{
    if (!m_font) {
        spdlog::warn("No font set lol");
        return {};
    }

    const auto rows = breakTextLines(text, box.width());

    const auto rowCount = rows.size();
    const auto textHeight = rowCount * (m_font->ascent() - m_font->descent()) + (rowCount - 1) * m_font->lineGap();
    auto y = [this, &box, textHeight] {
        switch (m_verticalAlign) {
        case VerticalAlign::Top:
            return box.min.y + m_font->ascent();
        case VerticalAlign::Bottom:
            return box.max.y - textHeight + m_font->ascent();
        case VerticalAlign::Middle:
        default:
            return 0.5f * (box.min.y + box.max.y) - 0.5f * textHeight + m_font->ascent();
        }
    }();
    const auto lineHeight = m_font->ascent() - m_font->descent() + m_font->lineGap();
    float textWidth = 0.0f;
    for (const auto &row : rows) {
        assert(std::abs(horizontalAdvance(row.text) - row.width) < 1e-3);
        textWidth = std::max(textWidth, row.width);
        const float x = [this, &box, rowWidth = row.width] {
            switch (m_horizontalAlign) {
            case HorizontalAlign::Left:
                return box.min.x;
            case HorizontalAlign::Right:
                return box.max.x - rowWidth;
            case HorizontalAlign::Center:
            default:
                return 0.5f * (box.min.x + box.max.x) - 0.5f * rowWidth;
            }
        }();
        drawText(glm::vec2(x, y), color, depth, row.text);
        y += lineHeight;
    }

    return glm::vec2(textWidth, textHeight);
}

void UIPainter::drawCircle(const glm::vec2 &center, float radius, const glm::vec4 &fillColor, const glm::vec4 &outlineColor, float outlineSize, int depth)
{
    const auto p0 = center - glm::vec2(radius, radius);
    const auto p1 = center + glm::vec2(radius, radius);

    const auto innerRadius = 0.5f - outlineSize / (2.0f * radius);
    const auto size = glm::vec4(innerRadius, 0, 0, 0);

    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Circle);
    addQuad({ { p0.x, p0.y }, { 0.0f, 0.0f } },
            { { p1.x, p0.y }, { 1.0f, 0.0f } },
            { { p1.x, p1.y }, { 1.0f, 1.0f } },
            { { p0.x, p1.y }, { 0.0f, 1.0f } },
            fillColor, outlineColor, size, depth);
}

void UIPainter::addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, const glm::vec4 &size, int depth)
{
    const auto quad = GX::SpriteBatcher::QuadVerts {
        { { glm::vec2(m_transform * glm::vec4(v0.position, 0, 1)), v0.textureCoords, fgColor, bgColor, size },
          { glm::vec2(m_transform * glm::vec4(v1.position, 0, 1)), v1.textureCoords, fgColor, bgColor, size },
          { glm::vec2(m_transform * glm::vec4(v2.position, 0, 1)), v2.textureCoords, fgColor, bgColor, size },
          { glm::vec2(m_transform * glm::vec4(v3.position, 0, 1)), v3.textureCoords, fgColor, bgColor, size } }
    };
    m_spriteBatcher->addSprite(texture, quad, depth);
}

void UIPainter::addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, int depth)
{
    addQuad(texture, v0, v1, v2, v3, fgColor, bgColor, glm::vec4(0), depth);
}

void UIPainter::addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &color, int depth)
{
    addQuad(texture, v0, v1, v2, v3, color, glm::vec4(0), glm::vec4(0), depth);
}

void UIPainter::addQuad(const GX::AbstractTexture *texture, const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, int depth)
{
    addQuad(texture, v0, v1, v2, v3, glm::vec4(0), glm::vec4(0), glm::vec4(0), depth);
}

void UIPainter::addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, const glm::vec4 &size, int depth)
{
    addQuad(nullptr, v0, v1, v2, v3, fgColor, bgColor, size, depth);
}

void UIPainter::addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &fgColor, const glm::vec4 &bgColor, int depth)
{
    addQuad(nullptr, v0, v1, v2, v3, fgColor, bgColor, glm::vec4(0), depth);
}

void UIPainter::addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, const glm::vec4 &color, int depth)
{
    addQuad(nullptr, v0, v1, v2, v3, color, glm::vec4(0), glm::vec4(0), depth);
}

void UIPainter::addQuad(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3, int depth)
{
    addQuad(nullptr, v0, v1, v2, v3, glm::vec4(0), glm::vec4(0), glm::vec4(0), depth);
}

void UIPainter::drawRoundedRect(const GX::BoxF &box, float radius, const glm::vec4 &fillColor, const glm::vec4 &outlineColor, float outlineSize, int depth)
{
    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Circle);

    const auto scale = m_transform[0][0];
    const auto innerRadius = 0.5f - (outlineSize / scale) / (2.0f * radius);
    const auto size = glm::vec4(innerRadius, 0, 0, 0);

    const auto drawPatch = [this, &fillColor, &outlineColor, size, depth](const glm::vec2 &p0, const glm::vec2 &p1, const glm::vec2 &t0, const glm::vec2 &t1) {
        addQuad({ { p0.x, p0.y }, { t0.x, t0.y } },
                { { p1.x, p0.y }, { t1.x, t0.y } },
                { { p1.x, p1.y }, { t1.x, t1.y } },
                { { p0.x, p1.y }, { t0.x, t1.y } },
                fillColor,
                outlineColor,
                size,
                depth);
    };

    const auto x0 = box.min.x;
    const auto x1 = box.min.x + radius;
    const auto x2 = box.max.x - radius;
    const auto x3 = box.max.x;

    const auto y0 = box.min.y;
    const auto y1 = box.min.y + radius;
    const auto y2 = box.max.y - radius;
    const auto y3 = box.max.y;

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

void UIPainter::drawThickLine(const glm::vec2 &from, const glm::vec2 &to, float thickness, const glm::vec4 &fromColor, const glm::vec4 &toColor, int depth)
{
    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::ThickLine);

    const auto dir = glm::normalize(to - from);
    const auto tangent = glm::vec2(-dir.y, dir.x);

    const auto p0 = from - 0.5f * thickness * tangent;
    const auto p1 = from + 0.5f * thickness * tangent;

    const auto p2 = to - 0.5f * thickness * tangent;
    const auto p3 = to + 0.5f * thickness * tangent;

    addQuad({ p0, { 0.0f, 0.0f } },
            { p2, { 1.0f, 0.0f } },
            { p3, { 1.0f, 1.0f } },
            { p1, { 0.0f, 1.0f } },
            fromColor,
            toColor,
            depth);
}

void UIPainter::drawGlowCircle(const glm::vec2 &center, float radius, const glm::vec4 &glowColor, const glm::vec4 &bgColor, float glowDistance, float glowStrength, int depth)
{
    const auto outerRadius = 3.0f * radius;

    const auto &p0 = center - glm::vec2(outerRadius, outerRadius);
    const auto &p1 = center + glm::vec2(outerRadius, outerRadius);

    const auto size = glm::vec4(2.0f * outerRadius, radius, glowDistance, glowStrength);

    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::GlowCircle);
    addQuad({ { p0.x, p0.y }, { 0.0f, 0.0f } },
            { { p1.x, p0.y }, { 1.0f, 0.0f } },
            { { p1.x, p1.y }, { 1.0f, 1.0f } },
            { { p0.x, p1.y }, { 0.0f, 1.0f } },
            glowColor, bgColor, size, depth);
}

void UIPainter::drawPixmap(const glm::vec2 &pos, const GX::PackedPixmap &pixmap, int depth)
{
    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Decal);

    const auto &p0 = pos;
    const auto p1 = p0 + glm::vec2(pixmap.width, pixmap.height);

    const auto &textureCoords = pixmap.textureCoords;
    const auto &t0 = textureCoords.min;
    const auto &t1 = textureCoords.max;

    addQuad(pixmap.texture,
            { { p0.x, p0.y }, { t0.x, t0.y } },
            { { p1.x, p0.y }, { t1.x, t0.y } },
            { { p1.x, p1.y }, { t1.x, t1.y } },
            { { p0.x, p1.y }, { t0.x, t1.y } },
            depth);
}

void UIPainter::drawCircleGauge(const glm::vec2 &center, float radius, const glm::vec4 &startColor, const glm::vec4 &endColor, float startAngle, float endAngle, float currentAngle, int depth)
{
    const auto p0 = center - glm::vec2(radius, radius);
    const auto p1 = center + glm::vec2(radius, radius);

    const auto size = glm::vec4(2.0f * radius, startAngle, endAngle, currentAngle);

    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::CircleGauge);
    addQuad({ { p0.x, p0.y }, { 0.0f, 0.0f } },
            { { p1.x, p0.y }, { 1.0f, 0.0f } },
            { { p1.x, p1.y }, { 1.0f, 1.0f } },
            { { p0.x, p1.y }, { 0.0f, 1.0f } },
            startColor, endColor, size, depth);
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

std::vector<UIPainter::TextRow> UIPainter::breakTextLines(const std::u32string &text, float maxWidth) const
{
    assert(m_font);

    std::vector<TextRow> rows;

    using Position = std::pair<std::u32string::const_iterator, float>;

    Position rowStart = { text.begin(), 0.0f };
    std::optional<Position> lastBreak;

    const auto spaceWidth = m_font->getGlyph(' ')->advanceWidth;

    const auto makeRow = [](auto start, float xStart, auto end, float xEnd) {
        return TextRow { std::u32string_view(&*start, std::distance(start, end)), xEnd - xStart };
    };

    float lineWidth = 0.0f;
    for (auto it = text.begin(); it != text.end(); ++it) {
        const auto ch = *it;
        if (ch == ' ') {
            if (lineWidth - rowStart.second > maxWidth) {
                if (lastBreak) {
                    rows.push_back(makeRow(rowStart.first, rowStart.second, lastBreak->first, lastBreak->second));
                    rowStart = { lastBreak->first + 1, lastBreak->second + spaceWidth };
                    lastBreak = { it, lineWidth };
                } else {
                    rows.push_back(makeRow(rowStart.first, rowStart.second, it, lineWidth));
                    rowStart = { it + 1, lineWidth + spaceWidth };
                }
            } else {
                lastBreak = { it, lineWidth };
            }
        }
        lineWidth += m_font->getGlyph(ch)->advanceWidth;
    }
    if (rowStart.first != text.end()) {
        if (lineWidth - rowStart.second > maxWidth && lastBreak) {
            rows.push_back(makeRow(rowStart.first, rowStart.second, lastBreak->first, lastBreak->second));
            rows.push_back(makeRow(lastBreak->first + 1, lastBreak->second + spaceWidth, text.end(), lineWidth));
        } else {
            rows.push_back(makeRow(rowStart.first, rowStart.second, text.end(), lineWidth));
        }
    }

    return rows;
}

void UIPainter::setVerticalAlign(VerticalAlign align)
{
    m_verticalAlign = align;
}

void UIPainter::setHorizontalAlign(HorizontalAlign align)
{
    m_horizontalAlign = align;
}

std::size_t UIPainter::FontHasher::operator()(const Font &font) const
{
    std::size_t hash = 17;
    hash = hash * 31 + static_cast<std::size_t>(font.pixelHeight);
    hash = hash * 31 + std::hash<std::string>()(font.name);
    return hash;
}
