#pragma once
#include "glm/fwd.hpp"
#include "engine/core/reflection.hpp"
namespace tmt
{
struct BoneHierarchyRenderer
{
	glm::vec4 color;
	float line_width;
};
struct NoBone {};
}
TMT_COMPONENT(tmt::BoneHierarchyRenderer, "BoneHierarchyRenderer", (color, line_width));