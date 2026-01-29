#pragma once

#include "editor/core/window.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/entity.hpp"

namespace tmt {

struct VoxelSceneNode;
struct Name;
struct Transform;

class VoxelVolume;

class NodeHierarchy : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_FLOWCHART " Node hierarchy"; }
    constexpr bool default_open() const override { return true; }

    [[nodiscard]] Entity get_selected_entity() const { return selected_entity; }
    void set_selected_entity(const Entity entity) { selected_entity = entity; }

    void new_svh();
    void open_svh();
    void save_svh();
    void save_svh_as();
    void import_file(const std::string& file_description, const std::string& file_extension) const;
    void export_file(const std::string& file_description, const std::string& file_extension) const;

    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

    static Entity recurse_build_scene(const VoxelSceneNode& node, bool assign_new_uuids = false, const Entity parent_entity = entt::null, const glm::mat4& parent_matrix = {1.0f});
    static void recurse_parse_scene(Entity entity, const Transform& transform, VoxelSceneNode& node);

    static std::vector<char> encode_voxel_scene();

   private:
    void recurse_display_node(Entity entity, const Name& name, Transform& transform);

    IO::FileLocation loaded_location {};
    Entity selected_entity {entt::null};
};

}  // namespace tmt