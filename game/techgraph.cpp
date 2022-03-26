#include "techgraph.h"

#include <codecvt>
#include <ioutil.h>
#include <locale>

#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

namespace {
std::u32string utf8ToUtf32(const std::string &s)
{
    static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cv;
    return cv.from_bytes(s.data());
}

StateVector loadStateVector(const rapidjson::Value &value)
{
    return {
        value["extropy"].GetDouble(),
        value["energy"].GetDouble(),
        value["material"].GetDouble(),
        value["carbon"].GetDouble()
    };
}
} // namespace

bool TechGraph::load(const std::string &jsonPath)
{
    units.clear();

    auto json = GX::Util::readFile(jsonPath);
    if (!json) {
        spdlog::warn("Failed to read graph file {}", jsonPath);
        return false;
    }
    json->push_back('\0');

    rapidjson::Document document;
    rapidjson::ParseResult ok = document.Parse(reinterpret_cast<const char *>(json->data()));
    if (!ok) {
        spdlog::warn("Failed to parse graph file {}, error: {}, offset: {}", jsonPath, ok.Code(), ok.Offset());
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
        unit->name = utf8ToUtf32(unitSettings["name"].GetString());
        unit->description = utf8ToUtf32(unitSettings["description"].GetString());
        unit->type = [type = unitSettings["type"].GetString()] {
            return type == "Booster"s ? Unit::Type::Booster : Unit::Type::Generator;
        }();
        const auto &positionArray = unitSettings["position"];
        assert(positionArray.IsArray());
        unit->position = glm::vec2(positionArray[0].GetDouble(), positionArray[1].GetDouble());
        unit->baseCost = loadStateVector(unitSettings["cost"]);
        unit->yield = loadStateVector(unitSettings["yield"]);
        unit->boost = [this, &unitSettings] {
            const rapidjson::Value &boost = unitSettings["boost"];
            const auto factor = boost["factor"].GetDouble();
            const auto targetIndex = boost["target"].GetInt();
            const Unit *target = targetIndex >= 0 && targetIndex < units.size() ? units[targetIndex].get() : nullptr;
            return Boost { factor, target };
        }();
        const auto &dependenciesArray = unitSettings["dependencies"];
        assert(dependenciesArray.IsArray());
        for (const auto &value : dependenciesArray.GetArray()) {
            const auto index = value.GetInt();
            assert(index >= 0 && index < units.size());
            unit->dependencies.push_back(units[index].get());
        }
    }

    return true;
}
