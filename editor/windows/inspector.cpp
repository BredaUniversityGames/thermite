#include "inspector.hpp"

#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/engine.hpp"

#include "engine/core/system_collection.hpp"
#include "engine/core/components/all.hpp"
#include "engine/core/logger.hpp"

#include "engine/tools/serialize_defines.hpp"

#include "editor/editor.hpp"

#include "editor/imgui/types/glm.hpp"
#include "editor/imgui/types/entt.hpp"

#include "editor/windows/hierarchy.hpp"

#include "editor/imgui/components/all.hpp"
#include "editor/imgui/types/all.hpp"

namespace tmt {

void Inspector::display() {
    const auto& hierarchy = editor.windows.get<Hierarchy>();

    const MenuContext menu_context {
        .primary_entity = hierarchy.get_first_selected_entity(),
        .selected_entities = hierarchy.get_selected_entities(),
    };

    AllComponents::for_each([menu_context](auto type_tag) {
        using T = typename decltype(type_tag)::type;  // Extract type from tag

        const bool has_component = tmt::engine.ecs.has_component<T>(menu_context.primary_entity);
        if (has_component == false) return;

        T& component_instance = tmt::engine.ecs.get_component<T>(menu_context.primary_entity);

        const auto& name = tmt::Component<T>::get_name();
        const bool is_header_open = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
        const bool right_clicked = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

        const std::string popup_id = "ComponentOptionsPopup_" + std::string(name);
        if (right_clicked) {
            ImGui::OpenPopup(popup_id.c_str());
        }

        if (ImGui::BeginPopup(popup_id.c_str())) {
            if (ImGui::MenuItem("Remove Component")) {
                for (const Entity& entity : menu_context.selected_entities) {
                    const bool has_comp = engine.ecs.has_component<T>(entity);
                    if (has_comp == false) continue;
                    tmt::engine.ecs.remove_component<T>(entity);
                }
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
            if (ImGui::MenuItem("Copy Component")) {
                const json serialized = tmt::Serializer::serialize(component_instance);
                ImGui::SetClipboardText(serialized.dump().c_str());
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::BeginMenu("Paste")) {
                if (ImGui::MenuItem("Paste Component")) {
                    const char* clipboard_text = ImGui::GetClipboardText();
                    if (clipboard_text != nullptr) {
                        const json deserialized = json::parse(clipboard_text, nullptr, false);
                        if (deserialized.is_discarded()) {
                            Log::error("Failed to parse clipboard JSON for component '{}'", name);
                        } else {
                            for (const Entity& entity : menu_context.selected_entities) {
                                engine.ecs.add_or_get_component<T>(entity);
                                T& target_instance = tmt::engine.ecs.get_component<T>(entity);
                                Serializer::deserialize(deserialized, target_instance);
                            }
                        }
                    }
                    ImGui::CloseCurrentPopup();
                } else if (ImGui::MenuItem("Paste Values Only")) {
                    const char* clipboard_text = ImGui::GetClipboardText();
                    if (clipboard_text != nullptr) {
                        const json deserialized = json::parse(clipboard_text, nullptr, false);
                        if (deserialized.is_discarded()) {
                            Log::error("Failed to parse clipboard JSON for component '{}'", name);
                        } else {
                            for (const Entity& entity : menu_context.selected_entities) {
                                const bool has_comp = tmt::engine.ecs.has_component<T>(entity);
                                if (has_comp == false) continue;
                                T& target_instance = tmt::engine.ecs.get_component<T>(entity);
                                Serializer::deserialize(deserialized, target_instance);
                            }
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        if (is_header_open == false) return;

        const json before = tmt::Serializer::serialize(component_instance);

        const ImResponse response = ImReflect::Input("", component_instance);
        const bool changed = response.get<T>().is_changed();

        if (changed == false) return;
        if (menu_context.selected_entities.size() <= 1) return;

        auto after = tmt::Serializer::serialize(component_instance);
        const json diff = nlohmann::json::diff(before, after);

        for (const Entity& entity : menu_context.selected_entities) {
            if (entity == menu_context.primary_entity) continue;

            const bool has_component = tmt::engine.ecs.has_component<T>(entity);
            if (has_component == false) continue;

            T& other_instance = tmt::engine.ecs.get_component<T>(entity);
            json other_json = tmt::Serializer::serialize(other_instance);
            other_json.patch_inplace(diff);
            tmt::Serializer::deserialize(other_json, other_instance);
        }
    });

    add_component(menu_context);
}

void Inspector::add_component(const MenuContext& menu_context) {
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        AllComponents::for_each([menu_context](auto type_tag) {
            using T = typename decltype(type_tag)::type;  // Extract type from tag

            for (const Entity& entity : menu_context.selected_entities) {
                const bool has_component = tmt::engine.ecs.has_component<T>(entity);
                if (has_component) return;
                const auto& name = tmt::Component<T>::get_name();
                if (ImGui::MenuItem(name)) {
                    tmt::engine.ecs.add_component<T>(entity);
                }
            }
            ImGui::CloseCurrentPopup();
        });
        ImGui::EndPopup();
    }
}

void Inspector::on_editor_start() {}

void Inspector::on_editor_update(const tmt::FrameData& time) {}

void Inspector::on_editor_end() {}

}  // namespace tmt