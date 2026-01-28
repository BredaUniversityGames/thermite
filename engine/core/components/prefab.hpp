#pragma once
#include "engine/core/io.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/reflection.hpp"
#include "engine/core/components/entity_id.hpp"

namespace tmt {

using PrefabInstanceID = UUID;

struct Prefab {
    IO::FileLocation source_location;
    EntityID source_entity;
    EntityID root_entity;
    PrefabInstanceID instance_id;
};

}  // namespace tmt

TMT_COMPONENT(tmt::Prefab, "Prefab", (source_location, source_entity, root_entity, instance_id));