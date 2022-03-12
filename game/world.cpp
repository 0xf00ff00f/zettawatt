#include "world.h"

#include "uipainter.h"

#include <fontcache.h>

#include <fmt/format.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <iostream>

using namespace std::string_literals;

namespace {
std::tuple<int, int, char32_t> formattedValue(double value)
{
    static const char32_t *units = U" kMGTPEZY";
    size_t unit = 0;
    while (value >= 1000) {
        value /= 1000;
        ++unit;
    }
    return { static_cast<int>(value), static_cast<int>(value * 1000) % 1000, units[unit] };
}

constexpr const char *FontName = "IBMPlexSans-Regular.ttf";

constexpr const auto GraphColor = glm::vec4(1, 0, 0, 1);

template<typename StringT>
void paintCentered(UIPainter *painter, float x, float y, const glm::vec4 &color, int depth, const StringT &s)
{
    const auto advance = painter->horizontalAdvance(s);
    painter->drawText(glm::vec2(x - 0.5f * advance, y), color, depth, s);
}

class Wobble
{
public:
    explicit Wobble(float radius);

    void update(float elapsed);
    glm::vec2 offset() const;

private:
    struct Wave {
        Wave(float radius)
            : dir(glm::circularRand(radius))
            , phase(glm::linearRand(0.0f, 2.0f * glm::pi<float>()))
            , speed(glm::linearRand(1.0f, 3.0f))
        {
        }
        glm::vec2 eval(float t) const
        {
            return dir * sinf(speed * t + phase);
        }
        glm::vec2 dir;
        float phase;
        float speed;
    };
    std::vector<Wave> m_waves;
    float m_t = 0.0f;
};

Wobble::Wobble(float radius)
{
    std::generate_n(std::back_inserter(m_waves), 3, [radius] {
        return Wave(glm::linearRand(0.5f * radius, radius));
    });
}

void Wobble::update(float elapsed)
{
    m_t += elapsed;
}

glm::vec2 Wobble::offset() const
{
    auto o = glm::vec2(0);
    for (const auto &wave : m_waves)
        o += wave.eval(m_t);
    return o;
}
} // namespace

GraphItem::GraphItem(World *world)
    : m_world(world)
{
}

GraphItem::~GraphItem() = default;

bool GraphItem::mousePressEvent(const glm::vec2 &pos)
{
    if (!contains(pos))
        return false;
    return handleMousePress();
}

void GraphItem::mouseReleaseEvent(const glm::vec2 &pos)
{
    if (contains(pos))
        handleMouseRelease();
}

void GraphItem::mouseMoveEvent(const glm::vec2 &pos)
{
    m_hovered = contains(pos);
}

bool GraphItem::handleMousePress()
{
    return false;
}

void GraphItem::handleMouseRelease()
{
}

class UnitItem : public GraphItem
{
public:
    UnitItem(Unit *unit, World *world);
    ~UnitItem();

    glm::vec2 position() const override;
    float radius() const override;
    void update(double elapsed) override;
    void paint(UIPainter *painter) const override;
    bool contains(const glm::vec2 &pos) const override;

    bool handleMousePress() override;

private:
    Unit *m_unit;
    Wobble m_wobble;
    static constexpr auto Radius = 25.0f;
};

UnitItem::UnitItem(Unit *unit, World *world)
    : GraphItem(world)
    , m_unit(unit)
    , m_wobble(6.0f)
{
}

UnitItem::~UnitItem() = default;

void UnitItem::update(double elapsed)
{
    m_wobble.update(elapsed);
}

glm::vec2 UnitItem::position() const
{
    return m_unit->position + m_wobble.offset();
}

float UnitItem::radius() const
{
    return m_hovered ? 1.2f * Radius : Radius;
}

