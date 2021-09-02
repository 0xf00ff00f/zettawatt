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
} // namespace

World::World()
{
    m_stateDelta.extropy = 123;
    m_stateDelta.energy = 4113;
    m_stateDelta.materials = 1234;
    m_stateDelta.carbon = 2345;
}

World::~World() = default;

void World::update(double elapsed)
{
    m_state += m_stateDelta * elapsed;
}

void World::updateStateDelta()
{
    StateVector delta;
    for (const auto &unit : m_techGraph) {
        if (!unit->count)
            continue;
        StateVector generated = unit->generated;
        for (const auto &booster : unit->boosters) {
            if (booster.enabled)
                generated *= booster.boost;
        }
        delta += unit->count * generated;
    }
    m_stateDelta = delta;
}

void World::paint(UIPainter *painter) const
{
    paintState(painter);
}

void World::paintState(UIPainter *painter) const
{
    const auto sceneBox = painter->sceneBox();

    auto paintCounter = [painter](float centerX, const std::u32string &label, const std::string &unit, double value, double delta) {
        static const char *fontName = "IBMPlexSans-Regular.ttf";
        static const auto LabelFont = UIPainter::Font { fontName, 40 };
        static const auto CounterFontBig = UIPainter::Font { fontName, 80 };
        static const auto CounterFontSmall = UIPainter::Font { fontName, 40 };
        static const auto DeltaFont = UIPainter::Font { fontName, 40 };

        float y = 0;

        auto paintCentered = [painter, centerX, &y](auto s) {
            const auto advance = painter->horizontalAdvance(s);
            painter->drawText(glm::vec2(centerX - 0.5f * advance, y), glm::vec4(1), 0, s);
        };

        // label
        {
            painter->setFont(LabelFont);
            paintCentered(label);
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
                const auto text = fmt::format("{}", big);
                painter->setFont(CounterFontBig);
                paintCentered(text);
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
            paintCentered(text);
        }
    };

    constexpr auto CounterWidth = 300.0f;

    paintCounter(-1.5f * CounterWidth, U"EXTROPY"s, ""s, m_state.extropy, m_stateDelta.extropy);
    paintCounter(-0.5f * CounterWidth, U"ENERGY"s, "Wh"s, m_state.energy, m_stateDelta.energy);
    paintCounter(0.5f * CounterWidth, U"MATERIALS"s, "t"s, m_state.materials, m_stateDelta.materials);
    paintCounter(1.5f * CounterWidth, U"CO\U00002082"s, "t"s, m_state.carbon, m_stateDelta.carbon);
}
