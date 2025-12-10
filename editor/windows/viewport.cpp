#include "viewport.hpp"
#include <imgui.h>
#include "ImGuizmo.h"
#include "editor/editor.hpp"
#include "hierarchy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/renderer/renderer.hpp"

void tmt::Viewport::on_editor_start() 
{
	ImGuizmo::AllowAxisFlip(false);
}

void tmt::Viewport::on_editor_update(const tmt::FrameData&) 
{
	
}

void tmt::Viewport::on_editor_end() {}

void tmt::Viewport::display() 
{
    uint32_t flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;
	ImGui::Begin(get_title().c_str(), nullptr, flags);
	
	if(ImGui::Button("switch space"))
	{
		gizmo_space = static_cast<uint8_t>(!gizmo_space);
	}
	if(ImGui::Button("switch mode"))
	{
		++gizmo_operation;
		if(gizmo_operation == GIZMO_OP_COUNT) gizmo_operation = 0;
	}

	auto vec2 = ImGui::GetContentRegionAvail();
	width = vec2.x;
	height = vec2.y;

	auto window_pos = ImGui::GetCursorScreenPos();

	ImGuizmo::SetRect(window_pos.x, window_pos.y, width, height);

	ImGuizmo::SetDrawlist();

	engine.renderer.set_viewport_size(width, height);

    ImGui::Image((ImTextureRef)engine.renderer.get_imgui_viewport(), vec2);

	gizmo_manip();

	ImGui::End();
}

void tmt::Viewport::gizmo_manip() 
{
    auto& hierarchy = editor.windows.get<Hierarchy>();

    if (hierarchy.is_entity_selected()) {
        Entity cam_entity = Camera::get_active_camera();
        if (cam_entity == entt::null) return;

        Transform& transform = engine.ecs.get_component<Transform>(cam_entity);
        Camera& camera = engine.ecs.get_component<Camera>(cam_entity);
        auto view = glm::inverse(transform.get_world_matrix());

        const float aspect_ratio = static_cast<float>(engine.window.width) / static_cast<float>(engine.window.height);
        auto perspective = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);

        Entity selected_entity = hierarchy.get_first_selected_entity();
        Transform& selected_transform = engine.ecs.get_component<Transform>(selected_entity);
        glm::mat4 selected_matrix = selected_transform.get_world_matrix();

        ImGuizmo::Manipulate(
            &view[0][0], &perspective[0][0], static_cast<ImGuizmo::OPERATION>(gizmo_operations[gizmo_operation]), static_cast<ImGuizmo::MODE>(gizmo_space), &selected_matrix[0][0]
        );
        selected_transform.set_world_matrix(selected_matrix);
    }
}
