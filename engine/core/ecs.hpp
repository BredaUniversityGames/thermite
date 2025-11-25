#pragma once
#include <set>
#include <memory>
#include <stdexcept>

#include "time_step.hpp"
#include "system.hpp"

#include "events/game.hpp"

namespace tmt {

class Ecs : public OnStart,
            public OnUpdate,
            public OnFixedUpdate,
            public OnEnd {
   public:
    Ecs() = default;
    ~Ecs() {
        /* CLean up all systems */
        for (auto* system : systems) {
            delete system;
        }
    }

    template <typename T, typename... Args>
    T& register_system(Args&&... args) {
        static_assert(
            std::is_base_of_v<ISystem, T>, "T must inherit from ISystem"
        );

        T* system = new T(std::forward<Args>(args)...);

        for (auto* existing_system : systems) {
            if (typeid(*existing_system) == typeid(T)) {
                throw std::runtime_error("System already registered");
            }
        }

        systems.push_back(system);
        return *system;
    }

    template <typename T>
    T& get_system() {
        for (auto* system : systems) {
            if (T* casted = dynamic_cast<T*>(system)) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T>
    T* try_get_system() {
        for (auto* system : systems) {
            if (T* casted = dynamic_cast<T*>(system)) {
                return casted;
            }
        }
        return nullptr;
    }

   private:
    std::vector<ISystem*> systems;

    void on_start() override;

    void on_update(const FrameData& time) override;

    void on_fixed_update(const FrameData& time) override;

    void on_end() override;
};

}  // namespace tmt