#pragma once

#include "engine/engine.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources.hpp"
#include "engine/tools/serializer.hpp"

template <typename T>
    requires std::derived_from<T, tmt::FileResource>
tmt::json tag_invoke(JsonReflect::serialize_t, const T& value) {
    // Serialize the file location
    return JsonReflect::to_json(value.file_location);
}

template <typename T>
    requires std::derived_from<T, tmt::FileResource>
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, T& value) {
    // Deserialize the file location
    auto& mutable_file_location = const_cast<tmt::IO::FileLocation&>(value.file_location);
    JsonReflect::from_json(j.at("file_location"), mutable_file_location);
}

template <typename T>
    requires std::derived_from<T, tmt::FileResource>
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::RuntimeResource<T>& value) {
    // Serialize the underlying file resource
    return JsonReflect::to_json(*value.file_resource);
}

template <typename T>
    requires std::derived_from<T, tmt::FileResource>
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RuntimeResource<T>& value) {
    JsonReflect::from_json(j, *value.file_resource);
}

template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::ResourceRef<T>& value) {
    JsonReflect::json j;
    j["file_location"] = JsonReflect::to_json(value.file_location);
    return j;
}
template <typename T>
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::ResourceRef<T>& value) {
    JsonReflect::from_json(j.at("file_location"), value.file_location);
    if constexpr (std::derived_from<T, tmt::FileResource>) {
        value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;
    } else if constexpr (requires { typename T::is_runtime_resource; }) {
        value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;
    }
}