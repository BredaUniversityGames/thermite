#include "navigation_system.hpp"

#include "engine/engine.hpp"
#include "engine/core/renderer/pipelines/debug_pipeline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/input/input.hpp"

std::string tmt::NavigationSystem::get_name() { return std::string(); }

void tmt::NavigationSystem::on_start() { 
	nav_mesh.generate_mesh({tmt::IO::Location::PROJECT, "dragon128.vengi"});
    int start = rand() % nav_mesh.nodes.size();
    int end = rand() % nav_mesh.nodes.size();
    path = nav_mesh.find_path(start, end);
}

void tmt::NavigationSystem::on_update(const FrameData& time) { 
    nav_mesh.inspect(); 
    
    for (int i = 0; i < (int)path.size() - 1; i++) {
        tmt::engine.renderer.draw_line(path[i], path[i + 1], glm::vec3(1.0f, 0.0f, 1.0f));
    }

    Input& input = tmt::engine.input;
    if (input.is_keyboard_button_pressed(Key::P)) {
        int start = rand() % nav_mesh.nodes.size();
        int end = rand() % nav_mesh.nodes.size();
        path = nav_mesh.find_path(start, end);
    }
}

void tmt::NavigationSystem::on_end() {}