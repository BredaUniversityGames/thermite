#include "rig_controller.hpp"

#include "engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"

#ifdef THERMITE_EDITOR
    #include <extern/imgui-node-editor/imgui_node_editor.h>
#endif  // THERMITE_EDITOR

namespace tmt {

#ifdef THERMITE_EDITOR

RigController::RigController() {
    if (editor_context == nullptr) editor_context = ax::NodeEditor::CreateEditor();

    // Destroying components is WEIRD, sometimes the destructor gets called 2 times because a second object is created, and
    // if you have a pointer which needs to be freed this causes problems This is the awful solution
    const bool not_connected = engine.ecs.get_registry().on_destroy<RigController>().empty();
    if (not_connected) engine.ecs.get_registry().on_destroy<RigController>().connect<&on_destroy>();
}

void RigController::on_destroy(entt::registry& registry, const Entity entity) {
    const auto& controller = registry.get<RigController>(entity);
    ax::NodeEditor::DestroyEditor(controller.editor_context);
}

#endif  // THERMITE_EDITOR

void RigController::set_parameter_float(const std::string& name, const float value) {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return;
    }

    Variant& parameter = parameters.at(name);
    if (parameter.index() != 0) {
        Log::error("Parameter \"{}\" is not float type!", name);
        return;
    }

    parameter = value;
}

void RigController::set_parameter_int(const std::string& name, const int value) {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return;
    }

    Variant& parameter = parameters.at(name);
    if (parameter.index() != 1) {
        Log::error("Parameter \"{}\" is not int type!", name);
        return;
    }

    parameter = value;
}

void RigController::set_parameter_bool(const std::string& name, const bool value) {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return;
    }

    Variant& parameter = parameters.at(name);
    if (parameter.index() != 2) {
        Log::error("Parameter \"{}\" is not bool type!", name);
        return;
    }

    parameter = value;
}

void RigController::set_parameter_trigger(const std::string& name, const bool value) {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return;
    }

    Variant& parameter = parameters.at(name);
    if (parameter.index() != 3) {
        Log::error("Parameter \"{}\" is not trigger type!", name);
        return;
    }

    parameter = Trigger { value };
}

float RigController::get_parameter_float(const std::string& name) const {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return 0.0f;
    }

    const Variant& parameter = parameters.at(name);
    if (parameter.index() != 0) {
        Log::error("Parameter \"{}\" is not float type!", name);
        return 0.0f;
    }

    return std::get<float>(parameter);
}

int RigController::get_parameter_int(const std::string& name) const {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return 0;
    }

    const Variant& parameter = parameters.at(name);
    if (parameter.index() != 1) {
        Log::error("Parameter \"{}\" is not int type!", name);
        return 0;
    }

    return std::get<int>(parameter);
}

bool RigController::get_parameter_bool(const std::string& name) const {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return false;
    }

    const Variant& parameter = parameters.at(name);
    if (parameter.index() != 2) {
        Log::error("Parameter \"{}\" is not bool type!", name);
        return false;
    }

    return std::get<bool>(parameter);
}

bool RigController::get_parameter_trigger(const std::string& name) const {
    if (!parameters.contains(name)) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return false;
    }

    const Variant& parameter = parameters.at(name);
    if (parameter.index() != 3) {
        Log::error("Parameter \"{}\" is not trigger type!", name);
        return false;
    }

    return std::get<Trigger>(parameter).value;
}

void RigController::remove_state(const std::string& name) {
    if (name == "Start" || !states.contains(name)) return;

    states.erase(name);
    std::erase_if(transitions, [name](const Transition& transition) { return transition.from == name || transition.to == name; });
}

void RigController::remove_parameter(const std::string& name) {
    if (!parameters.contains(name)) return;

    parameters.erase(name);
    for (auto& transition : transitions) {
        auto& conditions = transition.conditions;
        std::erase_if(conditions, [name](const Condition& condition) { return condition.parameter_name == name; });
    }
}

void RigController::rename_state(const std::string& old_name, const std::string& new_name) {
    if (!states.contains(old_name)) return;

    auto extracted = states.extract(old_name);
    extracted.key() = new_name;
    states.insert(std::move(extracted));

    for (auto& transition : transitions) {
        if (transition.from == old_name)
            transition.from = new_name;
        else if (transition.to == old_name)
            transition.to = new_name;
    }
}

void RigController::rename_parameter(const std::string& old_name, const std::string& new_name) {
    if (!parameters.contains(old_name)) return;

    auto extracted = parameters.extract(old_name);
    extracted.key() = new_name;
    parameters.insert(std::move(extracted));

    for (auto& transition : transitions) {
        auto& conditions = transition.conditions;
        for (auto& condition : conditions) {
            if (condition.parameter_name == old_name) condition.parameter_name = new_name;
        }
    }
}

bool RigController::check_transition_conditions(const Transition& transition, std::vector<Trigger*>& satisfied_triggers) {
    for (auto& condition : transition.conditions) {
        Variant& parameter = parameters.at(condition.parameter_name);
        if (!condition.is_satisfied(parameter)) return false;

        auto* trigger = std::get_if<Trigger>(&parameter);
        if (trigger != nullptr) satisfied_triggers.push_back(trigger);
    }

    return true;
}

}  // namespace tmt