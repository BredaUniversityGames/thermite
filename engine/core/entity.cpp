#include "entity.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

template <typename R>
    requires TypeRange<R, Entity>
Entity EntityHelper::upper_parent(const R& container) {
    Entity current_parent = entt::null;
    for (const auto& entity : container) {
        if (!EntityHelper::is_valid(entity)) continue;
        const auto& transform = engine.ecs.get_component<Transform>(entity);
        Entity parent = transform.get_parent();
        if (!EntityHelper::is_valid(parent)) continue;
        if (!EntityHelper::is_valid(current_parent)) {
            current_parent = parent;
        } else {
            /* Find common ancestor */
            Entity temp_parent = parent;
            while (temp_parent != entt::null) {
                if (temp_parent == current_parent) break;
                const auto& temp_transform = engine.ecs.get_component<Transform>(temp_parent);
                temp_parent = temp_transform.get_parent();
            }
            current_parent = temp_parent;
            if (current_parent == entt::null) break;
        }
    }
    return current_parent;
}

template Entity EntityHelper::upper_parent(const std::vector<Entity>&);
template Entity EntityHelper::upper_parent(const std::set<Entity>&);
template Entity EntityHelper::upper_parent(const std::unordered_set<Entity>&);

}  // namespace tmt