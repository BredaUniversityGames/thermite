#include "inspector.hpp"

#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/engine.hpp"
#include "engine/core/system_collection.hpp"
#include "engine/core/components/all.hpp"

#include "editor/editor.hpp"

#include "editor/imgui/types/glm.hpp"
#include "editor/imgui/types/entt.hpp"

#include "editor/windows/hierarchy.hpp"

namespace tmt {

void Inspector::display() {
    const auto& hierarchy = editor.windows.get<Hierarchy>();
    const Entity selected_entity = hierarchy.get_first_selected_entity();

    AllComponents::for_each([selected_entity](auto type_tag) {
        using T = typename decltype(type_tag)::type;  // Extract type from tag

        const bool has_component = tmt::engine.ecs.has_component<T>(selected_entity);
        if (has_component) {
            const auto& name = typeid(T).name();
            if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen)) {
                T& component_instance = tmt::engine.ecs.get_component<T>(selected_entity);
                ImReflect::Input("", component_instance);
            }
        }
    });
}

void Inspector::on_editor_start() {}

void Inspector::on_editor_update(const tmt::FrameData& time) {}

void Inspector::on_editor_end() {}

}  // namespace tmt