#include "world.h"

#include "uipainter.h"

#include <fontcache.h>

#include <fmt/format.h>

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

    void update(double elapsed) override;
    void paint(UIPainter *painter) const override;
    bool contains(const glm::vec2 &pos) const override;

    void handleMousePress() override;

private:
    Unit *m_unit;

    static constexpr auto Radius = 50.0f;
};

UnitItem::UnitItem(Unit *unit, World *world)
    : GraphItem(world)
    , m_unit(unit)
{
}

UnitItem::~UnitItem() = default;

void UnitItem::update(double elapsed)
{
    // XXX
}

void UnitItem::paint(UIPainter *painter) const
{
    constexpr auto FontSize = 20;
    static const auto LabelFont = UIPainter::Font { FontName, FontSize };
    painter->setFont(LabelFont);

    const auto color = m_hovered ? glm::vec4(1, 1, 1, .5) : glm::vec4(1, 1, 1, .25);

    painter->drawCircle(m_unit->position, Radius, color, -1);

    const auto textBox = GX::BoxF { m_unit->position - glm::vec2(Radius, Radius), m_unit->position + glm::vec2(Radius, Radius) };

    painter->setVerticalAlign(UIPainter::VerticalAlign::Middle);
    painter->setHorizontalAlign(UIPainter::HorizontalAlign::Center);
    painter->drawTextBox(textBox, glm::vec4(1), 0, m_unit->name);
}

bool UnitItem::contains(const glm::vec2 &pos) const
{
    return glm::distance(pos, m_unit->position) < Radius;
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
        m_graphItems.emplace_back(new UnitItem(unit.get(), this));
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
    for (auto &item : m_graphItems)
        item->paint(painter);
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
