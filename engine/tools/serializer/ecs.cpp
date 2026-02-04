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

struct SerializeState {
    SerializeState(const tmt::Ecs& ecs_ref, const std::set<tmt::Entity>& original_entities_ref, const std::set<tmt::Entity>& all_entities_ref)
        : ecs(ecs_ref), original_entities(original_entities_ref), all_entities(all_entities_ref) {}

    const tmt::Ecs& ecs;
    /* Original provided entities */
    const std::set<tmt::Entity>& original_entities;
    /* Originals + children */
    const std::set<tmt::Entity>& all_entities;

    /* per prefab */
    std::set<tmt::Entity> referenced_entities_scope;
    /* Global */
    std::unordered_map<tmt::Entity, std::set<tmt::Entity>> reference_to_referencers;

    bool is_being_referenced(const tmt::Entity& entity) const {
        if (reference_to_referencers.contains(entity) == false) {
            return false;
        }

        const bool is_prefab = ecs.has_component<tmt::Prefab>(entity);
        if (is_prefab) {
            return false;
        }

        const auto& referencers = reference_to_referencers.at(entity);
        for (const auto& referencer : referencers) {
            const bool is_referencer_prefab = ecs.has_component<tmt::Prefab>(referencer);
            if (is_referencer_prefab == false) {
                return true;
            }

            const auto& prefab_comp = ecs.get_component<tmt::Prefab>(referencer);
            if (prefab_comp.instance_id != current_prefab_instance_id) {
                return true;
            }
        }
        return false;
    }

    /* All entities coming from prefabs that have been changed */
    /* Needed for tracking changes in prefab instances */
    std::set<tmt::Entity> changed_prefab_entities;

    std::optional<tmt::PrefabInstanceID> current_prefab_instance_id;

    tmt::SceneJson json;
};

/* Entity with state */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, SerializeState& state) {
    state.referenced_entities_scope.insert(entity);
    return static_cast<std::uint32_t>(entity);
}

static void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const tmt::DeserializeState& state) {
    tmt::Entity deserialized_entity = entt::null;
    tmt::Serializer::deserialize(j, deserialized_entity);
    if (state.current_entity_mapping) {
        /* Check if deserialized entity is in the mapping */
        auto& mapping = state.current_entity_mapping->value();
        auto it = mapping.find(deserialized_entity);
        if (it != mapping.end()) {
            entity = it->second;
        } else {
            entity = deserialized_entity;
        }
    } else {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] No current entity mapping found during deserialization.");
        entity = deserialized_entity;
    }
}

/* ECS */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs) {
    std::set<tmt::Entity> entities;
    for (const auto entity : ecs.get_registry().view<entt::entity>()) {
        entities.insert(entity);
    }
    tmt::json result = tmt::Serializer::serialize(entities, ecs);
    tmt::json& systems = result["systems"];
    const auto size = ecs.systems.size();
    for (const auto& system : ecs.systems) {
        const auto serialized = system->serialize();
        if (serialized.empty() == false) {
            const auto name = system->get_name();
            systems[name] = serialized;
        }
    }

    return result;
}

void tag_invoke(JsonReflect::deserialize_t, const tmt::json& j, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data) {
    std::set<tmt::Entity> new_entities;
    tmt::Serializer::deserialize(j, new_entities, ecs, prefab_data);

    if (j.contains("systems")) {
        const tmt::json& systems = j["systems"];
        for (auto& system : ecs.systems) {
            const auto name = system->get_name();
            if (systems.contains(name)) {
                const tmt::json& system_json = systems[name];
                system->deserialize(system_json);
            }
        }
    }
}

/* Single entity */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs) {
    const std::set<tmt::Entity> entities {entity};
    return tmt::Serializer::serialize(entities, ecs);
}

void tag_invoke(JsonReflect::deserialize_t, const tmt::json& j, tmt::Entity& entity, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data) {
    std::set<tmt::Entity> new_entities;
    tmt::Serializer::deserialize(j, new_entities, ecs, prefab_data);
    if (new_entities.empty()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Deserialized entity JSON resulted in no entities.");
        entity = entt::null;
    } else {
        entity = *tmt::EntityHelper::upper_parents(new_entities).begin();
    }
}

