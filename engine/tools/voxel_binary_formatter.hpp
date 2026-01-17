#pragma once

#include "engine/core/resource.hpp"

#include <vector>

namespace tmt {

struct VoxelSceneNode;
class VoxelScene;

bool load_voxel_scene(const std::vector<char>& data, VoxelSceneNode& hierarchy);
std::vector<char> save_voxel_scene(const VoxelScene& scene);

}  // namespace tmt