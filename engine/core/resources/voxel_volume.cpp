#include "voxel_volume.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

/* Recursively find the first model inside a voxel model hierarchy. */
const VoxelSceneNode* first_model(const VoxelSceneNode* parent) {
    if (parent->tree != nullptr) return parent;
    for (uint32_t i = 0u; i < parent->children.size(); ++i) {
        const VoxelSceneNode* model = first_model(&parent->children[i]);
        if (model != nullptr) return model;
    }
    return nullptr;
}

bool VoxelVolume::load() {
    /* TEMP: It's currently just grabbing the first model node */
    const VoxelSceneNode* model = first_model(&file_resource->hierarchy);

    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*model->tree.get());
    size = model->size;
    uuid = model->uuid;

    /* Create GPU buffers for the voxel data */
    VRAMBank& bank = engine.renderer.vram_bank();
    const BufferUsage storage = BufferUsage::Storage | BufferUsage::TransferDst;
    blas_nodes = bank.create_buffer("BLAS Nodes Buffer", storage, blas->node_count + SVT64_BUFFER_MEMORY, sizeof(Svt64Node)).expect("failed to create tree nodes buffer.");
    blas_voxels = bank.create_buffer("BLAS Voxels Buffer", storage, blas->voxel_count + SVT64_BUFFER_MEMORY, sizeof(MaterialIndex)).expect("failed to create voxel data buffer.");
    blas_palette = bank.create_buffer("BLAS Palette Buffer", storage, sizeof(MaterialPalette)).expect("failed to create material palette buffer.");
    voxel_buffer_size = blas->voxel_count + SVT64_BUFFER_MEMORY;
    nodes_buffer_size = blas->node_count + SVT64_BUFFER_MEMORY;

    /* Upload the voxel data into the GPU buffers */
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, sizeof(MaterialPalette));

    return true;
}

void VoxelVolume::unload() {
    /* Destroy the (bindless) GPU buffers */
    engine.renderer.destroy(blas_nodes);
    engine.renderer.destroy(blas_voxels);
    engine.renderer.destroy(blas_palette);

    /* Set the BLAS to null */
    blas = nullptr;
    size = glm::uvec3(0u);
}

void VoxelVolume::update() {
    // VRAMBank& bank = engine.renderer.vram_bank();
    // const BufferUsage storage = BufferUsage::Storage | BufferUsage::TransferDst;

    // /* Check if we need to resize the nodes buffer */
    // if (nodes_buffer_size < blas->node_count) {
    //     // bank.destroy(blas_nodes);
    //     // blas_nodes = bank.create_buffer("BLAS Nodes Buffer", storage, blas->node_count + SVT64_BUFFER_MEMORY, sizeof(Svt64Node)).expect("failed to create tree nodes buffer.");
    //     // bank.resize_buffer(blas_nodes, blas->node_count + SVT64_BUFFER_MEMORY, sizeof(Svt64Node));
    //     nodes_buffer_size = blas->node_count + SVT64_BUFFER_MEMORY;
    // } 

    // /* Check if we need to resize the voxels buffer */
    // if (voxel_buffer_size < blas->voxel_count) {
    //     // bank.destroy(blas_voxels);
    //     // blas_voxels = bank.create_buffer("BLAS Voxels Buffer", storage, blas->voxel_count + SVT64_BUFFER_MEMORY, sizeof(MaterialIndex)).expect("failed to create voxel data buffer.");
    //     // bank.resize_buffer(blas_voxels, blas->voxel_count + SVT64_BUFFER_MEMORY, sizeof(MaterialIndex));
    //     voxel_buffer_size = blas->voxel_count + SVT64_BUFFER_MEMORY;
    // }   

    if (blas->node_count > nodes_buffer_size || blas->voxel_count > voxel_buffer_size) {
        return;
    }

    /* Upload the new BLAS data */
    VRAMBank& bank = engine.renderer.vram_bank();
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, sizeof(MaterialPalette));
}

}  // namespace tmt
