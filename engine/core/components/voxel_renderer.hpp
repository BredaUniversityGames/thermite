#pragma once
#include <glm/glm.hpp>
#include "engine/core/component.hpp"

#include <memory>

#include "engine/core/resources/voxel_volume.hpp"

namespace tmt {

/* Voxel Renderer component used to visualize voxel data. */
struct VoxelRenderer {
    std::shared_ptr<VoxelVolume> resource {};
};

}  // namespace tmt

TMT_COMPONENT(tmt::VoxelRenderer, "Voxel Renderer", (size));