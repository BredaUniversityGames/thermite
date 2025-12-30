#pragma once
#include <memory>
#include <type_traits>
#include <typeindex>

#include "engine/core/ecs.hpp"
#include "engine/core/game/types.hpp"
#include "engine/core/game/game_component.hpp"
#include "engine/engine.hpp"
#include "engine/core/game/game_component_registry.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {
/* Attached to an entity */
class ComponentCollection {
   public:
    ComponentCollection() = default;
    ~ComponentCollection() = default;

    ComponentCollection(const ComponentCollection&) = delete;
    ComponentCollection& operator=(const ComponentCollection&) = delete;

    ComponentCollection(ComponentCollection&&) = default;
    ComponentCollection& operator=(ComponentCollection&&) = default;

    /* Compile time */
    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    T& add_component() {
        const ComponentIndex type_id = typeid(T);
        components[type_id] = std::make_unique<T>();
        return static_cast<T&>(*components[type_id]);
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    T& get_component() {
        const ComponentIndex type_id = typeid(T);
        return static_cast<T&>(*components.at(type_id));
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    void remove_component() {
        const ComponentIndex type_id = typeid(T);
        components.erase(type_id);
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    bool has_component() const {
        const ComponentIndex type_id = typeid(T);
        return components.contains(type_id);
    }

    /* Runtime */
    IGameComponent& add_component(const ComponentIndex& type_id) {
        auto info = engine.component_registry.get_component_info(type_id);
        components[type_id] = info.factory.create();
        return *components[type_id];
    }

    IGameComponent& get_component(const ComponentIndex& type_id) { return *components.at(type_id); }

    void remove_component(const ComponentIndex& type_id) { components.erase(type_id); }

    bool has_component(const ComponentIndex& type_id) const { return components.find(type_id) != components.end(); }

    const std::unordered_map<ComponentIndex, std::unique_ptr<IGameComponent>>& get_all_components() const { return components; }

   private:
    BEFRIEND_VISITABLE();
    std::unordered_map<ComponentIndex, std::unique_ptr<IGameComponent>> components;
};
}  // namespace tmt

TMT_COMPONENT(tmt::ComponentCollection, "ComponentCollection", (components));

/* Specialization for IGameComponent derived types */
namespace tmt {
template <typename T>
struct EcsComponentTraits<T, std::enable_if_t<std::is_base_of_v<IGameComponent, T>>> {
    static T& add(Registry& registry, Entity entity) {
        auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        return collection.add_component<T>();
    }
    static void remove(Registry& registry, Entity entity) {
        auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        collection.remove_component<T>();
    }
    static T& get(Registry& registry, Entity entity) {
        auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        return collection.get_component<T>();
    }
    static const T& get(const Registry& registry, Entity entity) {
        const auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        return collection.get_component<T>();
    }
    static bool has(const Registry& registry, Entity entity) {
        const auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        return collection.has_component<T>();
    }
    static T& add_or_get(Registry& registry, Entity entity) {
        auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        if (!collection.has_component<T>()) {
            return collection.add_component<T>();
        }
        return collection.get_component<T>();
    }
    static T* try_get(Registry& registry, Entity entity) {
        auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        if (collection.has_component<T>()) {
            return &collection.get_component<T>();
        }
        return nullptr;
    }
    static const T* try_get(const Registry& registry, Entity entity) {
        const auto& collection = registry.emplace_or_replace<ComponentCollection>(entity);
        if (collection.has_component<T>()) {
            return &collection.get_component<T>();
        }
        return nullptr;
    }
};
}  // namespace tmt