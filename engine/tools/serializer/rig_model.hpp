#pragma once

#include "engine/systems/animation/rig_model.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RigModel& value) {
    JsonReflect::Detail::from_json_visitable(j, value);

    if (value.data == nullptr) return;

    const auto entity = tmt::engine.ecs.get_entity(value);
    value.armature_entity = entity;

    // Reload to get the animations from the separate animation files.
    value.data->reload();
}