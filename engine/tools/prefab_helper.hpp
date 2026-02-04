#pragma once
#include "engine/core/io.hpp"
#include "engine/core/entity.hpp"

namespace tmt {
class PrefabHelper {
   public:
    static void create_prefab(const IO::FileLocation& location, const Entity& root);

    class Config {
       public:
        constexpr static const char* PREFAB_EXTENSION = ".prefab";
    };
};
}  // namespace tmt