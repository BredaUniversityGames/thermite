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

}  // namespace tmt