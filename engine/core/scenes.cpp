#include "scenes.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {
void Scenes::update() {
    if (next_scene == nullptr) return;

    swap_scenes();
}
void Scenes::swap_scenes() {
    /* Unload */
    active_scene->on_end();
    engine.ecs.clear();

    /* Load */
    active_scene = next_scene->factory.create();
    next_scene = nullptr;
    active_scene->on_pre_load();
    /* Deserialize */
    {
        // TODO
    }
    active_scene->on_post_load();

    /* Start */
    if (engine.game_controller.is_playing()) {
        active_scene->on_start();
    }
}
}  // namespace tmt
