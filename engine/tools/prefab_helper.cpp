#include "prefab_helper.hpp"
#include "engine/core/components/prefab.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/ecs.hpp"

namespace tmt {

void PrefabHelper::create_prefab(const IO::FileLocation& location, const Entity& root) {
    /* Serialize to disk */
    auto json = Serializer::serialize(root, engine.ecs);
    IO::write_text_file(location, json.dump(4));

    /* Add Prefab components */
    const auto& root_transform = engine.ecs.get_component<Transform>(root);

    std::set<Entity> entities = {root};
    entities.merge(root_transform.get_all_children());

    const PrefabInstanceID instance_id = UUIDGenerator::generate();

    for (const auto& entity : entities) {
        auto& prefab_comp = engine.ecs.add_or_get_component<Prefab>(entity);
        prefab_comp.source_location = location;
        prefab_comp.source_entity = engine.ecs.get_component<EntityID>(entity);
        prefab_comp.root_entity = engine.ecs.get_component<EntityID>(root);
        prefab_comp.instance_id = instance_id;
    }
}

}  // namespace tmt