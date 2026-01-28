#pragma once
#include "engine/core/io.hpp"
#include "engine/core/entity.hpp"

namespace tmt {
class PrefabHelper {
   public:
    static void create_prefab(const IO::FileLocation& location, const Entity& root);
};
}  // namespace tmt