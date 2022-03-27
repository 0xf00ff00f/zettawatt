#include "world.h"

#include "theme.h"
#include "tween.h"
#include "uipainter.h"

#include <fontcache.h>

#include <fmt/format.h>
#include <fmt/xchar.h>
#include <spdlog/spdlog.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <iostream>

using namespace std::string_literals;

namespace {
std::string iconPath(std::string_view basename)
{
    return std::string("assets/images/") + std::string(basename);
}

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

constexpr const char *FontName = "Arimo-Regular.ttf";

constexpr const auto BackgroundColor = glm::vec4(0.15, 0.15, 0.15, 1);

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

static const auto UnitLabelFont = UIPainter::Font { FontName, 25 };

class GraphItem
{
public:
    GraphItem(Unit *unit, const Theme *theme, World *world);
    ~GraphItem();

    bool mousePressEvent(const glm::vec2 &pos);
    void mouseReleaseEvent(const glm::vec2 &pos);
    void mouseMoveEvent(const glm::vec2 &pos);

    void initialize(UIPainter *painter);
    glm::vec2 position() const;
    float radius() const;
    void update(double elapsed);
    void paint(UIPainter *painter) const;
    bool contains(const glm::vec2 &pos) const;
    glm::vec4 color() const;
    bool isVisible() const;
    GX::BoxF boundingBox() const;

private:
    bool handleMousePress();
    void handleMouseRelease();
    bool isSelected() const { return m_world->currentUnit() == m_unit; }

    const Theme *m_theme;
    World *m_world;
    bool m_hovered = false;
    enum class State {
        Hidden,
        Inactive,
        Active,
        Selected,
    };
    State m_state = State::Hidden;
    State m_prevState = State::Hidden;
    Unit *m_unit;
    Wobble m_wobble;
    float m_stateTime = 0.0f;
    float m_stateTransitionTime = 0.0f;
    float m_acquireTime = 0.0f;
    GX::BoxF m_labelBox;
    GX::BoxF m_boundingBox;

