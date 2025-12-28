#include "scenes.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/events/scene.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/json.hpp"

#include "engine/tools/serializer/ecs.hpp"

namespace tmt {
void Scenes::update() {
    // next_scene is type index
    if (next_scene == TYPE_INDEX_NULL) {
        return;
    }

    swap_scenes();
}

void Scenes::swap_scenes() {
    if (next_scene == TYPE_INDEX_NULL) {
        Log::error(Log::Scope::ENGINE, "[Scenes] No scene enqueued");
        return;
    }
    if (registered_scenes.contains(next_scene) == false) {
        Log::error(Log::Scope::ENGINE, "[Scenes] Scene not registered");
        return;
    }

    /* Unload */
    if (active_scene) {
        OnPreUnloadScene::dispatch();
        active_scene->on_end();
        engine.ecs.clear();
        OnSceneEnd::dispatch();
    }

    /* Load */
    SceneInfo& info = registered_scenes.at(next_scene);
    active_scene = info.factory.create();
    next_scene = TYPE_INDEX_NULL;

    OnPreLoadScene::dispatch();
    active_scene->on_pre_load();

    /* Deserialize */
    deserialize_scene(info);

    OnPostLoadScene::dispatch();
    active_scene->on_post_load();

    /* Start */
    if (engine.game_controller.is_playing()) {
        active_scene->on_start();
        OnSceneStart::dispatch();
    }
}

void Scenes::deserialize_scene(const SceneInfo& info) {
    const ResourceRef<Json> json = engine.resources.load_resource<Json>(info.file_location);
    Serializer::deserialize(json->get_parsed_json(), tmt::engine.ecs);
}

}  // namespace tmt
