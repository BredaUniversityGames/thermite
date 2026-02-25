#pragma once

#include "editor/core/window.hpp"

#include <engine/core/components/rig_controller.hpp>
#include <engine/systems/animation/rig_model.hpp>

struct ImVec2;

namespace tmt {

class RigStateController : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_STAT_0 " Rig State Controller"; }

    void before_begin() override {}
    void end_display() override {}
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    void list_animations(const RigModel& model);
    void list_parameters(const Entity entity, RigController& controller);

    void draw_transitions(const RigController& controller);
    void draw_transition(ImVec2 start, ImVec2 middle, int transition_index) const;
    void draw_transition_select(ImVec2 middle, int transition_index, ImVec2 clickable_size);

    void draw_states(Entity entity, RigController& controller);
    void context_popups(Entity entity, RigController& controller);

    void inspect_state(Entity entity, RigController& controller);
    void inspect_transition(Entity entity, RigController& controller) const;

    static void list_conditions(Entity entity, RigController& controller, Transition& transition);

    bool open_state_context = false;
    bool open_transition_context = false;

    int selected_transition = -1;
    std::string selected_state;
    std::string connecting_state;

    std::map<std::string, ImVec2> positions;
};

}  // namespace tmt