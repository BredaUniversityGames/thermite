#include "voxel_connection.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/ecs.hpp"
#include "editor/imgui/types/all.hpp"
#include "editor/editor.hpp"
#include "editor/windows/viewport.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/transform.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::VoxelConnection& connection, ImSettings& settings, ImResponse& response) {
    static bool selecting = false;
    static tmt::Entity current_entity = entt::null;

    auto& type_settings = settings.get<tmt::VoxelConnection>();
    auto& type_response = response.get<tmt::VoxelConnection>();

    tmt::Entity self_entity = tmt::engine.ecs.get_entity(connection);

    // Add a new entry to the connections
    if (ImGui::Button("Attatch Entity")) {
        selecting = true;
        current_entity = self_entity;
        //connection.connections.push_back(tmt::EntityLink {});
        //type_response.changed();
    }

    // For all connections in the component
    for (tmt::EntityLink& link : connection.connections) {
        ImGui::PushID(&link);
        ImGui::SeparatorText("Connection");
        ImReflect::Input("Connected Entity", link.connected_entity, type_settings, type_response);
        ImGui::BeginDisabled();
        ImReflect::Input("Connection Voxel", link.connection_voxel, type_settings, type_response);
        ImGui::EndDisabled();
        if (ImGui::Button("Remove")) {
            connection.connections.erase(connection.connections.begin() + (&link - &connection.connections[0]));
            type_response.changed();
        }
        ImGui::PopID();

        if (link.connected_entity == entt::null) continue;

        tmt::VoxelRenderer* curr_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(self_entity);
        tmt::VoxelBody* curr_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(self_entity);
        tmt::Transform* curr_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(self_entity);
        tmt::Transform* other_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(link.connected_entity);

        // Make sure we have all components we need
        if (curr_renderer == nullptr || curr_body == nullptr || curr_transform == nullptr || other_transform == nullptr) return;
        glm::vec3 half_extents = glm::vec3(curr_renderer->resource->size) * 0.5f;
        glm::vec3 local_hit_pos = (glm::vec3(link.connection_voxel) + glm::vec3(0.5f) - half_extents) * UNITS_PER_VOXEL;
        glm::vec3 hit_world_pos = curr_transform->get_world_position() + (curr_transform->get_world_rotation() * local_hit_pos);

        tmt::engine.polyline.use_color(0.0f, 0.0f, 1.0f, 0.5f);
        tmt::engine.polyline.use_line_width(2.0f);
        tmt::engine.polyline.use_depth_testing(false);
        tmt::engine.polyline.draw_obb(hit_world_pos, glm::vec3(VOXEL_SIZE_HALF), curr_transform->get_world_rotation());
        tmt::engine.polyline.draw_line(other_transform->get_world_position(), hit_world_pos);
    }

    // if we are selecting
    if (selecting) {
        auto& viewport = tmt::editor.systems[tmt::editor.editor_mode].get<tmt::Viewport>();
        glm::vec2 mouse_pos = viewport.get_mouse_pos();

        tmt::Ray mouse_ray = tmt::engine.renderer.render_view.pixel_ray({ mouse_pos.x, mouse_pos.y });
        tmt::Hit hit = tmt::engine.renderer.trace_ray(mouse_ray);

        // Skip if we didn't hit anything
        if (hit.miss()) return;

        tmt::VoxelRenderer* renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(hit.entity);
        tmt::VoxelBody* body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(hit.entity);
        tmt::Transform* transform = tmt::engine.ecs.try_get_component<tmt::Transform>(hit.entity);

        // Make sure we have all components we need
        if (renderer == nullptr || body == nullptr || transform == nullptr) return;

        glm::vec3 half_extents = glm::vec3(renderer->resource->size) * 0.5f;
        glm::vec3 local_hit_pos = (glm::vec3(hit.coord) + glm::vec3(0.5f) - half_extents) * UNITS_PER_VOXEL;
        glm::vec3 hit_world_pos = transform->get_world_position() + (transform->get_world_rotation() * local_hit_pos); 

        tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f, 0.5f);
        tmt::engine.polyline.use_line_width(2.0f);
        tmt::engine.polyline.draw_obb(hit_world_pos, glm::vec3(VOXEL_SIZE_HALF + 0.01f), transform->get_world_rotation());

        // When we press left mouse button
        if (!ImGui::IsMouseClicked(0)) return;

        tmt::VoxelConnection* other_connection = tmt::engine.ecs.try_get_component<tmt::VoxelConnection>(hit.entity);

        // Create a connection if it doesn't exist already
        if (other_connection == nullptr) {
            other_connection = &tmt::engine.ecs.add_component<tmt::VoxelConnection>(hit.entity);
        }

        // Add a connection to the current entity from the hit coordinate
        other_connection->connections.emplace_back(tmt::EntityLink(current_entity, hit.coord));

        // Reset selection state
        selecting = false;
        current_entity = entt::null;
    }
}