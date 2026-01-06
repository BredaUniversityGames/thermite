#pragma once
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

#include "engine/core/entity.hpp"
#include "engine/core/ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

template <typename... T>
class ComponentDiff : public IUndoRedo {
   public:
    ComponentDiff(const Entity entity) : entity(entity) {}

    void before() { ((serialize_component<T>(before_json)), ...); }

    void after() { ((serialize_component<T>(after_json)), ...); }

    void commit() {
        auto diff = nlohmann::json::diff(before_json, after_json);
        if (diff.empty()) return;

        const std::string names = ((std::string(Component<T>::get_name()) + ", "), ...);
        send_to_manager(std::move(*this), "Modified: " + names);
    }

    void undo() override { ((deserialize_component<T>(before_json)), ...); }

    void redo() override { ((deserialize_component<T>(after_json)), ...); }

   private:
    const Entity entity = entt::null;

    tmt::json before_json;
    tmt::json after_json;

    template <typename U>
    void serialize_component(tmt::json& json_data) const {
        constexpr auto name = Component<U>::get_name();
        const bool has_component = engine.ecs.has_component<U>(entity);
        if (has_component) {
            const U& component = engine.ecs.get_component<U>(entity);
            json_data[Component<U>::get_name()] = Serializer::serialize(component);
        } else {
            json_data[name] = nullptr;
        }
    }

    template <typename U>
    void deserialize_component(const tmt::json& json_data) const {
        constexpr auto name = Component<U>::get_name();
        if (json_data.contains(name)) {
            if (json_data[name].is_null()) {
                if (engine.ecs.has_component<U>(entity)) {
                    engine.ecs.remove_component<U>(entity);
                }
            } else {
                if (engine.ecs.has_component<U>(entity) == false) {
                    engine.ecs.add_component<U>(entity);
                }
                U& component = engine.ecs.get_component<U>(entity);
                Serializer::deserialize(json_data[name], component);
            }
        }
    }

   protected:
    void inspect() override {}
};

}  // namespace tmt