#pragma once
#include <glm/glm.hpp>
#include "engine/core/component.hpp"

namespace tmt {

/* Voxel Renderer component used to visualize voxel data. */
struct VoxelRenderer {
    /* Size of the voxel data. (TODO: move this into the run-time resource) */
    glm::uvec3 size {};
};

}  // namespace tmt

TMT_COMPONENT(tmt::VoxelRenderer, "Voxel Renderer", (size));