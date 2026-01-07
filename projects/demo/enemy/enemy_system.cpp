#include "enemy_system.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/input/input.hpp"

#include "engine/engine.hpp"

std::string tmt::EnemySystem::get_name() { return std::string(); }

void tmt::EnemySystem::on_start() {}

void tmt::EnemySystem::on_update(const FrameData& time) { 
	for (const auto& [entity, enemy] : engine.ecs.get_registry().view<Enemy>().each()) {
        Input& input = tmt::engine.input;
        if (input.is_keyboard_button_pressed(Key::P)) {
            //int start = rand() % nav_mesh.nodes.size();
            //int end = rand() % nav_mesh.nodes.size();
            //nav_mesh.path = nav_mesh.find_path(start, end);
        }
    }
}

void tmt::EnemySystem::on_end() {}
