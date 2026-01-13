#pragma once

#include "core/resource.hpp"

#include <vector>

namespace tmt {
struct VoxelSceneNode;
class VoxelScene;

VoxelSceneNode load_trees(const std::vector<uint8_t>& data);
std::vector<uint8_t> save_trees(const ResourceRef<VoxelScene>& scene);
}  // namespace tmt