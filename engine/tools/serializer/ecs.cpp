#include "ecs.hpp"
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
#include "engine/tools/tree_map.hpp"

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

    /* Copy */
    std::optional<Json> component_value(const std::string& component_name, const tmt::EntityID& entity_id) const;

    /* entity entry */
    bool has_entity_entry(const Json& container, const tmt::EntityID& entity_id) const;
    void add_entity_entry(Json& container, const tmt::EntityID& entity_id, const Json& value) const;
    const Json& get_entity_entry(const Json& container, const tmt::EntityID& entity_id) const;
};

struct SerializeState {
    SerializeState(const tmt::Ecs& ecs_ref, const std::set<tmt::Entity>& original_entities_ref, const std::set<tmt::Entity>& all_entities_ref)
        : ecs(ecs_ref), original_entities(original_entities_ref), all_entities(all_entities_ref) {}

    const tmt::Ecs& ecs;
    /* Original provided entities */
    const std::set<tmt::Entity>& original_entities;
    /* Originals + children */
    const std::set<tmt::Entity>& all_entities;

    /* All entities coming from prefabs that have been changed */
    /* Needed for tracking changes in prefab instances */
    std::set<tmt::Entity> changed_prefab_entities;
    // TODO: add prefab entities that are referenced

    std::optional<tmt::PrefabInstanceID> current_prefab_instance_id;

    SceneJson json;
};

struct DeserializeState {
    tmt::Ecs& ecs;
    const SceneJson& json;

    // std::unordered_map<tmt::EntityID, tmt::Entity> entity_mapping {{tmt::NULL_UUID, entt::null}}; /* Default null to null value */

    tmt::TreeMap<tmt::UUID, tmt::Entity> entity_mapping {tmt::NULL_UUID, entt::null}; /* Default null to null value */
};

/* Entity with state */
Json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const SerializeState& state) {
    if (entity == entt::null) {
        return tmt::NULL_ENTITY_ID.str();
    }
    const auto& entity_id = state.ecs.get_component<tmt::EntityID>(entity);

    // TODO: maybe not needed if entity mapping works
    if (state.current_prefab_instance_id.has_value()) {
        if (state.ecs.has_component<tmt::Prefab>(entity)) {
            const auto& prefab = state.ecs.get_component<tmt::Prefab>(entity);
            if (prefab.instance_id == state.current_prefab_instance_id.value()) {
                return prefab.source_entity.str();
            }
        }
    }

    return entity_id.str();
}

