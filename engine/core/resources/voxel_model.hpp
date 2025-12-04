#pragma once

#include "engine/core/resource.hpp"

namespace tmt {

/* Voxel model resource, loaded from a `.vengi` file. */
class VoxelModel : public tmt::FileResource {
   public:
    VoxelModel(tmt::IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;
};

}  // namespace tmt
