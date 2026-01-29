#include "prefab_helper.hpp"
#include "engine/core/components/prefab.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/ecs.hpp"
#include <regex>

namespace tmt {

void PrefabHelper::create_prefab(const IO::FileLocation& location, const Entity& root) {
    /* Serialize to disk */
    const auto json = Serializer::serialize(root, engine.ecs);
    std::string dump = json.dump(4);

    const auto& root_transform = engine.ecs.get_component<Transform>(root);

    std::unordered_map<Entity, EntityID> entity_to_id_map;

    /* replace all uuids with new unique ones */
    auto all_entities = root_transform.get_all_children();
    all_entities.insert(root);
    for (const auto& entity : all_entities) {
        const auto& entity_id = engine.ecs.get_component<EntityID>(entity);
        const std::string old_uuid_str = entity_id.str();
        const UUID new_uuid = UUIDGenerator::generate();
        const std::string new_uuid_str = new_uuid.str();
        dump = std::regex_replace(dump, std::regex(old_uuid_str), new_uuid_str);
        entity_to_id_map[entity] = EntityID(new_uuid);
    }

    IO::write_text_file(location, dump);

    /* Add Prefab components */
    std::set<Entity> entities = {root};
    entities.merge(root_transform.get_all_children());

    const PrefabInstanceID instance_id = UUIDGenerator::generate();

    for (const auto& entity : entities) {
        auto& prefab_comp = engine.ecs.add_or_get_component<Prefab>(entity);
        prefab_comp.source_location = location;
        prefab_comp.instance_id = instance_id;
        prefab_comp.root_entity = engine.ecs.get_component<EntityID>(root);
        prefab_comp.source_entity = entity_to_id_map.at(entity);
    }
}

}  // namespace tmt