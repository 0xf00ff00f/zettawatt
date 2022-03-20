#include "world.h"

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
constexpr const char *BoldFontName = "Arimo-Bold.ttf";

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

static const auto UnitLabelFont = UIPainter::Font { FontName, 25 };

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
    glm::vec4 color() const override;
    bool handleMousePress() override;
    bool isVisible() const override;
    GX::BoxF boundingBox(UIPainter *painter) const override;

private:
    void initializeBoundingBox(UIPainter *painter) const;
    float labelAlpha() const;

    enum class State {
        Hidden,
        FadeIn,
        Inactive,
        Activating,
        Active,
    };
    State m_state = State::Hidden;
    Unit *m_unit;
    Wobble m_wobble;
    float m_stateTime = 0.0f;
    float m_acquireTime = 0.0f;
    mutable GX::BoxF m_boundingBox;

    static constexpr auto Radius = 25.0f;
    static constexpr auto LabelTextWidth = 120.0f;
    static constexpr auto LabelMargin = 10.0f;
    static constexpr auto ActivationTime = 2.0f;
    static constexpr auto FadeInTime = 2.0f;
    static constexpr auto AcquireAnimationTime = 1.0f;
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
    m_stateTime += elapsed;
    m_wobble.update(elapsed);
    if (m_acquireTime > 0.0f)
        m_acquireTime = std::max(static_cast<float>(m_acquireTime - elapsed), 0.0f);
    const auto setState = [this](State state) {
        m_state = state;
        m_stateTime = 0.0f;
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
            setState(State::FadeIn);
        break;
    }
    case State::FadeIn: {
        if (m_stateTime >= FadeInTime)
            setState(State::Inactive);
        break;
    }
    case State::Inactive: {
        if (m_unit->count > 0)
            setState(State::Activating);
        break;
    }
    case State::Activating: {
        if (m_stateTime >= ActivationTime)
            setState(State::Active);
        break;
    }
    case State::Active:
        break;
    }
}

glm::vec2 UnitItem::position() const
{
    auto p = m_unit->position;
    const auto wobbleWeight = [this] {
        switch (m_state) {
        case State::Inactive:
        case State::FadeIn:
        case State::Hidden:
            return 1.0f;
        case State::Activating:
            return 1.0f - m_stateTime / ActivationTime;
        default:
            return 0.0f;
        }
    }();
    p += wobbleWeight * m_wobble.offset();
    return p;
}

float UnitItem::radius() const
{
    if (m_acquireTime > 0.0f) {
        float t = m_acquireTime / AcquireAnimationTime;
        return tween<Tweeners::InQuadratic<float>>(Radius, static_cast<float>(1.5f * Radius), t);
    }
    return Radius;
}

glm::vec4 UnitItem::color() const
{
    constexpr const auto HiddenColor = glm::vec4(0);
    constexpr const auto ActiveColor = glm::vec4(1, 0, 0, 1);
    constexpr const auto InactiveColor = glm::vec4(0.25, 0.25, 0.25, 1);
    constexpr const auto AcquiredColor = glm::vec4(1);
    switch (m_state) {
    case State::Hidden:
        return HiddenColor;
    case State::FadeIn:
        return glm::mix(HiddenColor, InactiveColor, m_stateTime / FadeInTime);
    case State::Inactive:
        return InactiveColor;
    case State::Activating:
        return glm::mix(InactiveColor, ActiveColor, m_stateTime / ActivationTime);
    default:
        if (m_acquireTime > 0.0f) {
            float t = m_acquireTime / AcquireAnimationTime;
            return glm::mix(ActiveColor, AcquiredColor, t);
        }
        return ActiveColor;
    }
}

float UnitItem::labelAlpha() const
{
    constexpr const auto InactiveAlpha = 0.5f;
    constexpr const auto ActiveAlpha = 1.0f;
    switch (m_state) {
    case State::Hidden:
        return 0.0f;
    case State::FadeIn:
        return glm::mix(0.0f, InactiveAlpha, m_stateTime / FadeInTime);
    case State::Inactive:
        return InactiveAlpha;
    case State::Activating:
        return glm::mix(InactiveAlpha, ActiveAlpha, m_stateTime / ActivationTime);
    default:
        return ActiveAlpha;
    }
}

