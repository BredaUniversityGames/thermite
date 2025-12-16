#pragma once
#include <string>

#include "engine/events/game.hpp"

namespace tmt {
class Scene : public IGameEvents {
   public:
    Scene() = default;
    virtual ~Scene() = default;

    constexpr virtual std::string get_name() const = 0;
};
}  // namespace tmt