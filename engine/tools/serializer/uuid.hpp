#pragma once
#include <JsonReflect.hpp>
#include "engine/tools/uuid.hpp"
#include "engine/core/components/entity_id.hpp"

inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::UUID& uuid) {
    //
    return uuid.str();
}

inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::UUID& uuid) {
    //
    uuid = tmt::UUID::fromStrFactory(j.get<std::string>());
}

inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::EntityID& component) {
    //
    return component.str();
}

inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::EntityID& component) {
    //
    component = tmt::EntityID(j.get<std::string>());
}