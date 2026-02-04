#pragma once
#include "engine/core/io.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

using PrefabInstanceID = UUID;

struct Prefab {
    IO::FileLocation source_location {};
    tmt::Entity source_entity = entt::null; /* in Prefab */
    tmt::Entity root_entity = entt::null;   /* in Scene */
    PrefabInstanceID instance_id {NULL_UUID};
};

}  // namespace tmt

TMT_COMPONENT(tmt::Prefab, "Prefab", (source_location, source_entity, root_entity, instance_id));