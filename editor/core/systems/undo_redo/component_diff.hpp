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

    template <typename... Args>
    void take_snapshot(Args&&... args) {
        ((serialize_component<T>(before_json)), ...);
    }

    template <typename... Args>
    void commit(Args&&... args) {
        ((serialize_component<T>(after_json)), ...);
        const std::string names = ((std::string(Component<T>::get_name()) + ", "), ...);

        send_to_manager(std::move(*this), "Modified: " + names);
    }

    void undo() override { ((deserialize_component<T>(before_json)), ...); }

    void redo() override { ((deserialize_component<T>(after_json)), ...); }

   private:
    const Entity entity;

    tmt::json before_json;
    tmt::json after_json;

    template <typename U>
    void serialize_component(tmt::json& json_data) const {
        const U& component = engine.ecs.get_component<U>(entity);
        json_data[Component<U>::get_name()] = Serializer::serialize(component);
    }

    template <typename U>
    void deserialize_component(const tmt::json& json_data) const {
        U& component = engine.ecs.get_component<U>(entity);
        Serializer::deserialize(json_data[Component<U>::get_name()], component);
    }

   protected:
    void inspect() override {}
};

}  // namespace tmt