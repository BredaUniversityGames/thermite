#include "ecs.hpp"
#include "engine/tools/serializer.hpp"

#include "engine/core/entity.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/reflection.hpp"

#include "engine/core/components/all.hpp"
#include "engine/core/components/transform.hpp"

#include "engine/tools/serializer/all.hpp"
#include "engine/tools/fmt/nlohmann.hpp"
#include "engine/tools/profiler.hpp"

using Json = JsonReflect::json;

class SceneJson {
   private:
    Json json;

   public:
    SceneJson() = default;
    SceneJson(const Json& j) : json(j) {}

    const Json& get_json() const { return json; }
    std::string dump() const;

    /* version */
    bool has_version() const;
    uint64_t get_version() const;
    void set_version(uint64_t version);

    /* entities */
    bool has_entities() const;
    Json& entities();
    const Json& entities() const;

    /* components */
    bool has_components(const std::string& component_name) const;
    Json& components(const std::string& component_name);
    const Json& components(const std::string& component_name) const;

    /* entity entry */
    bool has_entity_entry(const Json& container, const tmt::Entity& entity) const;
    void entity_entry(Json& container, const tmt::Entity& entity, const Json& value) const;
    Json& entity_entry(Json& container, const tmt::Entity& entity) const;
};

struct SerializeState {
    const tmt::Ecs& ecs;
    /* Original provided entities */
    const std::set<tmt::Entity>& original_entities;
    /* Originals + children */
    const std::set<tmt::Entity>& all_entities;

    SceneJson json;
};

struct DeserializeState {
    tmt::Ecs& ecs;
    const SceneJson& json;

    std::unordered_map<tmt::Entity, tmt::Entity> entity_mapping {{entt::null, entt::null}}; /* Default null to null value */
};

/* Entt entity with state */
static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const DeserializeState& state) {
    tmt::Entity json_entity = entt::null;
    tmt::Serializer::deserialize(j, json_entity);

    if (state.entity_mapping.contains(json_entity)) {
        entity = state.entity_mapping.at(json_entity);
    } else {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Entity mapping does not contain entity {}", json_entity);
        entity = json_entity;
    }
}

/* ECS */
Json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs) {
    std::set<tmt::Entity> entities;
    for (const auto entity : ecs.get_registry().view<entt::entity>()) {
        entities.insert(entity);
    }
    return tmt::Serializer::serialize(entities, ecs);
}

void tag_invoke(JsonReflect::deserialize_t, const Json& j, tmt::Ecs& ecs) {
    std::set<tmt::Entity> new_entities;
    tmt::Serializer::deserialize(j, new_entities, ecs);
}

/* Single entity */
Json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs) {
    const std::set<tmt::Entity> entities {entity};
    return tmt::Serializer::serialize(entities, ecs);
}

void tag_invoke(JsonReflect::deserialize_t, const Json&, tmt::Entity&, tmt::Ecs&) {}

/* [Serialize] Set of entities */
Json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities_original, const tmt::Ecs& ecs) {
    TMT_ZONE_SCOPED_NS("Ecs::serialize");

    /* Add children */
    auto entities = entities_original;
    for (const auto entity : entities_original) {
        entities.merge(ecs.get_component<tmt::Transform>(entity).get_all_children());
    }

    SerializeState state {ecs, entities_original, entities, SceneJson()};

    state.json.set_version(tmt::Serializer::Config::VERSION);

    for (const auto entity : entities) {
        state.json.entities().push_back(entity);
    }

    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;

        /* If no component instance exists, continue */
        if (state.ecs.get_registry().view<ComponentType>().size() <= 0) return;

        constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
        TMT_ZONE_SCOPED_N(COMPONENT_NAME);

        Json& components = state.json.components(COMPONENT_NAME);

        for (const auto entity : state.all_entities) {
            if (state.ecs.has_component<ComponentType>(entity) == false) continue;

            const auto value = tmt::Serializer::serialize(state.ecs.get_component<ComponentType>(entity));
            state.json.entity_entry(components, entity, value);
        }
    });

    return state.json.get_json();
}

