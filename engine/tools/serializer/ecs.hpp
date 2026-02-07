#pragma once
#include <set>
#include <optional>
#include <JsonReflect.hpp>
#include "engine/core/entity.hpp"
#include "engine/tools/json.hpp"
#include "engine/tools/types/tree_map.hpp"
#include "engine/core/components/prefab.hpp" /* for PrefabInstanceID */

namespace tmt {

class Ecs;

}  // namespace tmt

/* ECS */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data = std::nullopt);

/* Single entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data = std::nullopt);

/* Set of entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, std::set<tmt::Entity>& entities, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data = std::nullopt);

/* Custom implementations */
/* forward declare */
namespace tmt {
struct SerializeState;
struct DeserializeState;
}  // namespace tmt

/* Single entity with states */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, tmt::SerializeState& state);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const tmt::DeserializeState& state);


/*
Needs sorting after transformation:
std::set<Entity>
std::multiset<Entity>
std::map<Entity, T>
std::multimap<Entity, T>
std::priority_queue<Entity>
*/

// Specialization for std::set
template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::set<T>& set, tmt::SerializeState& state) {
    std::vector<tmt::json> elements;
    for (const auto& item : set) {
        elements.push_back(tmt::Serializer::serialize(item, state));
    }
    std::sort(elements.begin(), elements.end());
    return elements;
}

// Specialization for std::multiset
template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::multiset<T>& set, tmt::SerializeState& state) {
    std::vector<tmt::json> elements;
    for (const auto& item : set) {
        elements.push_back(tmt::Serializer::serialize(item, state));
    }
    std::sort(elements.begin(), elements.end());
    return elements;
}

// Specialization for std::map
template <typename K, typename V>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::map<K, V>& map, tmt::SerializeState& state) {
    // Transform keys first, then sort
    std::vector<std::pair<tmt::json, tmt::json>> transformed;
    for (const auto& [key, value] : map) {
        transformed.emplace_back(tmt::Serializer::serialize(key, state), tmt::Serializer::serialize(value, state));
    }

    // Sort by transformed keys
    std::sort(transformed.begin(), transformed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    // Build JSON object
    tmt::json result = tmt::json::object();
    for (const auto& [k, v] : transformed) {
        result[k.dump()] = v;  // nlohmann::json requires string keys
    }
    return result;
}

// Specialization for std::unordered_set (for consistent diffing)
template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::unordered_set<T>& set, tmt::SerializeState& state) {
    std::vector<tmt::json> elements;
    for (const auto& item : set) {
        elements.push_back(tmt::Serializer::serialize(item, state));
    }
    std::sort(elements.begin(), elements.end());  // Sort for deterministic output
    return elements;
}

// Specialization for std::unordered_map
template <typename K, typename V>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::unordered_map<K, V>& map, tmt::SerializeState& state) {
    std::vector<std::pair<tmt::json, tmt::json>> transformed;
    for (const auto& [key, value] : map) {
        transformed.emplace_back(tmt::Serializer::serialize(key, state), tmt::Serializer::serialize(value, state));
    }

    std::sort(transformed.begin(), transformed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    tmt::json result = tmt::json::object();
    for (const auto& [k, v] : transformed) {
        result[k.dump()] = v;
    }
    return result;
}