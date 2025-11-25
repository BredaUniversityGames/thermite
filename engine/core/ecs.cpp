#include "ecs.hpp"

namespace tmt {

void Ecs::on_update(const FrameData& time) {
    for (auto* system : systems) {
        system->on_update(time);
    }
}

void Ecs::on_fixed_update(const FrameData& time) {
    for (auto* system : systems) {
        system->on_fixed_update(time);
    }
}

}  // namespace tmt
