#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/physics/physics_layers.hpp"

namespace tmt {

class PhysicsLayersEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return "Physics Layers"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void display() override;

   private:
    void draw_layer_list(PhysicsLayers& layers);
    void draw_collision_matrix(PhysicsLayers& layers);
    void set_layer_name(uint32_t layer, const std::string& name);
};

}  // namespace tmt
