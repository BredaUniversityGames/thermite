#include "edit.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/resources/voxel_scene.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/tools/svh_format.hpp"

#include "glm/gtx/quaternion.hpp"

namespace tmt {

void EditMode::on_switch_to() {
    engine.scenes.load_scene<VoxelEditScene>();

    VoxelSceneNode node;
    if (!edit_data.empty() && decode_svh(edit_data, node)) recurse_build_scene(node);

    edit_data.clear();
}

void EditMode::on_switch_away() {
    const auto& entities = engine.ecs.get_registry().storage<Transform>();
    if (entities.empty()) return;

    VoxelSceneNode node;
    for (const auto&& [entity, transform] : entities.each()) {
        if (transform.has_parent()) continue;

        recurse_parse_scene(entity, transform, node);
    }

    edit_data = encode_svh(node);
}

Entity EditMode::recurse_build_scene(VoxelSceneNode& node) {
    const Entity entity = engine.ecs.create_entity(node.name);

    Transform& transform = engine.ecs.get_component<Transform>(entity);
    transform.set_world_matrix(node.transform);

    if (node.tree) {
        const ResourceRef volume {{}, std::make_shared<VoxelVolume>(std::move(node))};
        engine.ecs.add_component<VoxelRenderer>(entity).resource = volume;
    }

    for (VoxelSceneNode& child : node.children) {
        const Entity child_entity = recurse_build_scene(child);
        transform.add_child(child_entity);
    }

    return entity;
}

void EditMode::recurse_parse_scene(const Entity entity, const Transform& transform, VoxelSceneNode& node) {
    node.name = engine.ecs.get_component<Name>(entity).name;

    const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.get_local_position());
    const glm::mat4 rotation = glm::toMat4(transform.get_local_rotation());
    const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.get_local_scale());
    node.transform = translation * rotation * scale;

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(entity);
    if (renderer != nullptr) {
        node.uuid = renderer->resource->uuid;
        node.size = renderer->resource->size;
        node.tree = std::make_unique<Svt64>(*renderer->resource->blas);
    }

    for (const Entity child : transform.get_children()) {
        const Transform& child_transform = engine.ecs.get_component<Transform>(child);
        recurse_parse_scene(child, child_transform, node.children.emplace_back());
    }
}

}  // namespace tmt