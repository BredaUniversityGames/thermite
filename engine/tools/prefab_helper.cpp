#include "prefab_helper.hpp"
#include "engine/core/components/prefab.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/json.hpp"

namespace tmt {

void PrefabHelper::create_prefab(const IO::FileLocation& location, const Entity& root) {
    /* Serialize to disk */
    const auto json = Serializer::serialize(root, engine.ecs);
    std::string dump = json.dump(4);

    IO::write_text_file(location, dump);

    /* Add Prefab components */
    std::set<Entity> entities = {root};
    const auto& root_transform = engine.ecs.get_component<Transform>(root);
    entities.merge(root_transform.get_all_children());

    const PrefabInstanceID instance_id = UUIDGenerator::generate();

    for (const auto& entity : entities) {
        auto& prefab_comp = engine.ecs.add_or_get_component<Prefab>(entity);
        prefab_comp.source_location = location;
        prefab_comp.instance_id = instance_id;
        prefab_comp.root_entity = root;
        prefab_comp.source_entity = entity;
    }
}

Entity PrefabHelper::instantiate_prefab(const IO::FileLocation& location, const Entity& parent) {
    /* Load prefab json from disk */
    const auto prefab_json_resource = engine.resources.load_resource<Json>(location);
    if (prefab_json_resource == nullptr) {
        Log::error(Log::Scope::ENGINE, "[PrefabHelper] instantiate_prefab: Failed to load prefab json at location '{}'", location.get_absolute_path().string());
        return entt::null;
    }

    /* New prefab */
    Prefab prefab_data {};
    prefab_data.source_location = location;
    prefab_data.instance_id = UUIDGenerator::generate();

    /* Deserialize entities */
    std::set<Entity> new_entities;
    Serializer::deserialize(prefab_json_resource->get_parsed_json(), new_entities, engine.ecs, prefab_data);
    if (new_entities.empty()) {
        Log::error(Log::Scope::ENGINE, "[PrefabHelper] instantiate_prefab: No entities deserialized from prefab at location '{}'", location.get_absolute_path().string());
        return entt::null;
    }

    /* Set parent */
    const Entity root_entity = *EntityHelper::upper_parents(new_entities).begin();
    auto& root_transform = engine.ecs.get_component<Transform>(root_entity);
    root_transform.set_parent(parent);
    return parent == entt::null ? root_entity : parent;
}

}  // namespace tmt