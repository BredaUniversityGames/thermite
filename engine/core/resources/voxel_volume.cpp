#include "voxel_volume.hpp"

#include <graphite/vram_bank.hh>

#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Recursively find the first model inside a voxel model hierarchy. */
VoxelModel* first_model(VoxelModel* parent) {
    if (parent->tree != nullptr) return parent;
    for (uint32_t i = 0u; i < parent->children.size(); ++i) {
        VoxelModel* model = first_model(&parent->children[i]);
        if (model != nullptr) return model;
    }
    return nullptr;
}

bool VoxelVolume::load() { 
    /* TEMP: It's currently just grabbing the first model node */
    const VoxelModel* model = first_model(&file_resource->hierarchy);

    /* Copy the voxel data from the model */
    tree = std::make_unique<Svt64>(*model->tree.get());
    width = model->width;
    height = model->height;
    depth = model->depth;

    // VRAMBank& bank = engine.renderer.vram_bank();
    // tree_nodes = bank.create_buffer();

    return true; 
}

void VoxelVolume::unload() {
    tree = nullptr;
    width = height = depth = 0u;
}

}  // namespace tmt
