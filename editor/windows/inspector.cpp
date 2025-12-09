#include "inspector.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "editor/editor.hpp"
#include "editor/windows/hierarchy.hpp"
#include "engine/core/system_collection.hpp"

#include "engine/core/components/all.hpp"

namespace tmt {

void Inspector::display() {
    const auto& hierarchy = editor.windows.get<Hierarchy>();
    const Entity selected_entity = hierarchy.get_first_selected_entity();

    AllComponents::for_each([selected_entity](auto component) {
        using T = std::decay_t<decltype(component)>;
        const bool has_component = tmt::engine.ecs.has_component<T>(selected_entity);
        if (has_component) {
            const auto& name = typeid(T).name();
            if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen)) {
            }
        }
    });
}

void Inspector::on_editor_start() {}

void Inspector::on_editor_update(const tmt::FrameData& time) {}

void Inspector::on_editor_end() {}

}  // namespace tmt