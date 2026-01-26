#include "voxel_volume.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>

#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/tools/serializer/uuid.hpp"

namespace tmt {

namespace {

/* Recursively find the first model inside a voxel model hierarchy. */
const VoxelSceneNode* first_model(const VoxelSceneNode& parent) {
    if (parent.tree != nullptr) return &parent;

    for (const VoxelSceneNode& child : parent.children) {
        const VoxelSceneNode* model = first_model(child);
        if (model != nullptr) return model;
    }
    return nullptr;
}

const VoxelSceneNode* find_model_by_uuid(const VoxelSceneNode& node, const UUID& uuid) {
    if (node.uuid == uuid) return &node;

    for (const VoxelSceneNode& child : node.children) {
        const VoxelSceneNode* return_node = find_model_by_uuid(child, uuid);
        if (return_node != nullptr) return return_node;
    }

    return nullptr;
}

}  // namespace

VoxelVolume::VoxelVolume(const VoxelSceneNode& node) : RuntimeResource<VoxelScene>({}) { 
    load_from_node(node); 
}

bool VoxelVolume::load() {
    const VoxelSceneNode* model;
    if (uuid == NULL_UUID)
        model = first_model(file_resource->hierarchy);
    else
        model = find_model_by_uuid(file_resource->hierarchy, uuid);

    if (model == nullptr) {
        Log::error("Failed to get model for VoxelVolume.");
        return false;
    }

    load_from_node(*model);

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

void VoxelVolume::update_if_dirty() {
    if (is_dirty == false) return;

    /* Mark this volume as no longer dirty */
    is_dirty = false;

    static bool logged = false;
    if (blas->voxel_count >= blas_voxels_capacity || blas->node_count >= blas_nodes_capacity) {
        if (logged == false) {
            Log::warn(Log::Scope::ENGINE, "Ran out of SVT64 buffer space.");
            logged = true;
        }
        return;
    }

    /* Re-upload voxel data */
    VRAMBank& bank = engine.renderer.vram_bank();
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, sizeof(MaterialPalette));
}

void VoxelVolume::load_from_node(const VoxelSceneNode& node) {
    /* Copy the voxel data from the model */
    blas = std::make_unique<Svt64>(*node.tree);
    size = node.size;
    uuid = node.uuid;

    /* Create GPU buffers for the voxel data */
    /* TODO: These buffers need to scale when the voxel data is modified at run-time! */
    VRAMBank& bank = engine.renderer.vram_bank();
    const BufferUsage storage = BufferUsage::Storage | BufferUsage::TransferDst;
    blas_nodes_capacity = blas->node_count + SVT64_BUFFER_MEMORY;
    blas_voxels_capacity = blas->voxel_count + SVT64_BUFFER_MEMORY;
    blas_nodes = bank.create_buffer("BLAS Nodes Buffer", storage, blas_nodes_capacity, sizeof(Svt64Node)).expect("failed to create tree nodes buffer.");
    blas_voxels = bank.create_buffer("BLAS Voxels Buffer", storage, blas_voxels_capacity, sizeof(MaterialIndex)).expect("failed to create voxel data buffer.");
    blas_palette = bank.create_buffer("BLAS Palette Buffer", storage, sizeof(MaterialPalette)).expect("failed to create material palette buffer.");

    /* Upload the voxel data into the GPU buffers */
    /* TODO: These buffers need to be updated when voxel data is modified at run-time! */
    bank.upload_buffer(blas_nodes, blas->nodes, 0u, blas->node_count * sizeof(Svt64Node));
    bank.upload_buffer(blas_voxels, blas->materials, 0u, blas->voxel_count * sizeof(MaterialIndex));
    bank.upload_buffer(blas_palette, &blas->palette, 0u, sizeof(MaterialPalette));
}

}  // namespace tmt
