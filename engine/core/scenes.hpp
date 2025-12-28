#pragma once
#include <type_traits>
#include <typeindex>

#include "engine/core/scene.hpp"
#include "engine/core/collection.hpp"
#include "engine/core/logger.hpp"

#include "engine/tools/type_factory.hpp"
#include "engine/core/io.hpp"

namespace tmt {

class Scenes {
    using SceneFactory = TypeFactory<std::unique_ptr<SceneBase>>;

    struct SceneInfo {
        const std::string name;
        const IO::FileLocation file_location;

        SceneFactory factory;
    };

    /* empty type_index */
    static inline const std::type_index TYPE_INDEX_NULL = std::type_index(typeid(void));

   public:
    Scenes() = default;
    ~Scenes() = default;

    /* Checks if the scenes need to be switched */
    void update();

    /* Swaps the active scene with the next scene */
    void swap_scenes();

    template <typename T>
        requires std::is_base_of_v<SceneBase, T>
    void register_scene() {
        const auto name = std::string(T::scene_name());
        const auto scene_path = std::filesystem::path(Config::SCENES_FOLDER) / (name + Config::SCENE_EXTENSION);
        SceneInfo info {name, {IO::Location::PROJECT, scene_path}};
        info.factory.register_type<T>();

        const std::type_index type_id = typeid(T);
        registered_scenes.emplace(type_id, std::move(info));

        if (next_scene == TYPE_INDEX_NULL) {
            /* Set the first registered scene as the next scene */
            next_scene = type_id;
        }
    }

    /* Enqueue a new scene to be loaded at the end of the frame */
    template <typename T>
        requires std::is_base_of_v<SceneBase, T>
    void enqueue_scene() {
        const std::type_index type_id = typeid(T);
        enqueue_scene(type_id);
    }

    void enqueue_scene(const std::type_index& type_id) {
        auto it = registered_scenes.find(type_id);
        if (it == registered_scenes.end()) {
            Log::error(Log::Scope::ENGINE, "Scenes::enqueue_scene: Scene not registered");
            return;
        }
        next_scene = type_id;
    }

    /* Immediatly load a new scene */
    template <typename T>
        requires std::is_base_of_v<SceneBase, T>
    void load_scene() {
        enqueue_scene<T>();
        swap_scenes();
    }

    std::unique_ptr<SceneBase>& get_active_scene() { return active_scene; }

    const std::unordered_map<std::type_index, SceneInfo>& get_registered_scenes() const { return registered_scenes; }

    struct Config {
        constexpr static const char* SCENE_EXTENSION = ".scene";
        constexpr static const char* SCENES_FOLDER = "scenes";
    };

   private:
    std::unique_ptr<SceneBase> active_scene = nullptr;

    std::type_index next_scene = TYPE_INDEX_NULL;

    std::unordered_map<std::type_index, SceneInfo> registered_scenes;

    void deserialize_scene(const SceneInfo& info);
};
}  // namespace tmt