    static constexpr auto Radius = 25.0f;
    static constexpr auto LabelTextWidth = 120.0f;
    static constexpr auto LabelMargin = 10.0f;
    static constexpr auto AcquireAnimationTime = 1.0f;
};

GraphItem::GraphItem(Unit *unit, const Theme *theme, World *world)
    : m_world(world)
    , m_theme(theme)
    , m_unit(unit)
    , m_wobble(6.0f)
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

void GraphItem::update(double elapsed)
{
    m_stateTime += elapsed;
    m_wobble.update(elapsed);
    if (m_acquireTime > 0.0f)
        m_acquireTime = std::max(static_cast<float>(m_acquireTime - elapsed), 0.0f);
    static constexpr auto StateTransitionTime = 2.0f;
    static constexpr auto SelectionTime = 0.25f;
    const auto setState = [this](State state, float transitionTime) {
        m_prevState = m_state;
        m_state = state;
        m_stateTime = 0.0f;
        m_stateTransitionTime = transitionTime;
    };
    switch (m_state) {
    case State::Hidden: {
        const auto shouldDisplay = [this] {
            if (m_unit->count > 0)
                return true;
            const auto &dependencies = m_unit->dependencies;
            if (dependencies.empty())
                return true;
            return std::all_of(dependencies.begin(), dependencies.end(), [](const Unit *unit) {
                return unit->count > 0;
            });
        }();
        if (shouldDisplay)
            setState(State::Inactive, StateTransitionTime);
        if (isSelected())
            setState(State::Selected, SelectionTime);
        break;
    }
    case State::Inactive: {
        if (m_unit->count > 0)
            setState(State::Active, StateTransitionTime);
        if (isSelected())
            setState(State::Selected, SelectionTime);
        break;
    }
    case State::Active:
        if (isSelected())
            setState(State::Selected, SelectionTime);
        break;
    case State::Selected:
        if (!isSelected())
            setState(m_unit->count > 0 ? State::Active : State::Inactive, SelectionTime);
        break;
    }
}

glm::vec2 GraphItem::position() const
{
    auto p = m_unit->position;
    const auto wobbleWeight = [this] {
        if (m_acquireTime > 0.0f && m_unit->count == 1)
            return m_acquireTime / AcquireAnimationTime;
        return m_unit->count > 0 ? 0.0f : 1.0f;
    }();
    p += wobbleWeight * m_wobble.offset();
    return p;
}

float GraphItem::radius() const
{
    if (m_acquireTime > 0.0f) {
        float t = m_acquireTime / AcquireAnimationTime;
        return tween<Tweeners::InQuadratic<float>>(Radius, static_cast<float>(1.5f * Radius), t);
    }
    return Radius;
}

glm::vec4 GraphItem::color() const
{
    const auto stateColor = [this](State state) -> glm::vec4 {
        switch (state) {
        case State::Hidden:
            return glm::vec4(m_theme->backgroundColor.xyz(), 0.0);
        case State::Inactive:
            return m_theme->inactiveUnit.color;
        case State::Active:
            return m_theme->activeUnit.color;
        case State::Selected:
            return m_theme->selectedUnit.color;
        default:
            assert(false);
            return {};
        }
    };
    if (m_stateTime < m_stateTransitionTime) {
        const auto t = m_stateTime / m_stateTransitionTime;
        return glm::mix(stateColor(m_prevState), stateColor(m_state), t);
    }
    return stateColor(m_state);
}

void GraphItem::initialize(UIPainter *painter)
{
    // circle
    const GX::BoxF circleBox { glm::vec2(-Radius), glm::vec2(Radius) };

    // label
    painter->setFont(UnitLabelFont);
    const auto textSize = painter->textBoxSize(LabelTextWidth, m_unit->name);
    const auto p = glm::vec2(0, Radius + LabelMargin);
    m_labelBox = GX::BoxF {
        p - glm::vec2(0.5f * textSize.x + LabelMargin, LabelMargin),
        p + glm::vec2(0.5f * textSize.x + LabelMargin, textSize.y + LabelMargin)
    };

    m_boundingBox = circleBox | m_labelBox;
}

GX::BoxF GraphItem::boundingBox() const
{
    return m_boundingBox + position();
}

namespace {

Theme::TextBox mix(const Theme::TextBox &lhs, const Theme::TextBox &rhs, float a)
{
    return {
        glm::mix(lhs.backgroundColor, rhs.backgroundColor, a),
        glm::mix(lhs.outlineColor, rhs.outlineColor, a),
        glm::mix(lhs.outlineThickness, rhs.outlineThickness, a),
        glm::mix(lhs.textColor, rhs.textColor, a),
    };
}

Theme::Unit mix(const Theme::Unit &lhs, const Theme::Unit &rhs, float a)
{
    return {
        glm::mix(lhs.color, rhs.color, a),
        mix(lhs.label, rhs.label, a),
        mix(lhs.counter, rhs.counter, a)
    };
}

}

void GraphItem::paint(UIPainter *painter) const
{
    const auto isSelected = this->isSelected();

    auto p = position();

    // painter->drawRoundedRect(m_boundingBox + p, 8.0f, glm::vec4(0), glm::vec4(0, 1, 0, 1), 3.0f, -100);

    const auto theme = [this]() -> Theme::Unit {
        const auto stateTheme = [this](State state) -> Theme::Unit {
            switch (state) {
            case State::Hidden: {
                const auto color = glm::vec4(m_theme->backgroundColor.xyz(), 0.0);
                return Theme::Unit {
                    color,
                    { color, color, 0.0f, color },
                    { color, color, 0.0f, color }
                };
            }
            case State::Inactive:
                return m_theme->inactiveUnit;
            case State::Active:
                return m_theme->activeUnit;
            case State::Selected:
                return m_theme->selectedUnit;
            default:
                assert(false);
                return {};
            }
        };
        if (m_stateTime < m_stateTransitionTime) {
            const auto t = m_stateTime / m_stateTransitionTime;
            return mix(stateTheme(m_prevState), stateTheme(m_state), t);
        }
        return stateTheme(m_state);
    }();

    const auto radius = this->radius();
    painter->drawCircle(p, radius, glm::vec4(0), theme.color, 6.0f, -1);

    if (m_world->canAcquire(m_unit)) {
        const auto glowDistance = 0.04 + 0.02 * std::sin(m_stateTime * 5.0);
        const auto glowStrength = 0.6;
        painter->drawGlowCircle(p, radius, m_theme->glowColor, BackgroundColor, glowDistance, glowStrength, 5);
    } else {
        const auto acquirable = [this] {
            if (m_unit->type == Unit::Type::Generator)
                return true;
            return m_unit->count == 0;
        }();
        if (acquirable) {
            constexpr auto RadiusDelta = 8;
            const auto addCircleGauge = [&p, painter](float radius, const glm::vec4 &color, float value) {
                constexpr auto StartAngle = 0;
                constexpr auto EndAngle = 1.25f * M_PI;
                float angle = StartAngle + value * (EndAngle - StartAngle);
                painter->drawCircleGauge(p, radius, 0.25f * color, color, StartAngle, EndAngle, angle, 2);
            };
            float r = radius + RadiusDelta;
            const auto &colors = m_theme->gaugeColors;
            const auto cost = m_unit->cost();
            const auto alpha = theme.label.backgroundColor.w;
            if (cost.energy > 0) {
                addCircleGauge(r, glm::vec4(colors.energy.xyz(), alpha), std::min(static_cast<float>(m_world->state().energy / cost.energy), 1.0f));
                r += RadiusDelta;
            }
            if (cost.material > 0) {
                addCircleGauge(r, glm::vec4(colors.material.xyz(), alpha), std::min(static_cast<float>(m_world->state().material / cost.material), 1.0f));
                r += RadiusDelta;
            }
            if (cost.extropy > 0) {
                addCircleGauge(r, glm::vec4(colors.extropy.xyz(), alpha), std::min(static_cast<float>(m_world->state().extropy / cost.extropy), 1.0f));
            }
        }
    }

    p += glm::vec2(0, Radius + LabelMargin);

    constexpr auto TextHeight = 80.0f;
    const auto textBox = GX::BoxF { p - glm::vec2(0.5f * LabelTextWidth, 0), p + glm::vec2(0.5f * LabelTextWidth, TextHeight) };
    painter->setVerticalAlign(UIPainter::VerticalAlign::Top);
    painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
    painter->setFont(UnitLabelFont);
    auto textSize = painter->drawTextBox(textBox, theme.label.textColor, 2, m_unit->name);

    auto outerBox = GX::BoxF { p - glm::vec2(0.5f * textSize.x + LabelMargin, LabelMargin), p + glm::vec2(0.5f * textSize.x + LabelMargin, textSize.y + LabelMargin) };
    constexpr auto BoxRadius = 8.0f;
    painter->drawRoundedRect(outerBox, BoxRadius, theme.label.backgroundColor, theme.label.outlineColor, theme.label.outlineThickness, 1);

    const auto count = m_unit->count;
    if (count > 1) {
        const auto center = glm::vec2(outerBox.max.x, outerBox.min.y);
        constexpr const auto CounterRadius = 22.0f;

        painter->drawCircle(center, CounterRadius, theme.counter.backgroundColor, theme.counter.outlineColor, theme.counter.outlineThickness, 3);

        const auto counterBox = GX::BoxF { center - 0.5f * glm::vec2(CounterRadius), center + 0.5f * glm::vec2(CounterRadius) };
        painter->setVerticalAlign(UIPainter::VerticalAlign::Middle);
        painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
        painter->setFont(UIPainter::Font { FontName, 20 });
        painter->drawTextBox(counterBox, theme.counter.textColor, 4, fmt::format(U"x{}", count));
    }
}

bool GraphItem::contains(const glm::vec2 &pos) const
{
    if (m_state == GraphItem::State::Hidden)
        return false;
    const auto p = position();
    if (glm::distance(pos, p) < radius())
        return true;
    return (m_labelBox + p).contains(pos);
}

bool GraphItem::handleMousePress()
{
    if (m_world->unitClicked(m_unit))
        m_acquireTime = AcquireAnimationTime;
    return true;
}

void GraphItem::handleMouseRelease()
{
}

bool GraphItem::isVisible() const
{
    return m_state != State::Hidden;
}

World::World() = default;
World::~World() = default;

void World::initialize(const Theme *theme, UIPainter *painter, TechGraph *techGraph)
{
    m_theme = theme;
    m_painter = painter;
    m_techGraph = techGraph;

    m_graphItems.clear();
    for (auto &unit : m_techGraph->units) {
        auto item = std::make_unique<GraphItem>(unit.get(), m_theme, this);
        item->initialize(painter);
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

    m_extropyIcon = m_painter->getPixmap("extropy.png");
    m_extropyIconSmall = m_painter->getPixmap("extropy-sm.png");
    m_energyIcon = m_painter->getPixmap("energy.png");
    m_energyIconSmall = m_painter->getPixmap("energy-sm.png");
    m_materialIcon = m_painter->getPixmap("material.png");
    m_materialIconSmall = m_painter->getPixmap("material-sm.png");
    m_carbonIcon = m_painter->getPixmap("carbon.png");
    m_carbonIconSmall = m_painter->getPixmap("carbon-sm.png");
}

void World::reset()
{
    m_state.extropy = 0;
    m_state.energy = 0;
    m_state.material = 0;
    m_state.carbon = 0;
    m_viewOffset = glm::vec2(0);
    m_currentUnit = nullptr;
}

void World::update(double elapsed)
{
    m_state += m_stateDelta * elapsed;

    for (auto &item : m_graphItems)
        item->update(elapsed);

    if (m_panningView)
        m_elapsedSinceClick += elapsed;
}

void World::updateStateDelta()
{
    StateVector delta;
    for (const auto &unit : m_techGraph->units) {
        if (!unit->count || unit->type != Unit::Type::Generator)
            continue;
        float boost = 1.0f;
        for (const auto &other : m_techGraph->units) {
            if (other->type == Unit::Type::Booster && other->count > 0 && other->boost.target == unit.get())
                boost *= other->boost.factor;
        }
        delta += unit->count * boost * unit->yield;
    }
    m_stateDelta = delta;
}

void World::paint() const
{
    paintGraph();
    paintState();
    paintCurrentUnitDescription();
}

void World::paintGraph() const
{
    m_painter->saveTransform();
    m_painter->translate(m_viewOffset);

    for (auto [from, to] : m_edges) {
        if (!from->isVisible() && !to->isVisible())
            continue;

        constexpr auto NodeBorder = 4.0f;

        auto fromPosition = from->position();
        auto toPosition = to->position();
        const auto d = glm::normalize(fromPosition - toPosition);
        fromPosition -= (from->radius() - NodeBorder) * d;
        toPosition += (to->radius() - NodeBorder) * d;
        m_painter->drawThickLine(fromPosition, toPosition, 5, from->color(), to->color(), -1);
    }

    for (auto &item : m_graphItems) {
        if (!item->isVisible())
            continue;
        const auto boundingBox = item->boundingBox() + m_viewOffset;
        if (!m_painter->sceneBox().contains(boundingBox))
            continue;
        item->paint(m_painter);
    }

    m_painter->restoreTransform();
}

void World::paintState() const
{
    constexpr auto TextDepth = 20;

    constexpr auto CounterWidth = 320.0f;
    constexpr auto CounterHeight = 160.0f;

    auto paintCounter = [this](float centerX, float centerY, const std::u32string &label, const std::string &unit, const GX::PackedPixmap &icon, double value, double delta) {
        if (value == 0.0)
            return;

        const auto &theme = m_theme->counter;

        const auto box = GX::BoxF { glm::vec2(centerX - 0.5 * CounterWidth, centerY - 0.5 * CounterHeight), glm::vec2(centerX + 0.5 * CounterWidth, centerY + 0.5 * CounterHeight) };
        m_painter->drawRoundedRect(box, 20, theme.backgroundColor, theme.outlineColor, theme.outlineThickness, TextDepth - 1);

        static const auto LabelFont = UIPainter::Font { FontName, 40 };
        static const auto CounterFontBig = UIPainter::Font { FontName, 70 };
        static const auto CounterFontSmall = UIPainter::Font { FontName, 40 };
        static const auto DeltaFont = UIPainter::Font { FontName, 40 };

        float y = centerY - 40;

        // label
        {
            m_painter->setFont(LabelFont);
            const auto advance = m_painter->horizontalAdvance(label) + icon.width;
            auto x = centerX - 0.5f * advance;
            const auto textHeight = m_painter->font()->ascent() + m_painter->font()->descent();
            // is this even right lol
            m_painter->drawPixmap(glm::vec2(x, y - 0.5f * (textHeight + icon.height)), icon, TextDepth);
            x += icon.width;
            m_painter->drawText(glm::vec2(x, y), theme.labelColor, TextDepth, label);
        }
        y += 60;

        // counter
        {
            const auto [big, small, power] = formattedValue(value);
            if (power != ' ') {
                const auto bigText = fmt::format("{}", big);
                const auto smallText = fmt::format(".{:03d}", small);
                const auto unitText = power != ' ' ? fmt::format("{}{}", static_cast<char>(power), unit) : unit;

                m_painter->setFont(CounterFontBig);
                const auto bigAdvance = m_painter->horizontalAdvance(bigText);
                const auto unitAdvance = m_painter->horizontalAdvance(unitText);

                m_painter->setFont(CounterFontSmall);
                const auto smallAdvance = m_painter->horizontalAdvance(smallText);

                const auto totalAdvance = bigAdvance + smallAdvance + unitAdvance;

                const auto left = centerX - 0.5f * totalAdvance;

                m_painter->setFont(CounterFontBig);
                m_painter->drawText(glm::vec2(left, y), theme.valueColor, TextDepth, bigText);
                m_painter->drawText(glm::vec2(left + bigAdvance + smallAdvance, y), theme.valueColor, TextDepth, unitText);

                m_painter->setFont(CounterFontSmall);
                m_painter->drawText(glm::vec2(left + bigAdvance, y), theme.valueColor, TextDepth, smallText);
            } else {
                const auto text = fmt::format("{}{}", big, unit);
                m_painter->setFont(CounterFontBig);
                paintCentered(m_painter, centerX, y, theme.valueColor, TextDepth, text);
            }
        }
        y += 40;

        // delta
        {
            const auto text = [&] {
                const auto [big, small, power] = formattedValue(delta);
                if (power == ' ') {
                    return fmt::format("{}{}/s", big, unit);
                } else {
                    return fmt::format("{}.{:03d}{}{}/s", big, small, static_cast<char>(power), unit);
                }
            }();
            m_painter->setFont(DeltaFont);
            paintCentered(m_painter, centerX, y, theme.deltaColor, TextDepth, text);
        }
    };

    const GX::BoxF sceneBox = m_painter->sceneBox();
    const float y = sceneBox.min.y + 0.5 * CounterHeight;

    paintCounter(-1.5f * CounterWidth, y, U"ENERGY"s, "Wh"s, m_energyIcon, m_state.energy, m_stateDelta.energy);
    paintCounter(-.5f * CounterWidth, y, U"MATERIALS"s, "t"s, m_materialIcon, m_state.material, m_stateDelta.material);
    paintCounter(.5f * CounterWidth, y, U"CO\U00002082"s, "t"s, m_carbonIcon, m_state.carbon, m_stateDelta.carbon);
    paintCounter(1.5f * CounterWidth, y, U"EXTROPY"s, ""s, m_extropyIcon, m_state.extropy, m_stateDelta.extropy);
}

void World::paintCurrentUnitDescription() const
{
    if (!m_currentUnit)
        return;

    const auto &theme = m_theme->unitDetails;

    // cost
    const auto formatCost = [](double value, const std::u32string &unit) -> std::u32string {
        if (value == 0.0)
            return {};
        static const char32_t *factors = U" kMGTPEZY";
        size_t factor = 0;
        while (value >= 1000) {
            value /= 1000;
            ++factor;
        }
        if (factor > 0)
            return fmt::format(U"{:.1f}{}{}", value, factors[factor], unit);
        else
            return fmt::format(U"{:.1f}{}", value, unit);
    };
    const auto cost = m_currentUnit->cost();

    static const auto TitleFont = UIPainter::Font { FontName, 25 };
    static const auto DescriptionFont = UIPainter::Font { FontName, 20 };

    constexpr auto MaxWidth = 420.0f;
    constexpr auto TitleMaxWidth = MaxWidth - 120.0f;
    constexpr auto Margin = 10.0f;
    constexpr auto BoxRadius = 8.0f;

    float textHeight = 0.0f;

    // cost
    m_painter->setFont(DescriptionFont);
    const auto costLines = (cost.energy > 0.0) + (cost.material > 0.0) + (cost.extropy > 0.0);
    const auto costHeight = costLines * m_painter->font()->pixelHeight();

    // title
    m_painter->setFont(TitleFont);
    auto titleSize = m_painter->textBoxSize(TitleMaxWidth, m_currentUnit->name);
    titleSize.y = std::max(titleSize.y, static_cast<float>(costHeight));
    textHeight += titleSize.y;

    // description
    m_painter->setFont(DescriptionFont);
    const auto descriptionSize = m_painter->textBoxSize(MaxWidth, m_currentUnit->description);
    textHeight += descriptionSize.y;

    // boost
    textHeight += m_painter->font()->pixelHeight();

    constexpr auto TitleTextWidth = TitleMaxWidth + 1.0f;
    constexpr auto TextWidth = MaxWidth + 1.0f;

    const auto topLeft = m_painter->sceneBox().max - glm::vec2(TextWidth + Margin, textHeight + Margin);

    m_painter->setVerticalAlign(UIPainter::VerticalAlign::Top);
    m_painter->setHorizontalAlign(UIPainter::HorizontalAlign::Left);

    // paint text
    {
        glm::vec2 p = topLeft;
        m_painter->setFont(TitleFont);
        m_painter->drawTextBox(GX::BoxF { p, p + glm::vec2(TitleTextWidth, titleSize.y) }, theme.titleColor, 20, m_currentUnit->name);

        p += glm::vec2(0, titleSize.y);
        m_painter->setFont(DescriptionFont);
        m_painter->drawTextBox(GX::BoxF { p, p + glm::vec2(TextWidth, descriptionSize.y) }, theme.descriptionColor, 20, m_currentUnit->description);

        p += glm::vec2(0, descriptionSize.y + m_painter->font()->ascent());
        if (m_currentUnit->type == Unit::Type::Booster) {
            std::u32string boostDescription;
            if (m_currentUnit->type == Unit::Type::Booster) {
                const auto factor = m_currentUnit->boost.factor;
                if (factor > 1.0)
                    boostDescription = fmt::format(U"{} {}% more efficient", m_currentUnit->boost.target->name, static_cast<int>((factor - 1) * 100 + 0.5f));
                else
                    boostDescription = fmt::format(U"{} {}% less efficient", m_currentUnit->boost.target->name, static_cast<int>((1 - factor) * 100 + 0.5f));
            }

            m_painter->drawText(p, glm::vec4(1, 1, 0, 1), 20, boostDescription);
        } else {
            static const auto prefix = "Produces "s;
            m_painter->drawText(p, theme.yieldColor, 20, prefix);
            p += glm::vec2(m_painter->horizontalAdvance(prefix), 0);
            const auto drawYield = [this, &theme, &p](const std::u32string &text, const GX::PackedPixmap &icon) {
                if (text.empty())
                    return;
                const auto textHeight = m_painter->font()->ascent() + m_painter->font()->descent();
                m_painter->drawPixmap(glm::vec2(p.x, p.y - 0.5f * (textHeight + icon.height)), icon, 20);
                p.x += icon.width;
                m_painter->drawText(p, theme.yieldColor, 20, text);
                p.x += m_painter->horizontalAdvance(text);
            };
            const auto &yield = m_currentUnit->yield;
            if (yield.energy > 0.0)
                drawYield(formatCost(yield.energy, U"Wh/s"), m_energyIconSmall);
            if (yield.material > 0.0)
                drawYield(formatCost(yield.material, U"t/s"), m_materialIconSmall);
            if (yield.carbon > 0.0)
                drawYield(formatCost(yield.carbon, U"t/s"), m_carbonIconSmall);
            if (yield.extropy > 0.0)
                drawYield(formatCost(yield.extropy, U"/s"), m_extropyIconSmall);
        }
    }

    // paint cost
    {
        glm::vec2 p = topLeft + glm::vec2(TextWidth, m_painter->font()->ascent());

        const auto drawCost = [this, &theme, &p](const std::u32string &text, const GX::PackedPixmap &icon) {
            if (text.empty())
                return;
            const auto advance = m_painter->horizontalAdvance(text) + icon.width;
            const auto textHeight = m_painter->font()->ascent() + m_painter->font()->descent();
            auto x = p.x - advance;
            m_painter->drawPixmap(glm::vec2(x, p.y - 0.5f * (textHeight + icon.height)), icon, 20);
            x += icon.width;
            m_painter->drawText(glm::vec2(x, p.y), theme.costColor, 20, text);
            p.y += m_painter->font()->pixelHeight();
        };
        if (cost.energy > 0.0)
            drawCost(formatCost(cost.energy, U"Wh"s), m_energyIconSmall);
        if (cost.material > 0.0)
            drawCost(formatCost(cost.material, U"t"s), m_materialIconSmall);
        if (cost.extropy > 0.0)
            drawCost(formatCost(cost.extropy, U""s), m_extropyIconSmall);
    }

    const auto outerBox = GX::BoxF { topLeft - glm::vec2(Margin, Margin), topLeft + glm::vec2(TextWidth + Margin, textHeight + Margin) };
    m_painter->drawRoundedRect(outerBox, BoxRadius, theme.backgroundColor, theme.outlineColor, theme.outlineThickness, 19);
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
    m_elapsedSinceClick = 0.0;
}

void World::mouseReleaseEvent(const glm::vec2 &pos)
{
    if (m_panningView) {
        if (m_elapsedSinceClick < 0.5)
            m_state.energy += glm::linearRand(5, 8);
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
        auto [min, max] = [this] {
            auto min = glm::vec2(std::numeric_limits<float>::max());
            auto max = glm::vec2(std::numeric_limits<float>::lowest());
            for (const auto &unit : m_graphItems) {
                if (unit->isVisible()) {
                    const auto p = unit->position();
                    min = glm::min(min, p);
                    max = glm::max(max, p);
                }
            }
            return std::pair(min, max);
        }();
        const auto viewportSize = m_painter->sceneBox().size();
        m_viewOffset = glm::max(m_viewOffset, -max - 0.5f * viewportSize);
        m_viewOffset = glm::min(m_viewOffset, -min + 0.5f * viewportSize);
    } else {
        for (auto &item : m_graphItems)
            item->mouseMoveEvent(pos - m_viewOffset);
    }
    m_lastMousePosition = pos;
}

bool World::unitClicked(Unit *unit)
{
    bool acquired = false;
    if (unit == m_currentUnit) {
        if (canAcquire(unit)) {
            m_state -= unit->cost();
            ++unit->count;
            updateStateDelta();
            acquired = true;
        }
    }
    m_currentUnit = unit;
    return acquired;
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
    const auto cost = unit->cost();
    return cost.extropy <= m_state.extropy && cost.energy <= m_state.energy && cost.material <= m_state.material;
}
