#pragma once

#include "engine/systems/animation/rig_model.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RigModel& value) {
    JsonReflect::Detail::from_json_visitable(j, value);
    if (!value.rig_path.relative_path.empty() && !value.rig_is_loaded) {
        auto entity = tmt::engine.ecs.get_entity(value);
        value.init(entity);
        value.data->reload();
    }

    if (value.rig_is_loaded) {
        if (!value.vox_path.relative_path.empty() && !value.vox_is_loaded) {
            value.attach_voxel_objects();
        }
    }
}