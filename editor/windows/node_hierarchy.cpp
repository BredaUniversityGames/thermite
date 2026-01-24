#include "node_hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/file_dialog.hpp"

namespace tmt {

namespace {

std::atomic_bool model_load_atomic {true};

Entity recurse_create_entities(const VoxelSceneNode& node, const ResourceRef<VoxelScene>& scene) {
    const Entity entity = engine.ecs.create_entity(node.name);

    Transform& transform = engine.ecs.get_component<Transform>(entity);
    transform.set_world_matrix(node.transform);

    if (node.tree) engine.ecs.add_component<VoxelRenderer>(entity).resource = engine.resources.copy_resource<VoxelVolume>(scene, node.uuid);

    for (const VoxelSceneNode& child : node.children) {
        const Entity child_entity = recurse_create_entities(child, scene);
        transform.add_child(child_entity);
    }

    return entity;
}

}  // namespace

void NodeHierarchy::menu_bar() const {
    if (!ImGui::BeginMenuBar()) return;

    if (ImGui::MenuItem("Load"))
        open_file_dialog(
            [this](const IO::FileLocation& location) {
                model_load_atomic.store(false);

                const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
                recurse_create_entities(resource->hierarchy, resource);

                model_load_atomic.store(true);
                model_load_atomic.notify_one();
            },
            {{"Voxel file", "svh;vengi"}}
        );

    ImGui::EndMenuBar();
}

void NodeHierarchy::display() {
    model_load_atomic.wait(false);
    menu_bar();

    const entt::basic_view view = engine.ecs.get_registry().view<Name, Transform>();
    for (const auto&& [entity, name, transform] : view.each()) {
        if (transform.get_parent() != entt::null) continue;

        recurse_display_node(entity, name, transform);
    }
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