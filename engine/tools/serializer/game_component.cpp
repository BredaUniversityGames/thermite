#include "game_component.hpp"
#include <nlohmann/json.hpp>
#include "engine/systems/gameplay/game_component.hpp"

JsonReflect::json tag_invoke(JsonReflect::serialize_lib_t, const tmt::IGameComponent& value) {
    //
    return value.serialize();
}

void tag_invoke(JsonReflect::deserialize_lib_t, const JsonReflect::json& j, tmt::IGameComponent& value) {
    //
    value.deserialize(j);
}