static void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const DeserializeState& state) {
    const std::string uuid_str = j.get<std::string>();
    const tmt::UUID uuid = tmt::UUID::fromStrFactory(uuid_str);

    if (state.entity_mapping.contains(uuid)) {
        entity = state.entity_mapping.at(uuid).value();
    } else {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Entity mapping does not contain UUID {}", uuid_str);
        entity = entt::null;
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

std::optional<Json> recursive_find_component_json(const SceneJson& prefab_json, const std::string& component_name, const tmt::EntityID& entity_id) {
    /*Check if it's a prefab inside this prefab */
    constexpr auto PREFAB_COMPONENT_NAME = tmt::Component<tmt::Prefab>::get_name();
    const std::optional<Json> maybe_value = prefab_json.component_value(PREFAB_COMPONENT_NAME, entity_id);
    if (maybe_value.has_value()) {
        const Json& prefab_component_json = maybe_value.value();

        tmt::Prefab prefab_component {};
        tmt::Serializer::deserialize<tmt::Prefab>(prefab_component_json, prefab_component);
        auto original_prefab_json = tmt::engine.resources.load_resource<tmt::Json>(prefab_component.source_location);

        SceneJson nested_prefab_json(original_prefab_json->get_parsed_json());
        return recursive_find_component_json(nested_prefab_json, component_name, prefab_component.source_entity);
    }
    return prefab_json.component_value(component_name, entity_id);
}

template <typename ComponentType>
void serialize_component(SerializeState& state) {
    /* If no component instance exists, continue */
    if (state.ecs.get_registry().view<ComponentType>().size() <= 0) return;

    constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
    TMT_ZONE_SCOPED_N(COMPONENT_NAME);

    for (const auto entity : state.all_entities) {
        if (state.ecs.has_component<ComponentType>(entity) == false) continue;
        const auto& component = state.ecs.get_component<ComponentType>(entity);
        const bool has_prefab = state.ecs.has_component<tmt::Prefab>(entity);
        const auto& entity_id = state.ecs.get_component<tmt::EntityID>(entity);

        if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
            const bool changed = state.changed_prefab_entities.contains(entity);
            const bool is_root = component.root_entity == entity_id;
            if (!changed && !is_root) {
                continue;
            }
        }

        if (has_prefab) {
            const tmt::Prefab& prefab = tmt::engine.ecs.get_component<tmt::Prefab>(entity);
            auto original_prefab_json = tmt::engine.resources.load_resource<tmt::Json>(prefab.source_location);
            SceneJson prefab_json(original_prefab_json->get_parsed_json());

            const auto maybe_value = recursive_find_component_json(prefab_json, COMPONENT_NAME, prefab.source_entity);
            if (maybe_value.has_value()) {
                /* diff */
                const Json prefab_component_json = maybe_value.value();

                state.current_prefab_instance_id = prefab.instance_id;
                const Json value = tmt::Serializer::serialize(component, state);
                state.current_prefab_instance_id = std::nullopt;

                const Json diff = nlohmann::json::diff(prefab_component_json, value);
                if (diff.empty() == false) {
                    Json value = Json::object();
                    value["diff"] = diff;

                    Json& components = state.json.components(COMPONENT_NAME);
                    state.json.add_entity_entry(components, entity_id, value);
                    state.changed_prefab_entities.insert(entity);
                }
                continue;
            }
        }
        Json& components = state.json.components(COMPONENT_NAME);
        const auto value = tmt::Serializer::serialize(component, state);
        state.json.add_entity_entry(components, entity_id, value);
    }
}

/* [Serialize] Set of entities */
Json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities_original, const tmt::Ecs& ecs) {
    TMT_ZONE_SCOPED_NS("Ecs::serialize");

    /* Add children */
    auto entities = entities_original;
    for (const auto entity : entities_original) {
        entities.merge(ecs.get_component<tmt::Transform>(entity).get_all_children());
    }

    SerializeState state {ecs, entities_original, entities};

    state.json.set_version(tmt::Serializer::Config::VERSION);

    /* Add entities object at the top*/
    state.json.entities();

    /* Serialize each component */
    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;
        if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
            return;
        }
        serialize_component<ComponentType>(state);
    });
    /* Serialize Prefab component last */
    serialize_component<tmt::Prefab>(state);

    /* Add entities */
    for (const auto entity : state.all_entities) {
        const auto& entity_id = ecs.get_component<tmt::EntityID>(entity);
        const std::string entity_id_str = entity_id.str();

        const bool is_prefab = ecs.has_component<tmt::Prefab>(entity);

        bool should_add = !is_prefab;
        if (is_prefab) {
            const tmt::Prefab& prefab = ecs.get_component<tmt::Prefab>(entity);
            const bool is_root = prefab.root_entity == entity_id;
            should_add |= is_root;
            should_add |= state.changed_prefab_entities.contains(entity);
        }

        if (should_add) {
            state.json.entities().push_back(entity_id_str);
        }
    }

    return state.json.get_json();
}

