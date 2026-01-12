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
}
TMT_COMPONENT(tmt::BoneHierarchyRenderer, "BoneHierarchyRenderer", (color, line_width));