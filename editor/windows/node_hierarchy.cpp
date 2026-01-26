#include "node_hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/file_dialog.hpp"
#include "engine/tools/svh_format.hpp"

#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

namespace tmt {

namespace {

struct VoxelEditUUID {
    UUID uuid {NULL_UUID};
};

std::atomic_bool model_load_atomic {true};

}  // namespace

void NodeHierarchy::new_svh() {
    engine.ecs.clear();
    loaded_location = {};
}

void NodeHierarchy::open_svh() {
    open_file_dialog(
        [this](const IO::FileLocation& location) {
            model_load_atomic.store(false);

            loaded_location = location;
            engine.ecs.clear();

            const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
            recurse_build_scene(resource->hierarchy);

            model_load_atomic.store(true);
            model_load_atomic.notify_one();
        },
        {{"Thermite voxel file", "svh"}}
    );
}

void NodeHierarchy::save_svh() {
    if (loaded_location.relative_path.empty()) {
        save_svh_as();
        return;
    }

    const std::vector<char> scene_data = encode_voxel_scene();
    IO::write_file(loaded_location, scene_data.data(), scene_data.size());
}

void NodeHierarchy::save_svh_as() {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            loaded_location = location;

            const std::vector<char> scene_data = encode_voxel_scene();
            IO::write_file(location, scene_data.data(), scene_data.size());
        },
        {{"Thermite voxel file", "svh"}}
    );
}

void NodeHierarchy::import_file() {
    open_file_dialog(
        [this](const IO::FileLocation& location) {
            model_load_atomic.store(false);

            loaded_location = {};
            engine.ecs.clear();

            const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
            recurse_build_scene(resource->hierarchy);

            model_load_atomic.store(true);
            model_load_atomic.notify_one();
        },
        {{"Vengi voxel file", "vengi"}}
    );
}

void NodeHierarchy::display() {
    model_load_atomic.wait(false);

    const entt::basic_view view = engine.ecs.get_registry().view<Name, Transform>();
    for (const auto&& [entity, name, transform] : view.each()) {
        if (transform.get_parent() != entt::null) continue;

        recurse_display_node(entity, name, transform);
    }
}

Entity NodeHierarchy::recurse_build_scene(const VoxelSceneNode& node) {
    const Entity entity = engine.ecs.create_entity(node.name);

    Transform& transform = engine.ecs.get_component<Transform>(entity);
    transform.set_world_matrix(node.transform);

    VoxelEditUUID& uuid_component = engine.ecs.add_component<VoxelEditUUID>(entity);
    uuid_component.uuid = node.uuid;

    if (node.tree) {
        VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(entity);

        // Create a fake voxel resource managed by the voxel editor.
        const ResourceRef volume {{}, std::make_shared<VoxelVolume>(node)};
        renderer.resource = volume;
    }

    for (const VoxelSceneNode& child : node.children) {
        const Entity child_entity = recurse_build_scene(child);
        transform.add_child(child_entity);
    }

    return entity;
}

void NodeHierarchy::recurse_parse_scene(Entity entity, const Transform& transform, VoxelSceneNode& node) {
    node.name = engine.ecs.get_component<Name>(entity).name;

    const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.get_local_position());
    const glm::mat4 rotation = glm::toMat4(transform.get_local_rotation());
    const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.get_local_scale());
    node.transform = translation * rotation * scale;

    node.uuid = engine.ecs.get_component<VoxelEditUUID>(entity).uuid;

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

std::vector<char> NodeHierarchy::encode_voxel_scene() {
    const auto& entities = engine.ecs.get_registry().storage<Transform>();
    if (entities.empty()) return {};  // Return empty scene encoding.

    VoxelSceneNode node;
    for (const auto&& [entity, transform] : entities.each()) {
        if (transform.has_parent()) continue;

        recurse_parse_scene(entity, transform, node);
    }

    return encode_svh(node);
}

void NodeHierarchy::recurse_display_node(const Entity entity, const Name& name, const Transform& transform) {
    ImGui::PushID(static_cast<int>(entity));

    const std::set<Entity>& children = transform.get_children();

    constexpr ImGuiTreeNodeFlags default_flags =
        ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    flags |= (entity == selected_entity ? ImGuiTreeNodeFlags_Selected : 0);

    const bool tree_open = ImGui::TreeNodeEx(name.name.c_str(), flags);
    if (ImGui::IsItemClicked()) selected_entity = entity;
    if (tree_open) {
        for (const Entity child : children) {
            const auto&& [child_name, child_transform] = engine.ecs.get_component<Name, Transform>(child);
            recurse_display_node(child, child_name, child_transform);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

}  // namespace tmt