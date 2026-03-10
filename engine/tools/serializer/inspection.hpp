#pragma once
#include <ImReflect_macro.hpp>
#include "engine/tools/serializer.hpp"

namespace tmt {

/* Act as a tag */
struct InspectOnly {};

inline constexpr InspectOnly inspect_only {};

}  // namespace tmt

template <typename T>
requires visit_struct::traits::is_visitable<T, ImReflect::Detail::ImContext>::value
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, T& value, tmt::InspectOnly) {
    /* Deserialize only the fields that are present in the inspector */
    JsonReflect::Detail::from_json_visitable<ImReflect::Detail::ImContext>(j, value);
}