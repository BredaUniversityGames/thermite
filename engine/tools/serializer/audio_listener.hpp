#pragma once

#include "engine/tools/serializer.hpp"
#include "engine/core/components/audio_listener.hpp"

inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::AudioListener& audio_listener) {
    float weight = 1.0f;
    tmt::Serializer::deserialize<float>(j["cached_weight"], weight);

    audio_listener.set_weight(weight);
}