std::optional<tmt::json> recursive_find_component_json(const tmt::SceneJson& prefab_json, const std::string& component_name, const tmt::Entity& entity_id) {
    /*Check if it's a prefab inside this prefab */
    constexpr auto PREFAB_COMPONENT_NAME = tmt::Component<tmt::Prefab>::get_name();
    const std::optional<tmt::json> maybe_value = prefab_json.component_value(PREFAB_COMPONENT_NAME, entity_id);
    if (maybe_value.has_value()) {
        const tmt::json& prefab_component_json = maybe_value.value();

        tmt::Prefab prefab_component {};
        tmt::Serializer::deserialize<tmt::Prefab>(prefab_component_json, prefab_component);
        auto original_prefab_json = tmt::engine.resources.load_resource<tmt::Json>(prefab_component.source_location);

        tmt::SceneJson nested_prefab_json(original_prefab_json->get_parsed_json());
        const std::optional<tmt::json> found = recursive_find_component_json(nested_prefab_json, component_name, prefab_component.source_entity);
        if (found.has_value() == false) {
            return found;
        }

        /* we found something, check if this prefab overrides it */
        const std::optional<tmt::json> override = prefab_json.component_value(component_name, entity_id);
        if (override.has_value() == false) {
            return found;
        }

        /* patch override */
        const tmt::json& override_value = override.value();
        if (override_value.contains("diff")) {
            const tmt::json& diff = override_value["diff"];
            tmt::json base = found.value();
            tmt::json patched = base.patch(diff);
            return patched;
        } else {
            return found;
        }
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

        if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
            const bool changed = state.changed_prefab_entities.contains(entity);
            const bool referenced = state.is_being_referenced(entity);
            const bool is_root = component.root_entity == entity;
            if (!changed && !is_root && !referenced) {
                continue;
            }
        } else {
            state.referenced_entities_scope.clear();
        }

        if (has_prefab) {
            const tmt::Prefab& prefab = tmt::engine.ecs.get_component<tmt::Prefab>(entity);
            auto original_prefab_json = tmt::engine.resources.load_resource<tmt::Json>(prefab.source_location);
            tmt::SceneJson prefab_json(original_prefab_json->get_parsed_json());

            const auto maybe_value = recursive_find_component_json(prefab_json, COMPONENT_NAME, prefab.source_entity);
            if (maybe_value.has_value()) {
                /* diff */
                const tmt::json prefab_component_json = maybe_value.value();

                state.current_prefab_instance_id = prefab.instance_id;
                const tmt::json value = tmt::Serializer::serialize(component, state);
                state.current_prefab_instance_id = std::nullopt;

                const tmt::json diff = nlohmann::json::diff(prefab_component_json, value);
                if (diff.empty() == false) {
                    tmt::json value = tmt::json::object();
                    value["diff"] = diff;

                    tmt::json& components = state.json.components(COMPONENT_NAME);
                    state.json.add_entity_entry(components, entity, value);
                    state.changed_prefab_entities.insert(entity);
                }
                continue;
            }
        }
        tmt::json& components = state.json.components(COMPONENT_NAME);
        const auto value = tmt::Serializer::serialize(component, state);
        state.json.add_entity_entry(components, entity, value);

        for (const auto& referenced_entity : state.referenced_entities_scope) {
            state.reference_to_referencers[referenced_entity].insert(entity);
        }
    }
}

/* [Serialize] Set of entities */
tmt::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities_original, const tmt::Ecs& ecs) {
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
        const bool is_prefab = ecs.has_component<tmt::Prefab>(entity);

        bool should_add = !is_prefab;
        if (is_prefab) {
            const tmt::Prefab& prefab = ecs.get_component<tmt::Prefab>(entity);
            const bool is_root = prefab.root_entity == entity;
            should_add |= is_root;
            should_add |= state.changed_prefab_entities.contains(entity);
            should_add |= state.is_being_referenced(entity);
        }

        if (should_add) {
            state.json.entities().push_back(entity);
        }
    }

    return state.json.get_json();
}

template <typename ComponentType>
void deserialize_component(tmt::DeserializeState& state) {
    constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
    TMT_ZONE_SCOPED_N(COMPONENT_NAME);

    if (state.json.has_components(COMPONENT_NAME) == false) return;

    const tmt::json& entries = state.json.components(COMPONENT_NAME);
    /* Add components */
    for (const auto& [json_key, json_value] : entries.items()) {
        tmt::Entity entity_key = tmt::EntityHelper::from_string(json_key);

        if (entity_key == entt::null) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] %s objects contain an entt::null entry.", COMPONENT_NAME);
            continue;
        }

        // todo: check if component removed in diff

        const tmt::Entity entity = state.current_entity_mapping->value().at(entity_key);
        state.ecs.add_or_get_component<ComponentType>(entity);
    }

    /* Deserialize values */
    for (const auto& [json_key, json_value] : entries.items()) {
        tmt::Entity entity_key = tmt::EntityHelper::from_string(json_key);

        if (entity_key == entt::null) {
            continue;
        }

        const tmt::Entity entity = state.current_entity_mapping->value().at(entity_key);
        ComponentType& component_value = state.ecs.get_component<ComponentType>(entity);

        /* check if diff */
        if (json_value.is_object() && json_value.contains("diff")) {
            const tmt::json diff = json_value.at("diff");
            SerializeState temp_state {state.ecs, {}, {}};
            tmt::json base = tmt::Serializer::serialize(component_value, temp_state);
            tmt::json patched = base.patch(diff);
            tmt::Serializer::deserialize<ComponentType>(patched, component_value, state);
        } else {
            if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
                /* Special handling for Prefab component */
                tmt::Prefab prefab_original;
                tmt::Serializer::deserialize<tmt::Prefab>(json_value, prefab_original, state);
                tmt::Serializer::deserialize<ComponentType>(json_value, component_value, state);
                component_value.source_entity = prefab_original.source_entity;
            } else {
                tmt::Serializer::deserialize<ComponentType>(json_value, component_value, state);
            }
        }
    }
}

