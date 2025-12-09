#pragma once

#include <memory>

#include "engine/core/resources/voxel_volume.hpp"

namespace tmt {

/* Voxel Renderer component used to visualize voxel data. */
struct VoxelRenderer {
    /* Size of the voxel data. (TODO: move this into the run-time resource) */
    glm::uvec3 size {};

    std::shared_ptr<VoxelVolume> resource {};
};

}  // namespace tmt
