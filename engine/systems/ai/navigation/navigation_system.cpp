#include "navigation_system.hpp"

#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/input/input.hpp"

std::string tmt::NavigationSystem::get_name() { return std::string(); }

void tmt::NavigationSystem::on_start() {}

void tmt::NavigationSystem::on_update(const FrameData& time) {
    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(1.0f);
    for (const auto& [entity, nav_mesh] : engine.ecs.get_registry().view<NavMesh>().each()) {
        glm::mat4 world_matrix = engine.ecs.get_component<Transform>(entity).get_world_matrix();
        for (auto& node : nav_mesh.nodes) {
            node.world_pos = glm::vec3(world_matrix * glm::vec4(node.local_pos, 1.0f));
        }

        nav_mesh.inspect();

        for (int i = 0; i < (int)nav_mesh.path.size() - 1; i++) {
            tmt::engine.polyline.draw_line(nav_mesh.nodes[nav_mesh.path[i]].world_pos, nav_mesh.nodes[nav_mesh.path[i + 1]].world_pos);
        }

        Input& input = tmt::engine.input;
        if (input.is_keyboard_button_pressed(Key::P)) {
            int start = rand() % nav_mesh.nodes.size();
            int end = rand() % nav_mesh.nodes.size();
            nav_mesh.path = nav_mesh.find_path(start, end);
        }
    }
}

void tmt::NavigationSystem::on_end() {}
