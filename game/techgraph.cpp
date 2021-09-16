#include "techgraph.h"

#include <ioutil.h>

#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

bool TechGraph::load(const std::string &jsonPath)
{
    units.clear();

    const auto json = GX::Util::readFile(jsonPath);
    if (!json) {
        spdlog::warn("Failed to read graph file {}", jsonPath);
        return false;
    }

    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char *>(json->data()));
    if (document.HasParseError()) {
        spdlog::warn("Failed to parse graph file {}", jsonPath);
        return false;
    }

    const auto &unitsArray = document["units"];
    assert(unitsArray.IsArray());

    const auto unitsCount = unitsArray.Size();

    units.reserve(unitsCount);
    std::generate_n(std::back_inserter(units), unitsCount, [] { return std::make_unique<Unit>(); });

    for (size_t i = 0; i < unitsCount; ++i) {
        const auto &unitSettings = unitsArray[i];
        auto &unit = units[i];
        unit->name = unitSettings["name"].GetString();
        unit->description = unitSettings["description"].GetString();
        const auto &positionArray = unitSettings["position"];
        assert(positionArray.IsArray());
        unit->position = glm::vec2(positionArray[0].GetFloat(), positionArray[1].GetFloat());
        const auto &dependenciesArray = unitSettings["dependencies"];
        assert(dependenciesArray.IsArray());
        for (size_t j = 0, size = dependenciesArray.Size(); j < size; ++j) {
            const auto index = dependenciesArray[j].GetInt();
            assert(index >= 0 && index < unitsCount);
            unit->dependencies.push_back(units[index].get());
        }
    }

    return true;
}
