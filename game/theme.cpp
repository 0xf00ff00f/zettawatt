#include "theme.h"

#include <ioutil.h>

#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <cassert>

namespace {
glm::vec4 parseColor(const rapidjson::Value &value)
{
    assert(value.IsArray());
    return glm::vec4(value[0].GetFloat(), value[1].GetFloat(), value[2].GetFloat(), value[3].GetFloat());
}

Theme::TextBox parseTextBox(const rapidjson::Value &value)
{
    assert(value.IsObject());
    Theme::TextBox rv;
    rv.backgroundColor = parseColor(value["backgroundColor"]);
    rv.outlineColor = parseColor(value["outlineColor"]);
    rv.outlineThickness = value["outlineThickness"].GetFloat();
    rv.textColor = parseColor(value["textColor"]);
    return rv;
}

Theme::Unit parseUnit(const rapidjson::Value &value)
{
    assert(value.IsObject());
    Theme::Unit rv;
    rv.color = parseColor(value["color"]);
    rv.label = parseTextBox(value["label"]);
    rv.counter = parseTextBox(value["counter"]);
    return rv;
}
} // namespace

bool Theme::load(const std::string &jsonPath)
{
    auto json = GX::Util::readFile(jsonPath);
    if (!json) {
        spdlog::warn("Failed to read theme file {}", jsonPath);
        return false;
    }
    json->push_back('\0');

    rapidjson::Document document;
    rapidjson::ParseResult ok = document.Parse(reinterpret_cast<const char *>(json->data()));
    if (!ok) {
        spdlog::warn("Failed to parse theme file {}", jsonPath);
        return false;
    }

    backgroundColor = parseColor(document["backgroundColor"]);
    counter = parseTextBox(document["counter"]);
    unitDetails = parseTextBox(document["unitDetails"]);
    inactiveUnit = parseUnit(document["inactiveUnit"]);
    activeUnit = parseUnit(document["activeUnit"]);
    selectedUnit = parseUnit(document["selectedUnit"]);

    return true;
}
