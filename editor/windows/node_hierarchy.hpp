#pragma once

#include "editor/core/window.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/entity.hpp"

#include <imgui.h>

namespace tmt {
struct Name;
struct Transform;

class VoxelVolume;

class NodeHierarchy : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_FLOWCHART " Node hierarchy"; }
    constexpr int get_window_flags() const override { return ImGuiWindowFlags_MenuBar; }
    constexpr bool default_open() const override { return true; }

    [[nodiscard]] Entity get_selected_entity() const { return selected_entity; }
    void set_selected_entity(const Entity entity) { selected_entity = entity; }

    void menu_bar() const;
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    void recurse_display_node(Entity entity, const Name& name, const Transform& transform);

    Entity selected_entity {entt::null};
};

}  // namespace tmt