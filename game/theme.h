#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>

struct Theme {
    struct TextBox {
        glm::vec4 backgroundColor;
        glm::vec4 outlineColor;
        float outlineThickness = 0.0f;
        glm::vec4 textColor;
    };
    struct Unit {
        glm::vec4 color;
        TextBox label;
        TextBox counter;
    };
    struct GaugeColors {
        glm::vec4 energy;
        glm::vec4 material;
        glm::vec4 extropy;
    };
    struct Counter {
        glm::vec4 backgroundColor;
        glm::vec4 outlineColor;
        float outlineThickness = 0.0f;
        glm::vec4 labelColor;
        glm::vec4 valueColor;
        glm::vec4 deltaColor;
    };
    struct UnitDetails {
        glm::vec4 backgroundColor;
        glm::vec4 outlineColor;
        float outlineThickness = 0.0f;
        glm::vec4 titleColor;
        glm::vec4 descriptionColor;
        glm::vec4 yieldColor;
        glm::vec4 costColor;
    };
    glm::vec4 backgroundColor;
    glm::vec4 glowColor;
    GaugeColors gaugeColors;
    Counter counter;
    UnitDetails unitDetails;
    Unit inactiveUnit;
    Unit activeUnit;
    Unit selectedUnit;

    bool load(const std::string &jsonPath);
};
