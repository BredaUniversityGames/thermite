#pragma once

#include <ImReflect_macro.hpp>

#include <glm/glm.hpp>

IMGUI_REFLECT(glm::vec2, x, y);
IMGUI_REFLECT(glm::vec3, x, y, z);
IMGUI_REFLECT(glm::vec4, x, y, z, w);
IMGUI_REFLECT(glm::quat, x, y, z, w);
IMGUI_REFLECT(glm::ivec2, x, y);
IMGUI_REFLECT(glm::ivec3, x, y, z);
IMGUI_REFLECT(glm::ivec4, x, y, z, w);
IMGUI_REFLECT(glm::uvec2, x, y);
IMGUI_REFLECT(glm::uvec3, x, y, z);
IMGUI_REFLECT(glm::uvec4, x, y, z, w);