#pragma once
#include <variant>
#include <cstdint>
#include "engine/core/ecs.hpp"
// #include "editor/inspectable.hpp"
// #include "systems/events.hpp"
// #include "tools/serialization.hpp"
#include "rig_model.hpp"

// #ifdef KUDZU_INSPECTOR
// #include "imgui/imgui.h"
// #endif

namespace tmt {

struct ImNodesEditorContext;

struct Trigger {
    Trigger() = default;
    Trigger(bool value_) : value(value_) {};

    // operators needed due to comparing std::variant<..., Trigger>
    [[nodiscard]] bool operator==(const Trigger&) const { return value; }
    [[nodiscard]] bool operator!=(const Trigger&) const { return value; }
    [[nodiscard]] bool operator>(const Trigger&) const { return value; }
    [[nodiscard]] bool operator<(const Trigger&) const { return value; }

    bool value = false;
};

struct AnimationState {
    std::string animation;

    float animation_speed = 1.0f;
    bool repeat = false;
    bool transition_on_finish = false;  // Only transition after finishing the animation.
};

using Variant = std::variant<float, int, bool, Trigger>;

struct Condition {
    enum class CompareType : uint8_t { EQUAL = 0, NOT_EQUAL = 1, GREATER = 2, LESS = 3 };
    using CompareFunctionMap = std::map<CompareType, std::function<bool(const Variant&, const Variant&)>>;

    static const CompareFunctionMap COMPARE_MAP;

    [[nodiscard]] bool is_satisfied(const Variant& parameter) const {
        const auto& compare = COMPARE_MAP.at(compare_type);
        const bool satisfied = compare(parameter, compare_value);
        return satisfied;
    };

    std::string parameter_name;

    Variant compare_value;
    CompareType compare_type = CompareType::EQUAL;
};

struct Transition {
    std::string from;
    std::string to;

    float transition_time = 0.0f;
    float animation_time = 0.0f;
    std::vector<Condition> conditions;
};

class RigController {
   public:
    friend class JsonSerializer;
    friend class JsonDeserializer;
    friend class RigModelManager;
    friend class RigControllerComponentInspector;

    RigController();
    static void on_destroy(entt::registry& registry, Entity entity);

    void set_parameter_float(const std::string& name, float value);
    void set_parameter_int(const std::string& name, int value);
    void set_parameter_bool(const std::string& name, bool value);
    void set_parameter_trigger(const std::string& name, bool value = true);

    [[nodiscard]] float get_parameter_float(const std::string& name);
    [[nodiscard]] int get_parameter_int(const std::string& name);
    [[nodiscard]] bool get_parameter_bool(const std::string& name);
    [[nodiscard]] bool get_parameter_trigger(const std::string& name);

    std::map<std::string, Variant> parameters;
    std::map<std::string, AnimationState> states = {{"Start", AnimationState {}}};
    std::vector<Transition> transitions;

    ImNodesEditorContext* editor_context = nullptr;

   private:
    std::string current_state = "Start";

    void remove_state(const std::string& name);
    void remove_parameter(const std::string& name);

    void rename_state(const std::string& old_name, const std::string& new_name);
    void rename_parameter(const std::string& old_name, const std::string& new_name);

    bool check_transition_conditions(const Transition& transition, std::vector<Trigger*>& satisfied_triggers);
};

class RigModelManager : public ISystem /*, public IPanel, public IKeyboardListener*/ {
   public:
    // RigModelManager();
    //~RigModelManager() {};

    std::string get_name() override;

    void on_start() override;
    void on_update(const FrameData& time) override;
    void inspect(float);
    void on_end() override;
    // void on_key_event(KeyboardEvent& event) override;

    // void list_animations(const RigModel& model);
    // void list_parameters(RigController& controller);

    /*void draw_transitions(const RigController& controller);
    void draw_transition(ImVec2 start, ImVec2 middle, int transition_index) const;
    void draw_transition_select(ImVec2 middle, int transition_index, ImVec2 clickable_size = ImVec2 {25.0f, 25.0f});

    void draw_states(RigController& controller);
    void context_popups(RigController& controller);

    void inspect_state(RigController& controller);
    void inspect_transition(RigController& controller);

    static void list_conditions(RigController& controller, Transition& transition);

    bool open_state_context = false;
    bool open_transition_context = false;

    int selected_transition = -1;
    std::string selected_state;
    std::string connecting_state;

    std::map<std::string, ImVec2> positions;

    void OnPanel() override;
    std::string GetName() const override { return "Rig state controller"; }
    std::string GetIcon() const override { return ICON_MD_STATE_MACHINE; }*/
};
}  // namespace tmt

// VISITABLE_STRUCT(kudzu::Trigger, value);
// VISITABLE_STRUCT(kudzu::AnimationState, animation, animation_speed, repeat);
// VISITABLE_STRUCT(kudzu::Transition, from, to, transition_time, conditions);
// VISITABLE_STRUCT(kudzu::Condition, parameter_name, compare_value, compare_type);