void UnitItem::initializeBoundingBox(UIPainter *painter) const
{
    // circle
    const GX::BoxF circleBox { glm::vec2(-Radius), glm::vec2(Radius) };

    // label
    painter->setFont(UnitLabelFont);
    const auto textSize = painter->textBoxSize(LabelTextWidth, m_unit->name);
    const auto p = glm::vec2(0, Radius + LabelMargin);
    const GX::BoxF labelBox {
        p - glm::vec2(0.5f * textSize.x + LabelMargin, LabelMargin),
        p + glm::vec2(0.5f * textSize.x + LabelMargin, textSize.y + LabelMargin)
    };

    m_boundingBox = circleBox | labelBox;
}

GX::BoxF UnitItem::boundingBox(UIPainter *painter) const
{
    if (!m_boundingBox)
        initializeBoundingBox(painter);
    return m_boundingBox + position();
}

void UnitItem::paint(UIPainter *painter) const
{
    auto p = position();

    // painter->drawRoundedRect(m_boundingBox + p, 8.0f, glm::vec4(0), glm::vec4(0, 1, 0, 1), 3.0f, -100);

    const auto color = this->color();
    painter->drawCircle(p, radius(), glm::vec4(0), color, 6.0f, -1);

    const auto labelAlpha = this->labelAlpha();

    if (m_world->canAcquire(m_unit)) {
        painter->drawGlowCircle(p, radius(), glm::vec4(0, 1, 1, 1), -2);
    } else {
        const auto acquirable = [this] {
            if (m_unit->type == Unit::Type::Generator)
                return true;
            return m_unit->count == 0;
        }();
        if (acquirable) {
            constexpr auto RadiusDelta = 8;
            constexpr auto EnergyColor = glm::vec3(1, 0.65, 0);
            constexpr auto MaterialColor = glm::vec3(0, 1, 1);
            constexpr auto ExtropyColor = glm::vec3(1, 0, 1);
            const auto addCircleGauge = [&p, painter](float radius, const glm::vec4 &color, float value) {
                constexpr auto StartAngle = 0;
                constexpr auto EndAngle = 1.5f * M_PI;
                float angle = StartAngle + value * (EndAngle - StartAngle);
                painter->drawCircleGauge(p, radius, 0.25f * color, color, StartAngle, EndAngle, angle, -2);
            };
            float r = radius() + RadiusDelta;
            const auto cost = m_world->actualCost(m_unit);
            if (m_unit->cost.energy > 0) {
                addCircleGauge(r, glm::vec4(EnergyColor, labelAlpha), std::min(static_cast<float>(m_world->state().energy / cost.energy), 1.0f));
                r += RadiusDelta;
            }
            if (m_unit->cost.material > 0) {
                addCircleGauge(r, glm::vec4(MaterialColor, labelAlpha), std::min(static_cast<float>(m_world->state().material / cost.material), 1.0f));
                r += RadiusDelta;
            }
            if (m_unit->cost.extropy > 0) {
                addCircleGauge(r, glm::vec4(ExtropyColor, labelAlpha), std::min(static_cast<float>(m_world->state().extropy / cost.extropy), 1.0f));
            }
        }
    }

    p += glm::vec2(0, Radius + LabelMargin);

    constexpr auto TextHeight = 80.0f;
    const auto textBox = GX::BoxF { p - glm::vec2(0.5f * LabelTextWidth, 0), p + glm::vec2(0.5f * LabelTextWidth, TextHeight) };
    painter->setVerticalAlign(UIPainter::VerticalAlign::Top);
    painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
    painter->setFont(UnitLabelFont);
    auto textSize = painter->drawTextBox(textBox, glm::vec4(1, 1, 1, labelAlpha), 2, m_unit->name);

    auto outerBox = GX::BoxF { p - glm::vec2(0.5f * textSize.x + LabelMargin, LabelMargin), p + glm::vec2(0.5f * textSize.x + LabelMargin, textSize.y + LabelMargin) };
    constexpr auto BoxRadius = 8.0f;
    painter->drawRoundedRect(outerBox, BoxRadius, glm::vec4(0, 0, 0, 0.75 * labelAlpha), glm::vec4(glm::vec3(color), labelAlpha), 3.0f, 1);

    const auto count = m_unit->count;
    if (count > 1) {
        const auto center = glm::vec2(outerBox.max.x, outerBox.min.y);
        constexpr const auto CounterRadius = 22.0f;

        painter->drawCircle(center, CounterRadius, glm::vec4(0, 0, 0, 0.75 * labelAlpha), glm::vec4(1, 1, 1, labelAlpha), 3.0f, 3);

        const auto counterBox = GX::BoxF { center - 0.5f * glm::vec2(CounterRadius), center + 0.5f * glm::vec2(CounterRadius) };
        painter->setVerticalAlign(UIPainter::VerticalAlign::Middle);
        painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
        painter->setFont(UIPainter::Font { FontName, 20 });
        painter->drawTextBox(counterBox, glm::vec4(1, 1, 1, labelAlpha), 4, fmt::format(U"x{}", count));
    }
}

