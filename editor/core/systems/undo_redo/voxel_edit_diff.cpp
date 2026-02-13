#include "voxel_edit_diff.hpp"

#include <engine/engine.hpp>
#include <engine/core/resources/voxel_volume.hpp>
#include <engine/core/components/voxel_renderer.hpp>

namespace tmt {

namespace {

// Get the resource with the UUID that we saved (storing the uuid prevents unnecessary storing of model resources).
ResourceRef<VoxelVolume> get_node_model(const UUID& uuid) {
    const entt::basic_group group = engine.ecs.get_registry().group<VoxelRenderer>(entt::get<Transform>);

    for (const auto&& [entity, renderer, transform] : group.each()) {
        if (renderer.resource->uuid == uuid) return renderer.resource;
    }

    // Should never happen, the change should only be available when a model with that UUID is loaded.
    return {};
}

}  // namespace

void VoxelEditDiff::add_change(const glm::uvec3& coord, const MaterialIndex* before_material, const MaterialIndex* material_after) {
    const bool is_empty_before = (before_material == nullptr);
    before_change.positions.emplace_back(coord.x, coord.y, coord.z, is_empty_before);
    if (!is_empty_before) before_change.indices.push_back(*before_material);

    const bool is_empty_after = (material_after == nullptr);
    after_change.positions.emplace_back(coord.x, coord.y, coord.z, is_empty_after);
    if (!is_empty_after) after_change.indices.push_back(*material_after);
}

void VoxelEditDiff::commit(const std::string& message) {
    // Don't commit the diff if there are no changes.
    if (before_change.positions.empty() && after_change.positions.empty()) return;

    send_to_manager(std::move(*this), message);
}

void VoxelEditDiff::undo() {
    const ResourceRef<VoxelVolume> model = get_node_model(node_uuid);

    size_t next_voxel_index = 0;
    for (auto [x, y, z, is_empty] : before_change.positions) {
        if (is_empty)
            model->blas->remove_voxel(x, y, z);
        else
            model->blas->set_voxel(x, y, z, before_change.indices[next_voxel_index++]);
    }

    model->set_dirty();
}

void VoxelEditDiff::redo() {
    const ResourceRef<VoxelVolume> model = get_node_model(node_uuid);

    size_t next_voxel_index = 0;
    for (auto [x, y, z, is_empty] : after_change.positions) {
        if (is_empty)
            model->blas->remove_voxel(x, y, z);
        else
            model->blas->set_voxel(x, y, z, after_change.indices[next_voxel_index++]);
    }

    model->set_dirty();
}

}  // namespace tmt