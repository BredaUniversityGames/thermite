#include "animation_system.hpp"

#include "core/logger.hpp"

#include "engine.hpp"
#include "rig_model.hpp"
// #include "graphics/debug_render.hpp"
#include "engine/core/renderer/pipelines/debug_pipeline.hpp"
#include "engine/core/renderer/renderer.hpp"
// #include "tools/profiler.hpp"

#define CONDITION(comparison) [](const Variant& parameter, const Variant& value) -> bool { return parameter comparison value; }

using namespace tmt;

const Condition::CompareFunctionMap Condition::COMPARE_MAP {
    {CompareType::EQUAL, CONDITION(==)},
    {CompareType::NOT_EQUAL, CONDITION(!=)},
    {CompareType::GREATER, CONDITION(>)},
    {CompareType::LESS, CONDITION(<)},
};

#pragma region RigController

tmt::RigController::RigController() {}

void tmt::RigController::on_destroy(entt::registry& registry, Entity entity) {}

void RigController::set_parameter_float(const std::string& name, const float value) {
    if (parameters.find(name) == parameters.end()) {
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
    if (parameters.find(name) == parameters.end()) {
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
    if (parameters.find(name) == parameters.end()) {
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
    if (parameters.find(name) == parameters.end()) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return;
    }

    Variant& parameter = parameters.at(name);
    if (parameter.index() != 3) {
        Log::error("Parameter \"{}\" is not trigger type!", name);
        return;
    }

    parameter = Trigger {value};
}

float RigController::get_parameter_float(const std::string& name) {
    if (parameters.find(name) == parameters.end()) {
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

int RigController::get_parameter_int(const std::string& name) {
    if (parameters.find(name) == parameters.end()) {
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

bool RigController::get_parameter_bool(const std::string& name) {
    if (parameters.find(name) == parameters.end()) {
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

bool RigController::get_parameter_trigger(const std::string& name) {
    if (parameters.find(name) == parameters.end()) {
        Log::error("Invalid parameter with name: \"{}\"", name);
        return false;
    }

    Variant& parameter = parameters.at(name);
    if (parameter.index() != 3) {
        Log::error("Parameter \"{}\" is not trigger type!", name);
        return false;
    }

    return std::get<Trigger>(parameter).value;
}

void RigController::remove_state(const std::string& name) {
    if (name == "Start" || states.find(name) == states.end()) return;

    states.erase(name);
    const auto new_end = std::remove_if(transitions.begin(), transitions.end(), [name](const Transition& transition) { return transition.from == name || transition.to == name; });
    transitions.erase(new_end, transitions.end());
}

void RigController::remove_parameter(const std::string& name) {
    if (parameters.find(name) == parameters.end()) return;

    parameters.erase(name);
    for (auto& transition : transitions) {
        auto& conditions = transition.conditions;
        const auto new_end = std::remove_if(conditions.begin(), conditions.end(), [name](const Condition& condition) { return condition.parameter_name == name; });

        conditions.erase(new_end, conditions.end());
    }
}

void RigController::rename_state(const std::string& old_name, const std::string& new_name) {
    if (states.find(old_name) == states.end()) return;

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
    if (parameters.find(old_name) == parameters.end()) return;

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

#pragma endregion

#pragma region RigModelManager

// RigModelManager::RigModelManager() : System(Engine.ecs()) { IKeyboardListener::priority = 1; }

std::string tmt::RigModelManager::get_name() { return "animation system"; }

void tmt::RigModelManager::on_start() {}

void RigModelManager::on_update(const FrameData& time) {
    Inspect(time.delta_time);
    for (const auto& [entity, rig, controller] : engine.ecs.get_registry().view<RigModel, RigController>().each()) {
        const std::string& current_state = controller.current_state;
        if (current_state.empty()) {
            const auto& transform = engine.ecs.get_component<Transform>(entity);
            const auto& name = engine.ecs.get_component<Name>(entity);
            Log::error("Trying to animate \"{}\" without a state!", name.name);
            continue;
        }

        for (auto& transition : controller.transitions) {
            if (transition.from != current_state) continue;

            std::vector<Trigger*> satisfied_triggers;
            const bool should_transition = controller.check_transition_conditions(transition, satisfied_triggers);
            if (!should_transition) continue;

            const AnimationState& from_state = controller.states[transition.from];
            if (from_state.transition_on_finish) {
                const auto& animations = rig.data->bones.back().animations;
                const auto& iterator = animations.find(from_state.animation);

                if (iterator != animations.end() && rig.time < (iterator->second.keyframes_pos.back().time - transition.transition_time)) continue;
            }

            // If the transition is successful we reset all the trigger parameters
            std::for_each(satisfied_triggers.begin(), satisfied_triggers.end(), [](Trigger* trigger) { *trigger = Trigger {false}; });

            const std::string& state_name = controller.current_state = transition.to;
            const AnimationState& state = controller.states[state_name];

            rig.set_animation_speed(state.animation_speed);

            if (state.animation.empty()) {
                if (rig.state == RigModel::State::ANIMATE_LOOP) rig.stop_loop(transition.transition_time);
                break;
            }

            rig.play_animation(state.animation, transition.transition_time, state.repeat);
            break;
        }
    }

    for (const auto& [entity, rig] : engine.ecs.get_registry().view<RigModel>().each()) {
        if (rig.state != RigModel::State::STATIONARY && !rig.is_transferring()) rig.time += rig.animation_speed * time.delta_time;

        if (rig.is_transferring()) rig.transfer_time += time.delta_time * rig.animation_speed;

        if (rig.transfer_time > rig.transfer_threshold) {
            switch (rig.state) {
                case RigModel::State::TRANSFERRING_TO_LOOP:
                    rig.state = RigModel::State::ANIMATE_LOOP;
                    break;
                case RigModel::State::TRANSFERRING_TO_ONCE:
                    rig.state = RigModel::State::ANIMATE_ONCE;
                    break;
                case RigModel::State::TRANSFERRING_TO_STOP:
                    rig.state = RigModel::State::STATIONARY;
                    break;

                case RigModel::State::ANIMATE_LOOP:
                case RigModel::State::ANIMATE_ONCE:
                case RigModel::State::STATIONARY:
                    break;
            }

            rig.time = 0.0f;
            rig.set_current_animation(rig.get_next_animation());
            rig.transfer_time = 0.0f;
        }

        const Bone& last_bone = rig.data->bones.back();
        const std::string& current_animation = rig.get_current_animation();
        if (last_bone.animations.find(current_animation) == last_bone.animations.end()) {
            Log::warn(R"(Animation "{}" isn't a valid animation!)", current_animation);
            continue;
        }

        const float animation_time = last_bone.animations.at(rig.get_current_animation()).keyframes_pos.back().time;

        if (rig.time >= animation_time) {
            if (rig.state == RigModel::State::ANIMATE_ONCE) {
                rig.state = RigModel::State::STATIONARY;
            } else if (rig.state == RigModel::State::ANIMATE_LOOP) {
                rig.time = rig.time - animation_time;
            }
        }

        // Engine.profiler().BeginSection("animation");

        for (const Entity bone_entity : rig.bone_entities) {
            auto& transform = engine.ecs.get_component<Transform>(bone_entity);
            // if (!transform.is_enabled()) continue;

            const BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entity);
            Bone& bone = rig.data->bones[bone_comp_id.id];

            rig.animate_translation(transform, bone);
            rig.animate_rotation(transform, bone);
            rig.animate_scale(transform, bone);
        }

        // Engine.profiler().EndSection("animation");
    }
}

void tmt::RigModelManager::on_end() {}

void RigModelManager::inspect(float) {
    int lines_drawn = 0;
    for (const auto& [entity, transform, rig] : engine.ecs.get_registry().view<Transform, RigModel>().each()) {
        for (const Entity bone_entity : rig.bone_entities) {
            auto& bone_transform = engine.ecs.get_component<Transform>(bone_entity);
            auto& bone_name = engine.ecs.get_component<Name>(bone_entity);
            const glm::vec3 joint = bone_transform.get_world_position();

            if (bone_transform.has_parent()) {
                auto& parent_transform = engine.ecs.get_component<Transform>(bone_transform.get_parent());
                const glm::vec3 parent_joint = parent_transform.get_world_position();
                engine.renderer.draw_line(parent_joint, joint, glm::vec3(1.0f, 0.0f, 0.0f));
                lines_drawn++;
            }
        }
    }
}

/*
#include "editor/inspector.hpp"
#include "editor/editor_colors.hpp"
#include <imgui/imgui_stdlib.h>
#include <imgui/imnodes.h>
#include <imnodes_internal.h>

namespace {

ImVec2 get_node_position(const int id) { return ImNodes::GetNodeScreenSpacePos(id) + (ImNodes::GetNodeDimensions(id) / 2.0); }

template <typename Type>
[[nodiscard]] std::string make_unique_name(const std::map<std::string, Type>& map, const std::string& string, int index = 1) {
    if (!string.empty() && !map.contains(string)) return string;

    const auto format = fmt::format(fmt::runtime(string + " ({})"), index);
    if (!map.contains(format)) return format;

    return make_unique_name(map, string, ++index);
}

bool mouse_clicked_state_node(const int node_id, const ImGuiMouseButton button) {
    if (ImGui::IsMouseClicked(button)) {
        const ImVec2 position = ImNodes::GetNodeScreenSpacePos(node_id);
        const ImVec2 size = ImNodes::GetNodeDimensions(node_id);
        if (ImGui::IsMouseHoveringRect(position, position + size)) return true;
    }

    return false;
}

const std::map<Condition::CompareType, std::string> COMPARE_NAMES {
    {Condition::CompareType::EQUAL, "=="}, {Condition::CompareType::NOT_EQUAL, "!="}, {Condition::CompareType::GREATER, ">"}, {Condition::CompareType::LESS, "<"}
};

ImRect grid_rect;

}  // namespace

void RigModelManager::draw_transition(const ImVec2 start, const ImVec2 middle, const int transition_index) const {
    constexpr float triangle_size = 12.0f;
    ImU32 color = 0xFFFFFFFF;
    if (transition_index == selected_transition) color = 0xFFEE9A39;  // Selection blue

    // Fancy math is to calculate a regular triangle that is actually in the middle of the line and looks good
    const ImVec2 vector = middle - start;
    ImGui::GetWindowDrawList()->AddLine(start, start + (vector * 2.0f), color, 5.0f);

    const glm::vec2 direction = normalize(glm::vec2 {vector.x, vector.y});

    const float angle = glm::atan(direction.x, direction.y);
    constexpr float offset_angle = glm::two_pi<float>() / 3.0f;

    const ImVec2 forward_point = middle + ImVec2 {glm::sin(angle), glm::cos(angle)} * triangle_size;
    const ImVec2 left_point = middle + ImVec2 {glm::sin(angle - offset_angle), glm::cos(angle - offset_angle)} * triangle_size;
    const ImVec2 right_point = middle + ImVec2 {glm::sin(angle + offset_angle), glm::cos(angle + offset_angle)} * triangle_size;

    ImGui::GetWindowDrawList()->AddTriangleFilled(forward_point, left_point, right_point, color);
}

void RigModelManager::draw_transition_select(const ImVec2 middle, int transition_index, const ImVec2 clickable_size) {
    const ImVec2 middle_pos = middle - clickable_size / 2.0f;

    ImGui::PushID(transition_index);

    ImGui::SetCursorPos(middle_pos - ImGui::GetWindowPos());
    if (ImGui::InvisibleButton("##Connection", clickable_size, ImGuiButtonFlags_PressedOnClick)) {
        selected_state.clear();
        selected_transition = transition_index;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !ImGui::IsPopupOpen("Transition context")) {
        selected_state.clear();
        selected_transition = transition_index;
        open_transition_context = true;
    }

    ImGui::PopID();
}

void RigModelManager::list_parameters(RigController& controller) {
    static DiffUtil<RigController> diff_util;

    auto& parameters = controller.parameters;

    static std::string search;
    ImGui::InputText("##search", &search);
    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_PLUS)) ImGui::OpenPopup("AddParameter");

    if (ImGui::BeginPopup("AddParameter")) {
        if (ImGui::MenuItem("float")) parameters[make_unique_name(parameters, "New float")] = 0.0f;
        if (ImGui::MenuItem("int")) parameters[make_unique_name(parameters, "New int")] = 0;
        if (ImGui::MenuItem("bool")) parameters[make_unique_name(parameters, "New bool")] = false;
        if (ImGui::MenuItem("trigger")) parameters[make_unique_name(parameters, "New trigger")] = Trigger {false};

        ImGui::EndPopup();
    }

    ImGui::Separator();

    static std::string selected_parameter;
    std::pair<std::string, std::string> edited_map_item;  // Item used to edit the map if the user changed the name of a parameter

    int index = 0;
    for (auto& [name, value] : parameters) {
        if (name.find(search) == std::string::npos) continue;

        ImGui::PushID(index);
        ImGui::BeginGroup();
        std::string editable = name;
        if (ImGui::InputText("##ParameterName", &editable, ImGuiInputTextFlags_EnterReturnsTrue)) edited_map_item = std::make_pair(name, editable);

        ImGui::SameLine();

        const size_t type_index = value.index();
        ImGui::EndDisabled();  // Disabled begin and end are flipped because we want to keep this region enabled within the disabled calls
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        switch (type_index) {
            case 0:  // float
                ImGui::DragFloat("##Parameter", &std::get<float>(value));
                break;
            case 1:  // int
                ImGui::DragInt("##Parameter", &std::get<int>(value));
                break;
            case 2:  // bool
                ImGui::Checkbox("##Parameter", &std::get<bool>(value));
                break;
            case 3: {  // Trigger
                auto& trigger = std::get<Trigger>(value);
                if (ImGui::RadioButton("##Parameter", trigger.value)) trigger.value = !trigger.value;
            } break;

            default:
                break;
        }
        ImGui::BeginDisabled(Engine.is_playing());
        ImGui::EndGroup();
        ImGui::PopID();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            selected_parameter = name;
            ImGui::OpenPopup("Parameter context");
        }

        index++;
    }

    if (!edited_map_item.first.empty()) {
        diff_util.take_snapshot(&controller);
        controller.rename_parameter(edited_map_item.first, make_unique_name(parameters, edited_map_item.second));
        diff_util.commit_changes("Rename parameter");
    }

    if (ImGui::BeginPopup("Parameter context")) {
        if (ImGui::MenuItem("Delete parameter")) {
            diff_util.take_snapshot(&controller);
            controller.remove_parameter(selected_parameter);
            diff_util.commit_changes("Delete parameter");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RigModelManager::list_animations(const RigModel& model) {
    for (auto& [name, animation] : model.data->bones[0].animations) {
        ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth);
        if (ImGui::BeginDragDropSource()) {
            ImGui::TextUnformatted(name.c_str());
            ImGui::SetDragDropPayload("Animation name", &name, sizeof(std::string));
            ImGui::EndDragDropSource();
        }
    }
}

void RigModelManager::draw_transitions(const RigController& controller) {
    const auto& transitions = controller.transitions;

    for (int i = 0; i < transitions.size(); i++) {
        auto& transition = transitions.at(i);
        if (!positions.contains(transition.from) || !positions.contains(transition.to)) continue;

        ImVec2 start = positions[transition.from];
        ImVec2 middle = (start + positions[transition.to]) / 2.0f;

        const auto find_opposite_transition = std::ranges::find_if(transitions, [transition](const Transition& other) { return transition.from == other.to && transition.to == other.from; });
        if (find_opposite_transition != transitions.end()) {
            const glm::vec2 swizzle = glm::normalize(glm::vec2 {-(middle.y - start.y), middle.x - start.x});
            start += ImVec2 {swizzle.x, swizzle.y} * 10.0f;
            middle += ImVec2 {swizzle.x, swizzle.y} * 10.0f;
        }

        draw_transition(start, middle, i);
        draw_transition_select(middle, i);
    }

    if (!connecting_state.empty()) {
        const ImVec2 start = positions[connecting_state];
        draw_transition(start, (start + ImGui::GetMousePos()) / 2.0f, -1);
    }
}

void RigModelManager::draw_states(RigController& controller) {
    positions.clear();
    for (auto& [name, state] : controller.states) {
        const int hash = static_cast<int>(std::hash<std::string> {}(name));

        if (name == controller.current_state) {
            ImNodes::PushColorStyle(ImNodesCol_NodeBackground, 0xFF177032);
            ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundHovered, 0xFF26BC54);
            ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundSelected, 0xFF26BC54);
        }
        ImNodes::BeginNode(hash);
        ImGui::TextUnformatted(name.c_str());

        if (mouse_clicked_state_node(hash, ImGuiMouseButton_Left)) {
            if (!connecting_state.empty()) {
                const auto exists_iterator =
                    std::ranges::find_if(controller.transitions, [this, name](const Transition& transition) { return transition.from == connecting_state && transition.to == name; });
                if (exists_iterator == controller.transitions.end() && connecting_state != name) {
                    DiffUtil<RigController> diff_util;
                    diff_util.take_snapshot(&controller);
                    controller.transitions.push_back(Transition {connecting_state, name});
                    diff_util.commit_changes("Create transition");
                }

                connecting_state.clear();
            } else {
                selected_transition = -1;
                selected_state = name;
            }
        }

        if (mouse_clicked_state_node(hash, ImGuiMouseButton_Right) && !ImGui::IsPopupOpen("State context")) {
            selected_transition = -1;
            selected_state = name;
            open_state_context = true;
        }

        positions[name] = get_node_position(hash);
        ImNodes::EndNode();

        if (name == controller.current_state) {
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
        }
    }

    // Clicked while connecting, but no node is being clicked
    if (!connecting_state.empty() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) connecting_state.clear();
}

void RigModelManager::context_popups(RigController& controller) {
    static DiffUtil<RigController> diff_util;

    if (open_state_context) {
        open_state_context = false;
        ImGui::OpenPopup("State context");
    } else if (open_transition_context) {
        open_transition_context = false;
        ImGui::OpenPopup("Transition context");
    } else if (connecting_state.empty() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && grid_rect.Contains(ImGui::GetMousePos())) {
        ImGui::OpenPopup("General context");
    }

    if (ImGui::BeginPopup("State context")) {
        if (!selected_state.empty()) {
            if (ImGui::MenuItem("Connect state")) {
                connecting_state = selected_state;
                ImGui::CloseCurrentPopup();
            }

            if (selected_state != "Start" && ImGui::MenuItem("Delete state")) {
                diff_util.take_snapshot(&controller);
                ImNodes::ClearNodeSelection();
                controller.remove_state(selected_state);
                if (controller.current_state == selected_state) controller.current_state.clear();
                diff_util.commit_changes("Delete state");
                selected_state.clear();
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Transition context")) {
        if (selected_transition != -1) {
            if (ImGui::MenuItem("Delete transition")) {
                diff_util.take_snapshot(&controller);
                controller.transitions.erase(controller.transitions.begin() + selected_transition);
                selected_transition = -1;
                diff_util.commit_changes("Delete transition");
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("General context")) {
        if (ImGui::MenuItem("Create state")) {
            diff_util.take_snapshot(&controller);
            const std::string state_name = make_unique_name(controller.states, "New state");
            const int hash = static_cast<int>(std::hash<std::string> {}(state_name));
            controller.states[state_name];
            ImNodes::SetNodeScreenSpacePos(hash, ImGui::GetMousePos());
            diff_util.commit_changes("Create state");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RigModelManager::inspect_state(RigController& controller) {
    if (selected_state == "Start") return;
    static DiffUtil<AnimationState> state_diff;
    auto& state = controller.states[selected_state];

    ImGui::SeparatorText("State settings: ");

    ImGui::SameLine();

    std::string editable = selected_state;
    if (ImGui::InputText("##ParameterName", &editable, ImGuiInputTextFlags_EnterReturnsTrue)) {
        const std::string& new_name = make_unique_name(controller.states, editable);

        DiffUtil<RigController> diff_util;
        diff_util.take_snapshot(&controller);
        controller.rename_state(selected_state, new_name);

        // Sets the node position properly to not launch the node to (0, 0) when the state is renamed;
        constexpr std::hash<std::string> hasher;
        const ImVec2 node_position = ImNodes::GetNodeScreenSpacePos(static_cast<int>(hasher(selected_state)));
        ImNodes::SetNodeScreenSpacePos(static_cast<int>(hasher(new_name)), node_position);

        diff_util.commit_changes("Rename state");
        selected_state = new_name;
    }

    ImGui::BeginGroup();

    ImGui::InputText("Animation", &state.animation, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Animation name");
        if (payload) {
            state_diff.take_snapshot(&state);
            const std::string& animation_name = *static_cast<std::string*>(payload->Data);
            state.animation = animation_name;
            state_diff.commit_changes("Edit state animation");
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::DragFloat("Animation speed", &state.animation_speed);
    ImGui::Checkbox("Repeat", &state.repeat);

    ImGui::EndGroup();

    if (ImGui::IsItemActivated()) state_diff.take_snapshot(&state);
    if (ImGui::IsItemDeactivatedAfterEdit()) state_diff.commit_changes("Edit state");
}

void RigModelManager::inspect_transition(RigController& controller) {
    static DiffUtil<Transition> diff_util;
    auto& transition = controller.transitions[selected_transition];

    ImGui::SeparatorText("Transition settings");

    const float text_input_width = (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(ICON_MD_ARROW_RIGHT).x) / 2.0f;
    ImGui::PushItemWidth(text_input_width);
    ImGui::InputText("##From", &transition.from, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();

    ImGui::Text(ICON_MD_ARROW_RIGHT);
    ImGui::SameLine();

    ImGui::PushItemWidth(text_input_width);
    ImGui::InputText("##To", &transition.to, ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();

    ImGui::DragFloat("Transition time", &transition.transition_time);
    if (ImGui::IsItemActivated()) diff_util.take_snapshot(&transition);
    if (ImGui::IsItemDeactivatedAfterEdit()) diff_util.commit_changes("Transition edited");

    ImGui::NewLine();

    list_conditions(controller, transition);
}

void RigModelManager::list_conditions(RigController& controller, Transition& transition) {
    static DiffUtil<Transition> diff_util;
    static int condition_index = -1;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, EDITCOL_BG_2);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, EDITCOL_BG_1);
    ImGui::PushStyleColor(ImGuiCol_Border, EDITCOL_BG_4);
    ImGui::BeginChild("Conditions", ImVec2 {0.0f, 0.0f}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
        ImGui::Text("Conditions");
        ImGui::EndMenuBar();
    }

    for (int i = 0; i < transition.conditions.size(); i++) {
        auto& condition = transition.conditions[i];

        ImGui::PushID(i);
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::BeginCombo("##Parameter name", condition.parameter_name.c_str())) {
            for (auto& [name, value] : controller.parameters) {
                if (ImGui::Selectable(name.c_str(), false)) {
                    condition.parameter_name = name;
                    condition.compare_value = value;
                    if (value.index() == 0)  // Float type
                        condition.compare_type = Condition::CompareType::GREATER;
                    else
                        condition.compare_type = Condition::CompareType::EQUAL;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();

        const size_t type_index = condition.compare_value.index();
        switch (type_index) {
            case 0:  // float
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                if (ImGui::BeginCombo("##Comparison type", COMPARE_NAMES.at(condition.compare_type).c_str())) {
                    if (ImGui::Selectable(">", false)) condition.compare_type = Condition::CompareType::GREATER;
                    if (ImGui::Selectable("<", false)) condition.compare_type = Condition::CompareType::LESS;
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat("##Parameter", &std::get<float>(condition.compare_value));
                break;
            case 1:  // int
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                if (ImGui::BeginCombo("##Comparison type", COMPARE_NAMES.at(condition.compare_type).c_str())) {
                    if (ImGui::Selectable("==", false)) condition.compare_type = Condition::CompareType::EQUAL;
                    if (ImGui::Selectable("!=", false)) condition.compare_type = Condition::CompareType::NOT_EQUAL;
                    if (ImGui::Selectable(">", false)) condition.compare_type = Condition::CompareType::GREATER;
                    if (ImGui::Selectable("<", false)) condition.compare_type = Condition::CompareType::LESS;
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragInt("##Parameter", &std::get<int>(condition.compare_value));
                if (ImGui::IsItemDeactivatedAfterEdit()) diff_util.commit_changes("Condition value edited");
                if (ImGui::IsItemActivated()) diff_util.take_snapshot(&transition);
                break;
            case 2: {  // bool
                auto& value = std::get<bool>(condition.compare_value);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##Parameter", (value ? "true" : "false"))) {
                    if (ImGui::Selectable("true", false)) value = true;
                    if (ImGui::Selectable("false", false)) value = false;
                    ImGui::EndCombo();
                }
            } break;
            case 3:  // trigger
                ImGui::NewLine();
                break;
            default:
                break;
        }
        ImGui::EndGroup();
        ImGui::PopID();

        if (ImGui::IsItemActivated()) diff_util.take_snapshot(&transition);
        if (ImGui::IsItemDeactivatedAfterEdit()) diff_util.commit_changes("Condition edited");

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            condition_index = i;
            ImGui::OpenPopup("Condition context");
        }
    }

    if (ImGui::BeginPopup("Condition context")) {
        if (ImGui::MenuItem("Delete condition")) {
            diff_util.take_snapshot(&transition);
            transition.conditions.erase(transition.conditions.begin() + condition_index);
            diff_util.commit_changes("Delete condition");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::BeginDisabled(controller.parameters.empty());
    if (ImGui::Button(ICON_MD_PLUS)) {
        auto& [name, value_variant] = *controller.parameters.begin();
        const size_t type_index = value_variant.index();

        Condition condition;
        switch (type_index) {
            case 0:  // Float
                condition = Condition {name, 0.0f, Condition::CompareType::GREATER};
                break;
            case 1:  // Int
                condition = Condition {name, 0, Condition::CompareType::EQUAL};
                break;
            case 2:  // Bool
                condition = Condition {name, true, Condition::CompareType::EQUAL};
                break;
            case 3:  // Trigger
                condition = Condition {name, Trigger {false}, Condition::CompareType::EQUAL};
            default:
                break;
        }

        diff_util.take_snapshot(&transition);
        transition.conditions.push_back(condition);
        diff_util.commit_changes("Condition added");
    }
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

void RigModelManager::OnPanel() {
    static DiffUtil<RigController> diff_util;

    const Entity selected = Engine.inspector().scene.selected_entity();
    auto [model, controller] = Engine.ecs().registry.try_get<RigModel, RigController>(selected);

    const bool valid_controller = (model != nullptr && controller != nullptr);
    if (!valid_controller) {
        selected_state.clear();
        selected_transition = -1;
    }

    bool selected_item = false;
    if (valid_controller) selected_item = (controller->states.contains(selected_state) || selected_transition != -1);

    const bool has_selection = selected_item && valid_controller;

    ImGui::BeginDisabled(Engine.is_playing());
    if (ImGui::BeginTable("Rig controller table", 2, ImGuiTableFlags_Resizable, ImVec2 {0.0f, ImGui::GetContentRegionAvail().y})) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Animations")) {
                ImGui::Separator();

                if (valid_controller) list_animations(*model);

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Parameters")) {
                if (valid_controller) list_parameters(*controller);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::TableSetColumnIndex(1);

        if (valid_controller) {
            const ImVec2 start_position = ImGui::GetCursorPos();
            ImGui::Dummy(ImGui::GetContentRegionAvail());
            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Animation name");
                if (payload) {
                    const std::string& animation_name = *static_cast<std::string*>(payload->Data);

                    diff_util.take_snapshot(controller);
                    const auto unique_name = make_unique_name(controller->states, animation_name);
                    controller->states[unique_name] = AnimationState {animation_name};

                    const int hash = static_cast<int>(std::hash<std::string> {}(unique_name));
                    ImNodes::SetNodeScreenSpacePos(hash, ImGui::GetMousePos());
                    diff_util.commit_changes("Create state");
                }

                ImGui::EndDragDropTarget();
            }
            ImGui::SetCursorPos(start_position);

            ImNodes::EditorContextSet(controller->editor_context);

            // Setup grid rect for context menus later
            grid_rect.Min = ImGui::GetCursorPos() + ImGui::GetWindowPos();
            grid_rect.Max = grid_rect.Min + ImGui::GetContentRegionAvail();

            ImNodes::BeginNodeEditor();
            draw_transitions(*controller);
            draw_states(*controller);
            ImNodes::EndNodeEditor();

            context_popups(*controller);

        } else {
            ImNodes::EditorContextSet(ImNodes::GetCurrentContext()->DefaultEditorCtx);  // Set the context to the default so the other contexts don't get messed up.
            ImNodes::BeginNodeEditor();
            ImNodes::EndNodeEditor();
        }

        ImGui::EndTable();
    }

    if (ImGui::Begin("Controller inspector") && has_selection) {
        if (!selected_state.empty())
            inspect_state(*controller);
        else if (selected_transition != -1)
            inspect_transition(*controller);
    }
    ImGui::End();

    ImGui::EndDisabled();
}

void RigModelManager::on_key_event(KeyboardEvent& event) {
    if (event.action != Input::KeyAction::Press || Engine.is_playing()) return;

    DiffUtil<RigController> diff_util;
    switch (event.key) {
        case Input::KeyboardKey::Delete: {
            auto&& [controller, model] = Engine.ecs().registry.try_get<RigController, RigModel>(Engine.inspector().scene.selected_entity());
            if (controller == nullptr || model == nullptr) break;

            if (selected_transition != -1) {
                if (selected_transition >= controller->transitions.size()) break;

                diff_util.take_snapshot(controller);
                controller->transitions.erase(controller->transitions.begin() + selected_transition);
                selected_transition = -1;
                diff_util.commit_changes("Delete transition");

            } else {
                std::vector<int> selected_nodes;
                selected_nodes.resize(ImNodes::NumSelectedNodes());
                if (selected_nodes.empty()) {
                    if (selected_state.empty()) break;

                    diff_util.take_snapshot(controller);
                    controller->remove_state(selected_state);
                    selected_state.clear();
                    diff_util.commit_changes("Delete state");
                    event.handled = true;
                    break;
                }

                ImNodes::GetSelectedNodes(selected_nodes.data());
                constexpr std::hash<std::string> hasher;

                diff_util.take_snapshot(controller);

                // Copy of the sates map to iterate over while removing items from the real map
                const auto states_copy = controller->states;
                for (const auto& [name, state] : states_copy) {
                    if (std::ranges::count(selected_nodes, static_cast<int>(hasher(name))) > 0) controller->remove_state(name);
                }

                diff_util.commit_changes("Delete state(s)");
            }
            event.handled = true;
            break;
        }
        default:
            break;
    }
}

RigController::RigController() {
    if (editor_context == nullptr) editor_context = ImNodes::EditorContextCreate();

    // Destroying components is WEIRD, sometimes the destructor gets called 2 times because a second object is created, and if
    // you have a pointer which needs to be freed this causes problems
    // This is the awful solution
    const bool not_connected = Engine.ecs().registry.on_destroy<RigController>().empty();
    if (not_connected) Engine.ecs().registry.on_destroy<RigController>().connect<&on_destroy>();
}

void RigController::on_destroy(entt::registry& registry, const Entity entity) {
    auto& controller = registry.get<RigController>(entity);
    ImNodes::EditorContextFree(controller.editor_context);
}

#else

RigController::RigController() {}
void RigController::on_destroy(entt::registry&, const Entity) {}
void RigModelManager::on_key_event(KeyboardEvent&) {}

#endif

#pragma endregion

*/
