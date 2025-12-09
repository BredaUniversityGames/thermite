#pragma once

#include <graphite/resources/handle.hh>

#include "voxel_scene.hpp"

namespace tmt {

/* Voxel volume run-time resource, created from a Voxel model resource. */
class VoxelVolume : public tmt::RuntimeResource<VoxelScene> {
   public:
    VoxelVolume(const std::shared_ptr<VoxelScene>& file_resource) : RuntimeResource<VoxelScene>(file_resource) {}
    ~VoxelVolume() { unload(); }

    bool load() override;
    void unload() override;

    uint64_t uuid[2] {};
    std::unique_ptr<Svt64> tree {};
    glm::uvec3 size {};

    Buffer tree_nodes {};
    Buffer tree_voxels {};
};

}  // namespace tmt
