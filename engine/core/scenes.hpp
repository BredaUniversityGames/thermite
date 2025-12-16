#pragma once
#include "engine/core/scene.hpp"
#include "engine/core/collection.hpp"

#include "engine/events/engine.hpp"

#include <functional>
#include <type_traits>

namespace tmt {

template <typename T>
class TypeFactory {
   public:
    using Type = T;

    template <typename U>
    void register_type() {
        factory = []() { return U(); };
    }

    T create() { return factory(); }

   private:
    std::function<T()> factory;
};

class Scenes : public OnEndFrame {
    using SceneFactory = TypeFactory<std::unique_ptr<Scene>>;

    struct SceneInfo {
        std::string name;
        SceneFactory factory;
    };

   public:
    Scenes() = default;
    ~Scenes() = default;

    /* Checks if the scenes need to be switched */
    void update() override;

    /* Swaps the active scene with the next scene */
    void swap_scenes();

    template <typename T>
        requires std::is_base_of_v<Scene, T>
    void register_scene(const std::string& name) {
        SceneInfo info;
        info.name = name;
        info.factory.register_type<T>();
        registered_scenes[typeid(T)] = std::move(info);
    }

    template <typename T>
        requires std::is_base_of_v<Scene, T>
    void enqueue_scene() {
        auto it = registered_scenes.find(typeid(T));
        if (it == registered_scenes.end()) {
            throw std::runtime_error("Scene not registered");
        }
        next_scene = &it->second;
    }

    std::unique_ptr<Scene>& get_active_scene() { return active_scene; }

   private:
    std::unique_ptr<Scene> active_scene = nullptr;

    SceneInfo* next_scene = nullptr;

    std::unordered_map<std::type_info, SceneInfo> registered_scenes;
};
}  // namespace tmt