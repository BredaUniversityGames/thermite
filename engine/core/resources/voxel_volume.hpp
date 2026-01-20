#pragma once

#include <graphite/resources/handle.hh>

#include "voxel_scene.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

/* Voxel volume run-time resource, created from a Voxel model resource. */
class VoxelVolume : public tmt::RuntimeResource<VoxelScene> {
   public:
    VoxelVolume(const std::shared_ptr<VoxelScene>& file_resource) : RuntimeResource<VoxelScene>(file_resource) {}
    ~VoxelVolume() { unload(); }

    bool load() override;
    void unload() override;

    /* Update the GPU data of the voxel volume. */
    void update();

    /* 128 bit unique identifier. */
    uint64_t uuid[2] {};

    /* Voxel acceleration structure. */
    uint32_t voxel_buffer_size = 0u;
    uint32_t nodes_buffer_size = 0u;
    std::unique_ptr<Svt64> blas {};
    glm::uvec3 size {};

    /* Voxel acceleration structure buffers. */
    Buffer blas_nodes {}, blas_voxels {}, blas_palette {};
};

}  // namespace tmt

TMT_OBJECT(tmt::VoxelVolume, (size));