#pragma once
#include <string>

#include "engine/events/game.hpp"

namespace tmt {
class Scene : public IGameEvents {
   public:
    Scene() = default;
    virtual ~Scene() = default;

    /* [Optional] */
    /* Gets called before deserialization */
    virtual void on_pre_load() {};
    /* Gets called after deserialization */
    virtual void on_post_load() {};
};
}  // namespace tmt