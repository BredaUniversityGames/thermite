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
    size = glm::uvec3(model->width, model->height, model->depth);

    /* Create GPU buffers for the voxel data */
    /* TODO: These buffers need to scale when the voxel data is modified at run-time! */
    VRAMBank& bank = engine.renderer.vram_bank();
    const BufferUsage usage = BufferUsage::Storage | BufferUsage::TransferDst;
    tree_nodes = bank.create_buffer(usage, tree->node_count, sizeof(Svt64Node)).expect("failed to create tree nodes buffer.");
    tree_voxels = bank.create_buffer(usage, tree->voxel_count, sizeof(MaterialIndex)).expect("failed to create voxel data buffer.");

    /* Upload the voxel data into the GPU buffers */
    /* TODO: These buffers need to be updated when voxel data is modified at run-time! */
    bank.upload_buffer(tree_nodes, tree->nodes, 0u, tree->node_count * sizeof(Svt64Node));
    bank.upload_buffer(tree_voxels, tree->voxels, 0u, tree->voxel_count * sizeof(MaterialIndex));

    return true; 
}

void VoxelVolume::unload() {
    /* Destroy the GPU buffers */
    VRAMBank& bank = engine.renderer.vram_bank();
    bank.destroy(tree_nodes);
    bank.destroy(tree_voxels);

    tree = nullptr;
    size = glm::uvec3(0u);
}

}  // namespace tmt
