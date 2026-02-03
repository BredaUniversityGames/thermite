#pragma once
#include <set>
#include <JsonReflect.hpp>
#include "engine/core/entity.hpp"
#include "engine/tools/json.hpp"
#include "engine/tools/tree_map.hpp"
#include "engine/core/components/prefab.hpp" /* for PrefabInstanceID */

namespace tmt {
class Ecs;

class SceneJson {
   private:
    json json_value;

   public:
    SceneJson() = default;
    SceneJson(const json& j) : json_value(j) {}

    const json& get_json() const { return json_value; }
    std::string dump() const;

    /* version */
    bool has_version() const;
    uint64_t get_version() const;
    void set_version(uint64_t version);

    /* entities */
    bool has_entities() const;
    json& entities();
    const json& entities() const;

    /* components */
    bool has_components(const std::string& component_name) const;
    json& components(const std::string& component_name);
    const json& components(const std::string& component_name) const;

    bool has_component_entry(const std::string& component_name, const tmt::Entity entity_id) const;
    /* Copy */
    std::optional<json> component_value(const std::string& component_name, const tmt::Entity entity_id) const;
    /* entity entry */
    bool has_entity_entry(const json& container, const tmt::Entity entity_id) const;
    void add_entity_entry(json& container, const tmt::Entity entity_id, const json& value) const;
    const json& get_entity_entry(const json& container, const tmt::Entity entity_id) const;
};

struct DeserializeState {
    tmt::Ecs& ecs;
    const tmt::SceneJson& json;

    tmt::PrefabInstanceID current_prefab_instance_id {tmt::NULL_UUID};
    tmt::Entity current_root_entity = entt::null;
    tmt::IO::FileLocation current_prefab_location;
    
    /* Prefab sequence to entity mapping */
    using EntityMap = tmt::TreeMap<tmt::PrefabInstanceID, std::unordered_map<tmt::Entity, tmt::Entity>>;
    EntityMap entity_mapping {tmt::NULL_UUID, {{entt::null, entt::null}}}; /* Default null to null value */
    EntityMap* current_entity_mapping = nullptr;
};

}  // namespace tmt

/* ECS */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Ecs& ecs);

/* Single entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, tmt::Ecs& ecs);

/* Set of entity */
JsonReflect::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities, const tmt::Ecs& ecs);
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, std::set<tmt::Entity>& entities, tmt::Ecs& ecs);