/* [Deserialize] */
static void deserialize_scene(std::set<tmt::Entity>& new_entities, tmt::Ecs& ecs, tmt::DeserializeState& state) {
    if (state.json.entities() == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"entities\" object. Json:\n---\n%s\n---", state.json.dump());
        return;
    }
    if (state.json.has_version() == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", state.json.dump());
        return;
    }

    const uint64_t version = state.json.get_version();
    if (version != tmt::Serializer::Config::VERSION) {
        tmt::Log::warn(
            tmt::Log::Scope::ENGINE, "[Serialization] Using outdated version, something might break. current version: %ui, provided version: %ui", version, tmt::Serializer::Config::VERSION
        );
    }

    const auto& json_entities = state.json.entities();
    std::unordered_map<tmt::Entity, tmt::Entity> new_to_old;

    /* Create entities */
    for (const auto& json_entity : json_entities) {
        tmt::Entity deserialized_entity_id = entt::null;
        tmt::Serializer::deserialize(json_entity, deserialized_entity_id);

        if (deserialized_entity_id == entt::null) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] \"Entities\" object contain an entt::null entity.");
            continue;
        }
        if (state.current_entity_mapping->has_value() == false) {
            state.current_entity_mapping->set_value({});
        }
        auto& mapping = state.current_entity_mapping->value();

        if (mapping.contains(deserialized_entity_id)) {
            /* Entity already exists in mapping, skip */
            new_to_old.insert({mapping.at(deserialized_entity_id), deserialized_entity_id});
            continue;
        } else {
            /* Create entity and add to mapping */
            const tmt::Entity created_entity = state.ecs.create_empty_entity(deserialized_entity_id);
            /* Register scene entities */
            mapping.insert({deserialized_entity_id, created_entity});
            new_to_old.insert({created_entity, deserialized_entity_id});
            new_entities.insert(created_entity);
        }
    }

    /* TODO: deserialize prefabs */
    {
        deserialize_component<tmt::Prefab>(state);

        struct PrefabInfo {
            tmt::IO::FileLocation source_location;
            tmt::Entity root_entity;
        };

        std::unordered_map<tmt::PrefabInstanceID, PrefabInfo> prefab_instance_ids;
        for (const auto& [new_entity, old_entity] : new_to_old) {
            constexpr auto PREFAB_COMPONENT_NAME = tmt::Component<tmt::Prefab>::get_name();
            auto prefab_entry = state.json.component_value(PREFAB_COMPONENT_NAME, old_entity);
            if (prefab_entry.has_value() == false) {
                continue;
            }

            /* Special handling for Prefab component */
            tmt::Prefab prefab_comp {};
            tmt::Serializer::deserialize<tmt::Prefab>(prefab_entry.value(), prefab_comp, state);
            tmt::Prefab prefab_comp_original {};
            tmt::Serializer::deserialize<tmt::Prefab>(prefab_entry.value(), prefab_comp_original);
            prefab_comp.source_entity = prefab_comp_original.source_entity;

            const tmt::PrefabInstanceID& instance_id = prefab_comp.instance_id;
            prefab_instance_ids[instance_id] = {
                prefab_comp.source_location,
                prefab_comp.root_entity,
            };

            /* add to new mapping absed on instance id */
            const tmt::Entity& source_entity = prefab_comp.source_entity;
            auto& branch = state.current_entity_mapping->get(instance_id);
            if (branch.has_value() == false) {
                branch.set_value({});
            }
            auto& mapping = branch.value();
            mapping.insert({source_entity, new_entity});
        }

        /* for each prefab instance */
        for (const auto& [prefab_instance_id, prefab_info] : prefab_instance_ids) {
            auto original_prefab_json = tmt::engine.resources.load_resource<tmt::Json>(prefab_info.source_location);

            tmt::DeserializeState prefab_state {state.ecs, tmt::SceneJson(original_prefab_json->get_parsed_json())};

            prefab_state.current_entity_mapping = &state.current_entity_mapping->get(prefab_instance_id);
            prefab_state.current_prefab_instance_id = prefab_instance_id;
            prefab_state.current_root_entity = prefab_info.root_entity;
            prefab_state.current_prefab_location = prefab_info.source_location;

            /* Deserialize prefab */
            std::set<tmt::Entity> prefab_entities;
            deserialize_scene(prefab_entities, state.ecs, prefab_state);

            /* Merge new entities */
            new_entities.insert(prefab_entities.begin(), prefab_entities.end());
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

    if (state.current_prefab_instance_id != tmt::NULL_UUID) {
        for (const auto& [new_entity, old_entity] : new_to_old) {
            /* Add new Prefab component since those are not stored in the prefab json */
            tmt::Prefab& prefab_comp = state.ecs.add_or_get_component<tmt::Prefab>(new_entity);
            prefab_comp.instance_id = state.current_prefab_instance_id;
            if (state.current_root_entity != entt::null) {
                prefab_comp.root_entity = state.current_root_entity;
            } else {
                const auto roots = tmt::EntityHelper::upper_parents(new_entities);
                if (roots.size() != 1) {
                    tmt::Log::warn(
                        tmt::Log::Scope::ENGINE, "[Serialization] Prefab instance has multiple root entities after deserialization. PrefabInstanceID: {}, Roots count: {}",
                        state.current_prefab_instance_id, roots.size()
                    );
                }
                prefab_comp.root_entity = *roots.begin();
            }
            prefab_comp.source_entity = old_entity;
            prefab_comp.source_location = state.current_prefab_location;
        }
    }
}

void tag_invoke(JsonReflect::deserialize_t, const tmt::json& j, std::set<tmt::Entity>& new_entities, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data) {
    tmt::DeserializeState state {ecs, tmt::SceneJson(j)};
    state.current_entity_mapping = &state.entity_mapping[tmt::NULL_UUID];
    if (prefab_data.has_value()) {
        state.current_prefab_instance_id = prefab_data->instance_id;
        state.current_root_entity = prefab_data->root_entity;
        state.current_prefab_location = prefab_data->source_location;
    }
    deserialize_scene(new_entities, ecs, state);
    state.entity_mapping.print();
}

std::string tmt::SceneJson::dump() const { return json_value.dump(4); }

bool tmt::SceneJson::has_version() const { return json_value.contains("version"); }

uint64_t tmt::SceneJson::get_version() const {
    if (json_value.contains("version")) {
        return json_value.at("version").get<uint64_t>();
    }
    tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", json_value.dump());
    return 0;
}

void tmt::SceneJson::set_version(uint64_t version) { json_value["version"] = version; }

bool tmt::SceneJson::has_entities() const { return json_value.contains("entities"); }

tmt::json& tmt::SceneJson::entities() { return json_value["entities"]; }

const tmt::json& tmt::SceneJson::entities() const { return json_value.at("entities"); }

bool tmt::SceneJson::has_components(const std::string& component_name) const { return json_value.contains(component_name); }

tmt::json& tmt::SceneJson::components(const std::string& component_name) { return json_value[component_name]; }

const tmt::json& tmt::SceneJson::components(const std::string& component_name) const { return json_value.at(component_name); }

bool tmt::SceneJson::has_component_entry(const std::string& component_name, const tmt::Entity entity_id) const {
    if (has_components(component_name) == false) return false;

    const json& component_container = components(component_name);

    return has_entity_entry(component_container, entity_id);
}

std::optional<tmt::json> tmt::SceneJson::component_value(const std::string& component_name, const tmt::Entity entity_id) const {
    if (has_components(component_name) == false) return std::nullopt;

    const json& component_container = components(component_name);
    if (has_entity_entry(component_container, entity_id) == false) return std::nullopt;

    return get_entity_entry(component_container, entity_id);
}

bool tmt::SceneJson::has_entity_entry(const json& container, const tmt::Entity entity_id) const {
    const std::string key = tmt::EntityHelper::to_string(entity_id);
    return container.contains(key);
}

void tmt::SceneJson::add_entity_entry(tmt::json& container, const tmt::Entity entity_id, const tmt::json& value) const {
    const std::string key = tmt::EntityHelper::to_string(entity_id);
    container[key] = value;
}

const tmt::json& tmt::SceneJson::get_entity_entry(const tmt::json& container, const tmt::Entity entity_id) const {
    const std::string key = tmt::EntityHelper::to_string(entity_id);
    return container.at(key);
}

/* Component specific deserialize functions */
// void tag_invoke(JsonReflect::deserialize_t, const Json& j, tmt::Entity& component, const tmt::DeserializeState& state) {
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