bool UnitItem::contains(const glm::vec2 &pos) const
{
    if (m_state == UnitItem::State::Hidden)
        return false;
    return glm::distance(pos, position()) < radius();
}

bool UnitItem::handleMousePress()
{
    if (m_world->unitClicked(m_unit))
        m_acquireTime = AcquireAnimationTime;
    return true;
}

bool UnitItem::isVisible() const
{
    return m_state != State::Hidden;
}

World::World() = default;
World::~World() = default;

void World::initialize(UIPainter *painter, TechGraph *techGraph)
{
    m_painter = painter;
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
        const auto boundingBox = item->boundingBox(m_painter) + m_viewOffset;
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
        const auto box = GX::BoxF { glm::vec2(centerX - 0.5 * CounterWidth, centerY - 0.5 * CounterHeight), glm::vec2(centerX + 0.5 * CounterWidth, centerY + 0.5 * CounterHeight) };
        m_painter->drawRoundedRect(box, 20, glm::vec4(0, 0, 0, 0.75), glm::vec4(1, 1, 1, 1), 4.0f, TextDepth - 1);

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
            m_painter->drawText(glm::vec2(x, y), glm::vec4(1), TextDepth, label);
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
                m_painter->drawText(glm::vec2(left, y), glm::vec4(1), TextDepth, bigText);
                m_painter->drawText(glm::vec2(left + bigAdvance + smallAdvance, y), glm::vec4(1), TextDepth, unitText);

                m_painter->setFont(CounterFontSmall);
                m_painter->drawText(glm::vec2(left + bigAdvance, y), glm::vec4(1), TextDepth, smallText);
            } else {
                const auto text = fmt::format("{}{}", big, unit);
                m_painter->setFont(CounterFontBig);
                paintCentered(m_painter, centerX, y, glm::vec4(1), TextDepth, text);
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
            paintCentered(m_painter, centerX, y, glm::vec4(1), TextDepth, text);
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
    const auto cost = actualCost(m_currentUnit);

    static const auto TitleFont = UIPainter::Font { BoldFontName, 25 };
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
        m_painter->drawTextBox(GX::BoxF { p, p + glm::vec2(TitleTextWidth, titleSize.y) }, glm::vec4(1), 20, m_currentUnit->name);

        p += glm::vec2(0, titleSize.y);
        m_painter->setFont(DescriptionFont);
        m_painter->drawTextBox(GX::BoxF { p, p + glm::vec2(TextWidth, descriptionSize.y) }, glm::vec4(1), 20, m_currentUnit->description);

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
            m_painter->drawText(p, glm::vec4(1, 1, 0, 1), 20, prefix);
            p += glm::vec2(m_painter->horizontalAdvance(prefix), 0);
            const auto drawYield = [this, &p](const std::u32string &text, const GX::PackedPixmap &icon) {
                if (text.empty())
                    return;
                const auto textHeight = m_painter->font()->ascent() + m_painter->font()->descent();
                m_painter->drawPixmap(glm::vec2(p.x, p.y - 0.5f * (textHeight + icon.height)), icon, 20);
                p.x += icon.width;
                m_painter->drawText(p, glm::vec4(1, 1, 0, 1), 20, text);
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

        const auto drawCost = [this, &p](const std::u32string &text, const GX::PackedPixmap &icon) {
            if (text.empty())
                return;
            const auto advance = m_painter->horizontalAdvance(text) + icon.width;
            const auto textHeight = m_painter->font()->ascent() + m_painter->font()->descent();
            auto x = p.x - advance;
            m_painter->drawPixmap(glm::vec2(x, p.y - 0.5f * (textHeight + icon.height)), icon, 20);
            x += icon.width;
            m_painter->drawText(glm::vec2(x, p.y), glm::vec4(1), 20, text);
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
    m_painter->drawRoundedRect(outerBox, BoxRadius, glm::vec4(0, 0, 0, 0.75), glm::vec4(1), 3.0f, 19);
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

StateVector World::actualCost(const Unit *unit) const
{
    return unit->cost * powf(1.2f, unit->count);
}

bool World::unitClicked(Unit *unit)
{
    m_currentUnit = unit;

    if (!canAcquire(unit))
        return false;

    m_state -= actualCost(unit);
    ++unit->count;
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
    const auto cost = actualCost(unit);
    return cost.extropy <= m_state.extropy && cost.energy <= m_state.energy && cost.material <= m_state.material;
}
