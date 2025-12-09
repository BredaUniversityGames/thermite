#pragma once
#include <entt/entt.hpp>
#include <ImReflect.hpp>

void tag_invoke(ImReflect::ImInput_t, const char* label, entt::entity& value, ImSettings& settings, ImResponse& response) {
    /* Cast to uint32_t */
    //ImReflect::Input(label, static_cast<uint32_t>(value), settings, response);
    uint32_t temp = static_cast<uint32_t>(value);
    ImReflect::Input(label, temp, settings, response);
    value = static_cast<entt::entity>(temp);
}

void tag_invoke(ImReflect::ImInput_t, const char* label, const entt::entity& value, ImSettings& settings, ImResponse& response) {
    /* Cast to uint32_t */
    const uint32_t temp = static_cast<uint32_t>(value);
    ImReflect::Input(label, temp, settings, response);
    /* value is const, can not be modified */
}