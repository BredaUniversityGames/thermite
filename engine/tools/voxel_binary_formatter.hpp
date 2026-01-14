#pragma once

#include "engine/core/resource.hpp"

#include <vector>

namespace tmt {
struct VoxelSceneNode;
class VoxelScene;

VoxelSceneNode load_voxel_scene(const std::vector<char>& data);
std::vector<uint8_t> save_voxel_scene(const ResourceRef<VoxelScene>& scene);
}  // namespace tmt