template <typename ComponentType>
void deserialize_component(DeserializeState& state) {
    constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
    TMT_ZONE_SCOPED_N(COMPONENT_NAME);

    if (state.json.has_components(COMPONENT_NAME) == false) return;

    const Json& entries = state.json.components(COMPONENT_NAME);
    /* Add components */
    for (const auto& [json_key, json_value] : entries.items()) {
        const auto json_entity_id = tmt::EntityID(json_key);

        if (json_entity_id == tmt::NULL_ENTITY_ID) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] %s objects contain an entt::null entry.", COMPONENT_NAME);
            continue;
        }

        // todo: check if component removed in diff

        const tmt::Entity entity = state.entity_mapping.at(json_entity_id).value();
        state.ecs.add_or_get_component<ComponentType>(entity);
    }

    /* Deserialize values */
    for (const auto& [json_key, json_value] : entries.items()) {
        const auto json_entity_id = tmt::EntityID(json_key);

        if (json_entity_id == tmt::NULL_ENTITY_ID) {
            continue;
        }

        const tmt::Entity entity = state.entity_mapping.at(json_entity_id).value();
        ComponentType& component_value = state.ecs.get_component<ComponentType>(entity);

        // const auto maybe_value = recursive_find_component_json(prefab_json, COMPONENT_NAME, prefab.source_entity);
        /* check if diff */
        if (json_value.is_object() && json_value.contains("diff")) {
            const Json diff = json_value.at("diff");
            SerializeState temp_state {state.ecs, {}, {}};
            Json base = tmt::Serializer::serialize(component_value, temp_state);
            Json patched = base.patch(diff);
            tmt::Serializer::deserialize<ComponentType>(patched, component_value, state);
        } else {
            tmt::Serializer::deserialize<ComponentType>(json_value, component_value, state);
        }
    }
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
    for (const auto& json_uuid : json_entities) {
        tmt::EntityID deserialized_entity_id = tmt::NULL_ENTITY_ID;
        tmt::Serializer::deserialize(json_uuid, deserialized_entity_id);

        if (deserialized_entity_id == tmt::NULL_ENTITY_ID) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] \"Entities\" object contain an entt::null entity.");
            continue;
        }

        /* Create entity and add to mapping */
        const tmt::Entity created_entity = state.ecs.create_empty_entity();
        state.entity_mapping[deserialized_entity_id] = created_entity;

        if (state.ecs.has_uuid(deserialized_entity_id)) {
            tmt::Log::warn(
                tmt::Log::Scope::ENGINE, "[Serialization] UUID collision detected! Entity {} already owns UUID {}. Generating a new UUID for entity {}.",
                *state.ecs.get_entity_by_uuid(deserialized_entity_id), deserialized_entity_id, created_entity
            );

            /* Generate a new UUID for the created entity */
            tmt::EntityID& id_component = state.ecs.add_or_get_component<tmt::EntityID>(created_entity);
            id_component = tmt::UUIDGenerator::generate();
            state.ecs.register_uuid(id_component, created_entity);
        } else {
            /* Set EntityID component */
            tmt::EntityID& id_component = state.ecs.add_or_get_component<tmt::EntityID>(created_entity);
            id_component = deserialized_entity_id;
            state.ecs.register_uuid(id_component, created_entity);
        }
    }

    /* Deserialize each component */
    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;
        if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
            return;
        }
        deserialize_component<ComponentType>(state);
    });

    /* Return newly created entities */
    for (const auto& [_, mapped_entity] : state.entity_mapping) {
        if (mapped_entity.value() != entt::null) {
            // TODO: refactor this to return all entities of tree
            new_entities.insert(mapped_entity.value());
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

std::optional<Json> SceneJson::component_value(const std::string& component_name, const tmt::EntityID& entity_id) const {
    if (has_components(component_name) == false) return std::nullopt;

    const Json& component_container = components(component_name);
    if (has_entity_entry(component_container, entity_id) == false) return std::nullopt;

    return get_entity_entry(component_container, entity_id);
}

bool SceneJson::has_entity_entry(const Json& container, const tmt::EntityID& entity_id) const {
    const auto key = entity_id.str();
    return container.contains(key);
}

void SceneJson::add_entity_entry(Json& container, const tmt::EntityID& entity_id, const Json& value) const {
    const auto key = entity_id.str();
    container[key] = value;
}

const Json& SceneJson::get_entity_entry(const Json& container, const tmt::EntityID& entity_id) const {
    const auto key = entity_id.str();
    return container.at(key);
}

/* Component specific deserialize functions */
// void tag_invoke(JsonReflect::deserialize_t, const Json& j, tmt::EntityID& component, const DeserializeState& state) {
//     component = tmt::UUID::fromStrFactory(j.get<std::string>());
//     if (state.ecs.has_uuid(component) == false) return;
//
//     /* uuid already exists as a component */
//     const tmt::Entity owner = *state.ecs.get_entity_by_uuid(component);
//     const tmt::Entity current_entity = state.ecs.get_entity(component);
//
//     if (owner == current_entity) return;
//
//     /* UUID collision detected */
//     tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] UUID collision detected! Entity {} already owns UUID {}. Generating a new UUID for entity {}.", owner, component,
//     current_entity);
//
//     /* Generate a new UUID for the current entity */
//     component = tmt::UUIDGenerator::generate();
//     state.ecs.register_uuid(component, current_entity);
// }