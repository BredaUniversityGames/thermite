#pragma once

#include "engine/core/resource.hpp"
#include "engine/shared/svt64.hpp"

namespace tmt {

struct VoxelModel {
    uint64_t uuid[2] {};
    std::unique_ptr<Svt64> tree {};
    std::vector<VoxelModel> children {};
    uint32_t width = 0u, height = 0u, depth = 0u;
};

/* Voxel scene resource, loaded from a `.vengi` file. */
class VoxelScene : public tmt::FileResource {
   public:
    VoxelScene(tmt::IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    VoxelModel hierarchy {};
};

}  // namespace tmt
