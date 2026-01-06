#pragma once

#include "engine/core/components/transform.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Transform& value) {
    JsonReflect::Detail::from_json_visitable(j, value);
    value.mark_dirty();
}