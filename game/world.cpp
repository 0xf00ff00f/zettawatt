#include "world.h"

#include "uipainter.h"

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
constexpr auto UnitRadius = 50.0f;

bool unitContains(const Unit *unit, const glm::vec2 &pos)
{
    return glm::distance(pos, unit->position) < UnitRadius;
}
} // namespace

World::World()
{
    m_state.extropy = 0;
    m_state.energy = 2100;
    m_state.material = 2000;
    m_state.carbon = 0;

    {
        m_techGraph = std::make_unique<TechGraph>();

        auto mine = std::make_unique<Unit>();
        mine->name = "Open-pit Mine"s;
        mine->position = glm::vec2(-100, 200);
        mine->cost = {
            .energy = 300,
        };

        auto furnace = std::make_unique<Unit>();
        furnace->name = "Blast Furnace"s;
        furnace->position = glm::vec2(-200, -100);
        furnace->cost = StateVector {
            .energy = 100,
            .material = 200,
        };
        furnace->yield = StateVector {
            .material = 800,
            .carbon = 300
        };

        auto oilRig = std::make_unique<Unit>();
        oilRig->name = "Oil Rig"s;
        oilRig->position = glm::vec2(100, 200);
        oilRig->cost = StateVector {
            .material = 100,
        };

        auto powerPlant = std::make_unique<Unit>();
        powerPlant->name = "Thermal Power Plant"s;
        powerPlant->position = glm::vec2(200, -100);
        powerPlant->cost = StateVector {
            .material = 100,
        };
        powerPlant->yield = StateVector {
            .energy = 200,
            .carbon = 300,
        };

        m_techGraph->units.push_back(std::move(mine));
        m_techGraph->units.push_back(std::move(furnace));
        m_techGraph->units.push_back(std::move(oilRig));
        m_techGraph->units.push_back(std::move(powerPlant));
    }
}

World::~World() = default;

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

namespace {
template<typename StringT>
void paintCentered(UIPainter *painter, float x, float y, const glm::vec4 &color, int depth, const StringT &s)
{
    const auto advance = painter->horizontalAdvance(s);
    painter->drawText(glm::vec2(x - 0.5f * advance, y), color, depth, s);
}
} // namespace

void World::paintGraph(UIPainter *painter) const
{
    static const auto LabelFont = UIPainter::Font { FontName, 30 };
    painter->setFont(LabelFont);

    for (const auto &unit : m_techGraph->units) {
        const auto color = unit->hovered ? glm::vec4(1, 1, 1, .5) : glm::vec4(1, 1, 1, .25);
        painter->drawCircle(unit->position, UnitRadius, color, -1);
        paintCentered(painter, unit->position.x, unit->position.y, glm::vec4(1), 0, unit->name);
    }
}

void World::paintState(UIPainter *painter) const
{
    constexpr auto CounterWidth = 320.0f;
    constexpr auto CounterHeight = 160.0f;

    auto paintCounter = [painter](float centerX, float centerY, const std::u32string &label, const std::string &unit, double value, double delta) {
        painter->drawRoundedRect(glm::vec2(centerX - 0.5 * CounterWidth, centerY - 0.5 * CounterHeight), glm::vec2(centerX + 0.5 * CounterWidth, centerY + 0.5 * CounterHeight), 20, glm::vec4(1, 1, 1, 0.25), -1);

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
    const auto &units = m_techGraph->units;
    for (auto &unit : m_techGraph->units) {
        if (unitContains(unit.get(), pos))
            unitClicked(unit.get());
    }
}

void World::mouseReleaseEvent(const glm::vec2 &pos)
{
}

void World::mouseMoveEvent(const glm::vec2 &pos)
{
    const auto &units = m_techGraph->units;
    for (auto &unit : m_techGraph->units) {
        unit->hovered = unitContains(unit.get(), pos);
    }
}

void World::unitClicked(Unit *unit)
{
    if (!canAcquire(unit))
        return;

    ++unit->count;
    m_state -= unit->cost;

    updateStateDelta();
}

bool World::canAcquire(const Unit *unit) const
{
    const auto &cost = unit->cost;
    return cost.extropy <= m_state.extropy && cost.energy <= m_state.energy && cost.material <= m_state.material;
}
