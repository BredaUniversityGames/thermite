#pragma once
#include <string>
#include <set>
#include <memory>
#include <stdexcept>

#include "time_step.hpp"

namespace tmt {
class System {
   public:
    /* [Required] */
    constexpr virtual std::string get_name() = 0;

    /* Events */
    /* [Required] */
    virtual void on_start() = 0;
    virtual void on_update(const TimeStep& time) = 0;
    virtual void on_end() = 0;
    /* [Optional] */
    virtual void on_fixed_update(const TimeStep& time) {};
};

class Ecs {
   public:
    Ecs();
    ~Ecs();

    template <typename T, typename... Args>
    T& register_system(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        systems.insert(std::move(system));
        return ref;
    }

    template <typename T>
    T& get_system() {
        for (const auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T>
    T* try_get_system() {
        for (const auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    std::set<std::unique_ptr<System>> systems;
};

}  // namespace tmt