/* [Deserialize] */
void tag_invoke(JsonReflect::deserialize_t, const Json& j, std::set<tmt::Entity>& new_entities, tmt::Ecs& ecs) {
    TMT_ZONE_SCOPED_NS("Ecs::deserialize");

    DeserializeState state {ecs, SceneJson(j)};

    if (state.json.entities() == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"entities\" object. Json:\n---\n%s\n---", j.dump());
        return;
    }
    if (state.json.has_version() == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", j.dump());
        return;
    }

    const uint64_t version = state.json.get_version();
    if (version != tmt::Serializer::Config::VERSION) {
        tmt::Log::warn(
            tmt::Log::Scope::ENGINE, "[Serialization] Using outdated version, something might break. current version: %ui, provided version: %ui", version, tmt::Serializer::Config::VERSION
        );
    }

    const auto& json_entities = state.json.entities();

    /* Create entities */
    for (const auto& json_entity : json_entities) {
        tmt::Entity deserialized_entity = entt::null;
        tmt::Serializer::deserialize(json_entity, deserialized_entity);

        if (deserialized_entity == entt::null) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] \"Entities\" object contain an entt::null entity.");
            continue;
        }

        const tmt::Entity created_entity = state.ecs.create_entity("", deserialized_entity);
        state.entity_mapping[deserialized_entity] = created_entity;
    }

    /* Deserialize each component */
    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;

        constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
        TMT_ZONE_SCOPED_N(COMPONENT_NAME);

        if (state.json.has_components(COMPONENT_NAME) == false) return;

        const Json& entries = state.json.components(COMPONENT_NAME);
        for (const auto& [json_key, json_value] : entries.items()) {
            const tmt::Entity json_entity = tmt::EntityHelper::from_string(json_key);

            if (json_entity == entt::null) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] %s objects contain an entt::null entry.", COMPONENT_NAME);
                continue;
            }

            const tmt::Entity entity = state.entity_mapping.at(json_entity);
            ComponentType& component_value = state.ecs.add_or_get_component<ComponentType>(entity);
            tmt::Serializer::deserialize(json_value, component_value, state);
        }
    });

    /* Return newly created entities */
    for (const auto& [_, mapped_entity] : state.entity_mapping) {
        if (mapped_entity != entt::null) {
            new_entities.insert(mapped_entity);
        }
    }
}

std::string SceneJson::dump() const { return json.dump(4); }

bool SceneJson::has_version() const { return json.contains("version"); }

uint64_t SceneJson::get_version() const {
    if (json.contains("version")) {
        return json.at("version").get<uint64_t>();
    }
    tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", json.dump());
    return 0;
}

void SceneJson::set_version(uint64_t version) { json["version"] = version; }

bool SceneJson::has_entities() const { return json.contains("entities"); }

Json& SceneJson::entities() { return json["entities"]; }

const Json& SceneJson::entities() const { return json.at("entities"); }

bool SceneJson::has_components(const std::string& component_name) const { return json.contains(component_name); }

Json& SceneJson::components(const std::string& component_name) { return json[component_name]; }

const Json& SceneJson::components(const std::string& component_name) const { return json.at(component_name); }

bool SceneJson::has_entity_entry(const Json& container, const tmt::Entity& entity) const {
    const auto key = tmt::EntityHelper::to_string(entity);
    return container.contains(key);
}

void SceneJson::entity_entry(Json& container, const tmt::Entity& entity, const Json& value) const {
    const auto key = tmt::EntityHelper::to_string(entity);
    container[key] = value;
}

Json& SceneJson::entity_entry(Json& container, const tmt::Entity& entity) const {
    const auto key = tmt::EntityHelper::to_string(entity);
    return container[key];
}
