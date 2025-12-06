#include "hierarchy.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"

namespace tmt {

void Hierarchy::start_section() {
    /*ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));*/
    ImGui::BeginChild("HierarchySection", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
}

void Hierarchy::end_section() {
    ImGui::EndChild();
    // ImGui::PopStyleColor();
    drag_drop_target(entt::null);

    if (selected_index_previous_end_cache != null_index) {
        selected_index_previous_end = selected_index_previous_end_cache;
        selected_index_previous_end_cache = null_index;
    } else {
        selected_index_previous_end = null_index;
    }
}

bool Hierarchy::display_entity(const HierarchyState& state) {
    const bool root_indent = state.depth() == 0;
    const bool has_parent = state.transform.has_parent();
    if (root_indent && has_parent) return false;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

    const bool is_selected = selected_entities.contains(state.entity);
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const bool is_leaf = state.transform.has_children() == false;
    if (is_leaf) flags |= ImGuiTreeNodeFlags_Leaf;

    const bool is_begin = state.position == selected_index_begin;
    if (is_begin) flags |= ImGuiTreeNodeFlags_Bullet;

    /* Debug name */
    // const auto name = state.name.name + " " + std::to_string(state.indent_level) + "-" + std::to_string(state.level_index);
    const auto name = state.name.name;

    const bool open_node = ImGui::TreeNodeEx(name.c_str(), flags);
    /* TODO: is a too early because it toggles when mouse down, but we use mouse released for selection */
    const bool just_opened = ImGui::IsItemToggledOpen();

    const bool is_hovered = ImGui::IsItemHovered();
    const bool is_left_mouse_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const bool is_payload_being_dragged = ImGui::GetDragDropPayload() != nullptr;
    const bool is_left_clicked = is_hovered && is_left_mouse_released;
    const bool is_ctrl_held = ImGui::GetIO().KeyCtrl;
    const bool is_shift_held = ImGui::GetIO().KeyShift;

    /* Drag and drop logic */
    drag_drop_source(state.entity);
    const bool dropped = drag_drop_target(state.entity);

    /* Selection logic */
    const bool select_entity = !dropped && !just_opened && !is_payload_being_dragged && is_left_clicked;
    if (select_entity) {
        const bool is_first_selected = selected_entities.empty();
        if (is_shift_held) {
            if (is_first_selected) {
                selected_index_begin = state.position;
                selected_index_end = state.position;
            } else {
                selected_index_previous_end_cache = selected_index_end;
                selected_index_end = state.position;
            }
        } else if (is_ctrl_held) {
            if (is_selected) {
                selected_entities.erase(state.entity);
            } else {
                selected_entities.insert(state.entity);
            }
        } else {
            selected_entities.clear();
            selected_entities.insert(state.entity);
            selected_index_begin = state.position;
            selected_index_end = state.position;
        }
    }

    /* Check if entity is between selected */
    const bool is_between = Hierarchy::is_between(state.position, selected_index_begin, selected_index_end);
    if (is_between) {
        selected_entities.insert(state.entity);
    }

    const bool is_between_end_and_previous_end = Hierarchy::is_between(state.position, selected_index_end, selected_index_previous_end);

    /* detect if the end position has switched sides */
    const bool was_above = (selected_index_previous_end.y < selected_index_begin.y);
    const bool is_above = (selected_index_end.y < selected_index_begin.y);
    const bool was_below = (selected_index_previous_end.y > selected_index_begin.y);
    const bool is_below = (selected_index_end.y > selected_index_begin.y);
    const bool has_switched_sides = (was_above && is_below) || (was_below && is_above);

    if (is_between_end_and_previous_end && !has_switched_sides) {
        selected_entities.erase(state.entity);
    }

    /* If the node is closed, don't display children */
    if (open_node == false) return false;

    const bool has_children = state.transform.has_children();
    /* Display children */
    if (has_children) {
        uint32_t level_index = 0;
        for (auto child : state.transform.get_children()) {
            const bool has_components = engine.ecs.has_component<Name, Transform>(child);
            if (!has_components) continue;

            auto& child_transform = engine.ecs.get_component<Transform>(child);
            auto& child_name = engine.ecs.get_component<Name>(child);

            HierarchyState child_state(child, child_transform, child_name);
            child_state.position.x = state.depth() + 1;
            child_state.position.y = level_index;

            const bool displayed = display_entity(child_state);
            if (displayed) level_index++;
        }
    }

    ImGui::TreePop();
    return true;
}

bool Hierarchy::drag_drop_source(const Entity dragged_entity) const {
    if (ImGui::BeginDragDropSource()) {
        DragNDropPayload payload_data;
        if (selected_entities.size() > 1) {
            payload_data.multiple = true;
            payload_data.entity = dragged_entity;
        } else {
            payload_data.multiple = false;
            payload_data.entity = dragged_entity;
        }
        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &payload_data, sizeof(DragNDropPayload));
        const auto& name = engine.ecs.get_component<Name>(dragged_entity).name;
        ImGui::TextUnformatted(name.c_str());
        ImGui::EndDragDropSource();
        return true;
    }
    return false;
}

bool Hierarchy::drag_drop_target(const Entity dropped_entity) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
            const DragNDropPayload* payload_data = (DragNDropPayload*)payload->Data;
            if (payload_data->multiple) {
                /* Multiple entities being moved */
                for (const auto& selected_entity : selected_entities) {
                    if (selected_entity == dropped_entity) continue;
                    auto& transform = engine.ecs.get_component<Transform>(selected_entity);
                    transform.set_parent(dropped_entity);
                }
            } else {
                /* Single entity being moved */
                Entity single_entity = payload_data->entity;
                if (dropped_entity != single_entity) {
                    auto& transform = engine.ecs.get_component<Transform>(single_entity);
                    transform.set_parent(dropped_entity);
                }
            }
            ImGui::EndDragDropTarget();
            selected_index_begin = null_index;
            selected_index_end = null_index;
            selected_index_previous_end = null_index;
            return true;
        }
        ImGui::EndDragDropTarget();
    }
    return false;
}

}  // namespace tmt