void UnitItem::paint(UIPainter *painter) const
{
    constexpr auto FontSize = 25;
    static const auto LabelFont = UIPainter::Font { FontName, FontSize };
    painter->setFont(LabelFont);

    auto p = position();
    painter->drawCircle(p, radius(), glm::vec4(0), GraphColor, 6.0f, -1);

    if (m_world->canAcquire(m_unit))
        painter->drawGlowCircle(p, radius(), glm::vec4(0, 1, 1, 1), -2);

    constexpr auto Margin = 10.0f;
    p += glm::vec2(0, Radius + Margin);

    constexpr auto TextWidth = 80.0f;
    constexpr auto TextHeight = 80.0f;
    const auto textBox = GX::BoxF { p - glm::vec2(0.5f * TextWidth, 0), p + glm::vec2(0.5f * TextWidth, TextHeight) };
    painter->setVerticalAlign(UIPainter::VerticalAlign::Top);
    painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
    auto textSize = painter->drawTextBox(textBox, glm::vec4(1), 2, m_unit->name);

    auto outerBox = GX::BoxF { p - glm::vec2(0.5f * textSize.x + Margin, Margin), p + glm::vec2(0.5f * textSize.x + Margin, textSize.y + Margin) };
    constexpr auto BoxRadius = 8.0f;
    painter->drawRoundedRect(outerBox, BoxRadius, glm::vec4(0, 0, 0, 0.75), glm::vec4(1, 1, 1, 1), 3.0f, 1);
}

bool UnitItem::contains(const glm::vec2 &pos) const
{
    return glm::distance(pos, position()) < radius();
}

bool UnitItem::handleMousePress()
{
    m_world->unitClicked(m_unit);
    return true;
}

World::World() = default;
World::~World() = default;

void World::initialize(TechGraph *techGraph)
{
    m_techGraph = techGraph;

    m_graphItems.clear();
    for (auto &unit : m_techGraph->units) {
        auto item = std::make_unique<UnitItem>(unit.get(), this);
        m_unitItems[unit.get()] = item.get();
        m_graphItems.emplace_back(std::move(item));
    }

    for (auto &unit : m_techGraph->units) {
        const auto *fromUnit = m_unitItems[unit.get()];
        for (const auto *dependency : unit->dependencies) {
            const auto *toUnit = m_unitItems[dependency];
            assert(toUnit);
            m_edges.push_back(Edge { fromUnit, toUnit });
        }
    }

    reset();
}

void World::reset()
{
    m_state.extropy = 0;
    m_state.energy = 2100;
    m_state.material = 2000;
    m_state.carbon = 0;

    m_viewOffset = glm::vec2(0);
}

void World::update(double elapsed)
{
    m_state += m_stateDelta * elapsed;

    for (auto &item : m_graphItems)
        item->update(elapsed);
}

void World::updateStateDelta()
{
    StateVector delta;
    for (const auto &unit : m_techGraph->units) {
        if (!unit->count)
            continue;
        StateVector yield = unit->yield;
        delta += unit->count * yield;
    }
    m_stateDelta = delta;
}

void World::paint(UIPainter *painter) const
{
    paintGraph(painter);
    paintState(painter);
}

void World::paintGraph(UIPainter *painter) const
{
    painter->saveTransform();
    painter->translate(m_viewOffset);

    for (auto [from, to] : m_edges) {
        constexpr auto NodeBorder = 4.0f;

        auto fromPosition = from->position();
        auto toPosition = to->position();
        const auto d = glm::normalize(fromPosition - toPosition);
        fromPosition -= (from->radius() - NodeBorder) * d;
        toPosition += (to->radius() - NodeBorder) * d;
        painter->drawThickLine(fromPosition, toPosition, 5, GraphColor, -1);
    }

    for (auto &item : m_graphItems)
        item->paint(painter);

    painter->restoreTransform();
}

