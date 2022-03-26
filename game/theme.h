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
    glm::vec4 backgroundColor;
    TextBox counter;
    TextBox unitDetails;
    Unit inactiveUnit;
    Unit activeUnit;
    Unit selectedUnit;

    bool load(const std::string &jsonPath);
};
