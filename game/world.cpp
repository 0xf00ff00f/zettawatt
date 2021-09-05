#include "world.h"

#include "uipainter.h"

#include <fontcache.h>

#include <fmt/format.h>
#include <glm/gtc/random.hpp>

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

template<typename StringT>
void paintCentered(UIPainter *painter, float x, float y, const glm::vec4 &color, int depth, const StringT &s)
{
    const auto advance = painter->horizontalAdvance(s);
    painter->drawText(glm::vec2(x - 0.5f * advance, y), color, depth, s);
}

} // namespace

GraphItem::GraphItem(World *world)
    : m_world(world)
{
}

GraphItem::~GraphItem() = default;

void GraphItem::mousePressEvent(const glm::vec2 &pos)
{
    if (contains(pos))
        handleMousePress();
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

void GraphItem::handleMousePress()
{
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

    void handleMousePress() override;

private:
    Unit *m_unit;
    glm::vec2 m_wobbleDirection;
    float m_localTime = 0.0f;
    static constexpr auto Radius = 20.0f;
};

UnitItem::UnitItem(Unit *unit, World *world)
    : GraphItem(world)
    , m_unit(unit)
    , m_wobbleDirection(glm::circularRand(12.0f))
{
}

UnitItem::~UnitItem() = default;

void UnitItem::update(double elapsed)
{
    m_localTime += elapsed;
}

glm::vec2 UnitItem::position() const
{
    return m_unit->position + sinf(3.0f * m_localTime) * m_wobbleDirection;
}

float UnitItem::radius() const
{
    return m_hovered ? 1.2f * Radius : Radius;
}

void UnitItem::paint(UIPainter *painter) const
{
    constexpr auto FontSize = 30;
    static const auto LabelFont = UIPainter::Font { FontName, FontSize };
    painter->setFont(LabelFont);

    const auto p = position();
    painter->drawCircle(p, radius(), glm::vec4(1), -1);

    const auto textBox = GX::BoxF { p + glm::vec2(-2.0 * Radius, Radius), p + glm::vec2(2.0 * Radius, 2.0 + Radius) };

    painter->setVerticalAlign(UIPainter::VerticalAlign::Top);
    painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
    painter->drawTextBox(textBox, glm::vec4(1), 0, m_unit->name);
}

bool UnitItem::contains(const glm::vec2 &pos) const
{
    return glm::distance(pos, position()) < radius();
}

void UnitItem::handleMousePress()
{
    m_world->unitClicked(m_unit);
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
        for (auto &dependency : unit->dependencies) {
            const auto *toUnit = m_unitItems[dependency.unit];
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
        for (const auto &project : m_techGraph->projects) {
            if (project->enabled && project->parent == unit.get())
                yield *= project->boost;
        }
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
    for (auto [from, to] : m_edges) {
        constexpr auto NodeBorder = 4.0f;

        auto fromPosition = from->position();
        auto toPosition = to->position();
        const auto d = glm::normalize(fromPosition - toPosition);
        fromPosition -= (from->radius() - NodeBorder) * d;
        toPosition += (to->radius() - NodeBorder) * d;
        painter->drawThickLine(fromPosition, toPosition, 5, glm::vec4(1), -1);
    }

    for (auto &item : m_graphItems) {
        item->paint(painter);
    }
}

void World::paintState(UIPainter *painter) const
{
    constexpr auto CounterWidth = 320.0f;
    constexpr auto CounterHeight = 160.0f;

    auto paintCounter = [painter](float centerX, float centerY, const std::u32string &label, const std::string &unit, double value, double delta) {
        const auto box = GX::BoxF { glm::vec2(centerX - 0.5 * CounterWidth, centerY - 0.5 * CounterHeight), glm::vec2(centerX + 0.5 * CounterWidth, centerY + 0.5 * CounterHeight) };
        painter->drawRoundedRect(box, 20, glm::vec4(1, 1, 1, 0.25), -1);

        static const auto LabelFont = UIPainter::Font { FontName, 40 };
        static const auto CounterFontBig = UIPainter::Font { FontName, 80 };
        static const auto CounterFontSmall = UIPainter::Font { FontName, 40 };
        static const auto DeltaFont = UIPainter::Font { FontName, 40 };

        float y = centerY - 40;

        // label
        {
            painter->setFont(LabelFont);
            paintCentered(painter, centerX, y, glm::vec4(1), 0, label);
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
                painter->drawText(glm::vec2(left, y), glm::vec4(1), 0, bigText);
                painter->drawText(glm::vec2(left + bigAdvance + smallAdvance, y), glm::vec4(1), 0, unitText);

                painter->setFont(CounterFontSmall);
                painter->drawText(glm::vec2(left + bigAdvance, y), glm::vec4(1), 0, smallText);
            } else {
                const auto text = fmt::format("{}{}", big, unit);
                painter->setFont(CounterFontBig);
                paintCentered(painter, centerX, y, glm::vec4(1), 0, text);
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
            paintCentered(painter, centerX, y, glm::vec4(1), 0, text);
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
    for (auto &item : m_graphItems)
        item->mousePressEvent(pos);
}

void World::mouseReleaseEvent(const glm::vec2 &pos)
{
    for (auto &item : m_graphItems)
        item->mouseReleaseEvent(pos);
}

void World::mouseMoveEvent(const glm::vec2 &pos)
{
    for (auto &item : m_graphItems)
        item->mouseMoveEvent(pos);
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
    const auto &cost = unit->cost;
    return cost.extropy <= m_state.extropy && cost.energy <= m_state.energy && cost.material <= m_state.material;
}
