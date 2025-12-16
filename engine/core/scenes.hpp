#pragma once
#include "engine/core/scene.hpp"
#include "engine/core/system_collection.hpp"

namespace tmt {
class Scenes : public SystemCollection<Scene> {
   public:
    Scenes() = default;
    ~Scenes() = default;
};
}  // namespace tmt