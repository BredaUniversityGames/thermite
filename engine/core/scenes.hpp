#pragma once
#include "engine/core/scene.hpp"
#include "engine/core/collection.hpp"

namespace tmt {
class Scenes : public Collection<Scene> {
   public:
    Scenes() = default;
    ~Scenes() = default;
};
}  // namespace tmt