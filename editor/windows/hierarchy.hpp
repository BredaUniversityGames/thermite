#pragma once
#include <entt/entt.hpp>
#include <set>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"

namespace tmt {
class Hierarchy {
   public:
    Hierarchy() = default;
    ~Hierarchy() = default;

    template <typename... Components>
    void display() {
        /* Enforce Transform and Name component */
        auto view = engine.ecs.get_registry().view<Transform, Name, Components...>();

        start_section();

        uint32_t index = 0;
        for (auto [entity, transform, name] : view.each()) {
            HierarchyState state(entity, transform, name);
            state.position = {0, index};

            const bool displayed = display_entity(state);

            if (displayed) index++;
        }

        end_section();
    }

   private:
    std::set<Entity> selected_entities;

    static constexpr glm::uvec2 null_index {std::numeric_limits<uint32_t>::max()};

    glm::uvec2 selected_index_begin = null_index;
    glm::uvec2 selected_index_end = null_index;

    struct HierarchyState {
        HierarchyState(Entity entity, Transform& transform, Name& name) : entity(entity), transform(transform), name(name) {};

        Entity entity = entt::null;
        Transform& transform;
        Name& name;

        /* x = depth/indent, y = row/index */
        glm::uvec2 position;

        uint32_t depth() const { return position.x; }
        uint32_t index() const { return position.y; }
    };

    void start_section();
    bool display_entity(const HierarchyState& state);
    void end_section();

    struct DragNDropPayload {
        bool multiple = false;
        Entity entity = entt::null;
    };

    bool drag_drop_source(const Entity dragged_entity) const;
    bool drag_drop_target(const Entity dropped_entity);
};
}  // namespace tmt