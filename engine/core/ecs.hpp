#pragma once
#include <set>
#include <memory>
#include <stdexcept>

#include "time_step.hpp"
#include "system.hpp"

#include "events/update.hpp"

namespace tmt {

class Ecs : public OnUpdate, public OnFixedUpdate {
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
        systems.insert(system);
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
    std::set<ISystem*> systems;

    void on_update(const FrameData& time) override;

    void on_fixed_update(const FrameData& time) override;
};

}  // namespace tmt