void World::paintState(UIPainter *painter) const
{
    constexpr auto TextDepth = 4;

    constexpr auto CounterWidth = 320.0f;
    constexpr auto CounterHeight = 160.0f;

    auto paintCounter = [painter](float centerX, float centerY, const std::u32string &label, const std::string &unit, double value, double delta) {
        const auto box = GX::BoxF { glm::vec2(centerX - 0.5 * CounterWidth, centerY - 0.5 * CounterHeight), glm::vec2(centerX + 0.5 * CounterWidth, centerY + 0.5 * CounterHeight) };
        painter->drawRoundedRect(box, 20, glm::vec4(0, 0, 0, 0.75), glm::vec4(1, 1, 1, 1), 4.0f, TextDepth - 1);

        static const auto LabelFont = UIPainter::Font { FontName, 40 };
        static const auto CounterFontBig = UIPainter::Font { FontName, 80 };
        static const auto CounterFontSmall = UIPainter::Font { FontName, 40 };
        static const auto DeltaFont = UIPainter::Font { FontName, 40 };

        float y = centerY - 40;

        // label
        {
            painter->setFont(LabelFont);
            paintCentered(painter, centerX, y, glm::vec4(1), TextDepth, label);
        }
        y += 60;

        // counter
        {
            const auto [big, small, power] = formattedValue(value);
            if (power != ' ') {
                const auto bigText = fmt::format("{}", big);
                const auto smallText = fmt::format(".{:03d}", small);
                const auto unitText = fmt::format("{}{}", static_cast<char>(power), unit);

                painter->setFont(CounterFontBig);
                const auto bigAdvance = painter->horizontalAdvance(bigText);
                const auto unitAdvance = painter->horizontalAdvance(unitText);

                painter->setFont(CounterFontSmall);
                const auto smallAdvance = painter->horizontalAdvance(smallText);

                const auto totalAdvance = bigAdvance + smallAdvance + unitAdvance;

                const auto left = centerX - 0.5f * totalAdvance;

                painter->setFont(CounterFontBig);
                painter->drawText(glm::vec2(left, y), glm::vec4(1), TextDepth, bigText);
                painter->drawText(glm::vec2(left + bigAdvance + smallAdvance, y), glm::vec4(1), TextDepth, unitText);

                painter->setFont(CounterFontSmall);
                painter->drawText(glm::vec2(left + bigAdvance, y), glm::vec4(1), TextDepth, smallText);
            } else {
                const auto text = fmt::format("{}{}", big, unit);
                painter->setFont(CounterFontBig);
                paintCentered(painter, centerX, y, glm::vec4(1), TextDepth, text);
            }
        }
        y += 40;

        // delta
        {
            const auto [big, small, power] = formattedValue(delta);
            const auto text = [&] {
                if (power == ' ') {
                    return fmt::format("{}{}/s", big, unit);
                } else {
                    return fmt::format("{}.{:03d}{}{}/s", big, small, static_cast<char>(power), unit);
                }
            }();
            painter->setFont(DeltaFont);
            paintCentered(painter, centerX, y, glm::vec4(1), TextDepth, text);
        }
    };

    const GX::BoxF sceneBox = painter->sceneBox();
    const float y = sceneBox.min.y + 0.5 * CounterHeight;

    paintCounter(-1.5f * CounterWidth, y, U"EXTROPY"s, ""s, m_state.extropy, m_stateDelta.extropy);
    paintCounter(-0.5f * CounterWidth, y, U"ENERGY"s, "Wh"s, m_state.energy, m_stateDelta.energy);
    paintCounter(0.5f * CounterWidth, y, U"MATERIALS"s, "t"s, m_state.material, m_stateDelta.material);
    paintCounter(1.5f * CounterWidth, y, U"CO\U00002082"s, "t"s, m_state.carbon, m_stateDelta.carbon);
}

void World::mousePressEvent(const glm::vec2 &pos)
{
    bool accepted = false;
    for (auto &item : m_graphItems) {
        if (item->mousePressEvent(pos - m_viewOffset))
            accepted = true;
    }
    m_panningView = !accepted;
    m_lastMousePosition = pos;
}

void World::mouseReleaseEvent(const glm::vec2 &pos)
{
    if (m_panningView) {
        m_panningView = false;
    } else {
        for (auto &item : m_graphItems)
            item->mouseReleaseEvent(pos - m_viewOffset);
    }
}

void World::mouseMoveEvent(const glm::vec2 &pos)
{
    if (m_panningView) {
        m_viewOffset += pos - m_lastMousePosition;
    } else {
        for (auto &item : m_graphItems)
            item->mouseMoveEvent(pos - m_viewOffset);
    }
    m_lastMousePosition = pos;
}

bool World::unitClicked(Unit *unit)
{
    if (!canAcquire(unit))
        return false;

    ++unit->count;
    m_state -= unit->cost;
    updateStateDelta();

    return true;
}

bool World::canAcquire(const Unit *unit) const
{
    if (unit->type == Unit::Type::Booster && unit->count > 0)
        return false;
    auto hasDependencies = [unit] {
        const auto &dependencies = unit->dependencies;
        if (dependencies.empty())
            return true;
        return std::all_of(dependencies.begin(), dependencies.end(), [](const Unit *unit) {
            return unit->count > 0;
        });
    }();
    if (!hasDependencies)
        return false;
    const auto &cost = unit->cost;
    return cost.extropy <= m_state.extropy && cost.energy <= m_state.energy && cost.material <= m_state.material;
}
