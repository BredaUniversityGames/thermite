#include "edit.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/resources/voxel_scene.hpp"
#include "engine/tools/svh_format.hpp"

#include "editor/windows/node_hierarchy.hpp"

namespace tmt {

void EditMode::on_switch_to() {
    engine.scenes.load_scene<VoxelEditScene>();

    VoxelSceneNode node;
    if (!edit_data.empty() && decode_svh(edit_data, node)) NodeHierarchy::recurse_build_scene(node);

    edit_data.clear();
}

void EditMode::on_switch_away() {
    const auto& entities = engine.ecs.get_registry().storage<Transform>();
    if (entities.empty()) return;

    VoxelSceneNode node;
    for (const auto&& [entity, transform] : entities.each()) {
        if (transform.has_parent()) continue;

        NodeHierarchy::recurse_parse_scene(entity, transform, node);
    }

    edit_data = encode_svh(node);
}

}  // namespace tmt