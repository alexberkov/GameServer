#include "json_loader.h"
#include <fstream>

namespace json_loader {

void LoadRoad(model::Map &map, const json::object &road_json, std::uint32_t& id) {
    model::Point roadStart{};
    roadStart.x = value_to<int>(road_json.at("x0"));
    roadStart.y = value_to<int>(road_json.at("y0"));
    model::Coord roadEnd;
    if (road_json.contains("x1")) {
        roadEnd = value_to<int>(road_json.at("x1"));
        map.AddRoad(model::Road{model::Road::HORIZONTAL, roadStart, roadEnd, id});
    } else {
        roadEnd = value_to<int>(road_json.at("y1"));
        map.AddRoad(model::Road{model::Road::VERTICAL, roadStart, roadEnd, id});
    }
    ++id;
}

void LoadBuilding(model::Map &map, const json::object &building_json) {
    model::Point pos{};
    pos.x = value_to<int>(building_json.at("x"));
    pos.y = value_to<int>(building_json.at("y"));
    model::Size size{};
    size.width = value_to<int>(building_json.at("w"));
    size.height = value_to<int>(building_json.at("h"));
    map.AddBuilding(model::Building{{pos, size}});
}

void LoadOffice(model::Map &map, const json::object &office_json) {
    util::Tagged<std::string, model::Office> officeId(value_to<std::string>(office_json.at("id")));
    model::Point pos{};
    pos.x = value_to<int>(office_json.at("x"));
    pos.y = value_to<int>(office_json.at("y"));
    model::Offset offset{};
    offset.dx = value_to<int>(office_json.at("offsetX"));
    offset.dy = value_to<int>(office_json.at("offsetY"));
    map.AddOffice({officeId, pos, offset});
}

void LoadMap(model::Game &game, const json::object &map_json, double defaultDogSpeed, size_t defaultBagCapacity) {
    json::object static_map;
    auto map_id_str = value_to<std::string>(map_json.at("id"));
    auto mapName = value_to<std::string>(map_json.at("name"));
    static_map.emplace("id", map_id_str);
    static_map.emplace("name", mapName);
    util::Tagged<std::string, model::Map> mapId(map_id_str);

    double dogSpeed = defaultDogSpeed;
    if (map_json.contains("dogSpeed"))
        dogSpeed = value_to<double>(map_json.at("dogSpeed"));

    size_t bagCapacity = defaultBagCapacity;
    if (map_json.contains("bagCapacity"))
        bagCapacity = value_to<size_t>(map_json.at("bagCapacity"));

    auto roads = map_json.at("roads").as_array();
    auto buildings = map_json.at("buildings").as_array();
    auto offices = map_json.at("offices").as_array();
    auto lootTypes = map_json.at("lootTypes").as_array();

    static_map.emplace("roads", roads);
    static_map.emplace("buildings", buildings);
    static_map.emplace("offices", offices);
    static_map.emplace("lootTypes", lootTypes);
    model::Map map {mapId, mapName, dogSpeed, bagCapacity, json::serialize(static_map)};

    size_t type = 0;
    for (auto &loot: lootTypes) {
        auto value = value_to<size_t>(loot.as_object().at("value"));
        map.AddObject(type, value);
        ++type;
    }

    std::uint32_t road_id = 0;
    for (auto road_json: roads) {
        LoadRoad(map, road_json.as_object(), road_id);
    }
    map.FillIntersections();
    for (auto building_json: buildings)
        LoadBuilding(map, building_json.as_object());
    for (auto office_json: offices)
        LoadOffice(map, office_json.as_object());
    map.SetLootTypes(lootTypes.size());

    game.AddMap(map);
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream ifs;
    ifs.open(json_path);
    if (!ifs.is_open())
        throw std::invalid_argument("Incorrect filepath");
    std::string game_str(std::istreambuf_iterator<char>(ifs), {});
    auto game_json = json::parse(game_str);

    model::Game game;

    double defaultDogSpeed = 1.0;
    if (game_json.as_object().contains("defaultDogSpeed"))
        defaultDogSpeed = value_to<double>(game_json.as_object().at("defaultDogSpeed"));

    size_t defaultBagCapacity = 3;
    if (game_json.as_object().contains("defaultBagCapacity"))
        defaultBagCapacity = value_to<size_t>(game_json.as_object().at("defaultBagCapacity"));

    auto lootGenerator = game_json.as_object().at("lootGeneratorConfig");
    auto lootGenPeriod = value_to<double>(lootGenerator.as_object().at("period"));
    auto lootGenProbability = value_to<double>(lootGenerator.as_object().at("probability"));
    game.SetLootGenParams(lootGenPeriod, lootGenProbability);

    double defaultRetirementTime = 60.0;
    if (game_json.as_object().contains("dogRetirementTime"))
        defaultRetirementTime = value_to<double>(game_json.as_object().at("dogRetirementTime"));

    auto retirementTimeMs = static_cast<size_t>(defaultRetirementTime * 1000);
    game.SetRetirementParams(retirementTimeMs);

    auto maps = game_json.as_object().at("maps").as_array();
    for (auto map_json: maps) LoadMap(game, map_json.as_object(), defaultDogSpeed, defaultBagCapacity);

    return game;
}

}  // namespace json_loader
