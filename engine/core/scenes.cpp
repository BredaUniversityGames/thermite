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
    if (next_scene_type == NULL_SCENE) return;

    swap_scenes();
}

void Scenes::swap_scenes() {
    if (next_scene_type == NULL_SCENE) {
        Log::error(Log::Scope::ENGINE, "[Scenes] swap_scenes: No scene enqueued");
        return;
    }
    if (registered_scenes.contains(next_scene_type) == false) {
        Log::error(Log::Scope::ENGINE, "[Scenes] swap_scenes: Scene not registered");
        return;
    }

    /* Unload */
    if (active_scene) {
        OnPreUnloadScene::dispatch();
        active_scene->on_end();
        active_scene_type = NULL_SCENE;
        engine.ecs.clear();
        OnSceneEnd::dispatch();
    }

    /* Load */
    SceneInfo& info = registered_scenes.at(next_scene_type);
    active_scene = info.factory.create();
    active_scene_type = next_scene_type;
    next_scene_type = NULL_SCENE;

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

void Scenes::serialize_active_scene() {
    if (active_scene_type == NULL_SCENE) {
        Log::error(Log::Scope::ENGINE, "[Scenes] serialize_active_scene: No active scene");
        return;
    }

    const SceneInfo& info = registered_scenes.at(active_scene_type);

    json serialized = Serializer::serialize(engine.ecs);
    IO::write_text_file(info.file_location, serialized.dump(4), true);

    active_scene_json->unload();
    active_scene_json->load();

    OnSceneSerialized::dispatch();
}

void Scenes::deserialize_scene(const SceneInfo& info) {
    if (IO::file_exists(info.file_location) == false) return;

    active_scene_json = engine.resources.load_resource<Json>(info.file_location);
    if (active_scene_json == nullptr) {
        Log::error(Log::Scope::ENGINE, "[Scenes] deserialize_scene: Failed to load active scene JSON");
        return;
    }
    Serializer::deserialize(active_scene_json->get_parsed_json(), tmt::engine.ecs);
}

void Scenes::enqueue_scene(const SceneIndex& type_id) {
    if (registered_scenes.contains(type_id) == false) {
        Log::error(Log::Scope::ENGINE, "[Scenes] Enqueue scene: Scene not registered");
        return;
    }
    next_scene_type = type_id;
}

const Scenes::SceneInfo& Scenes::get_active_scene_info() const {
    if (active_scene_type == NULL_SCENE) {
        Log::error(Log::Scope::ENGINE, "[Scenes] get_active_scene_info: No active scene");
        throw std::runtime_error("No active scene");
    }
    return registered_scenes.at(active_scene_type);
}

}  // namespace tmt
