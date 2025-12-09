#include "voxel_scene.hpp"

#include <graphite/vram_bank.hh>

#include "engine/tools/vengi_parser.hpp"
#include "engine/core/logger.hpp"
#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

VoxelModel load_hierarchy(vengi::Node* node) {
    Log::info("{} ({}{})", node->name, node->uuid[0], node->uuid[1]);

    VoxelModel model {};
    model.uuid[0] = node->uuid[0];
    model.uuid[1] = node->uuid[1];

    if (node->type == vengi::NodeType::MODEL) {
        RawVoxels raw_voxels {};
        const vengi::Region& region = node->voxel_data->region;
        raw_voxels.voxels.resize(region.width() * region.height() * region.depth());
        raw_voxels.w = model.width = region.width();
        raw_voxels.h = model.height = region.height();
        raw_voxels.d = model.depth = region.depth();

        const glm::ivec3 min = region.lower;
        const glm::ivec3 max = region.upper;
        for (int32_t z = min.z; z < max.z; ++z) {
            for (int32_t y = min.y; y < max.y; ++y) {
                for (int32_t x = min.x; x < max.x; ++x) {
                    const uint32_t i = (z - min.z) * region.height() * region.width() + (y - min.y) * region.width() + (x - min.x);
                    const vengi::VoxelInformation& voxel = node->voxel_data->voxels[i];
                    raw_voxels.voxels[i] = voxel.color_index;
                }
            }
        }

        model.tree = std::make_unique<Svt64>();
        model.tree->build(raw_voxels);

        Log::info("{} ({}, {}, {})", node->name, raw_voxels.w, raw_voxels.h, raw_voxels.d);
    }

    /* TODO: Only include Model type nodes */
    /* TODO: Load the voxel palette */
    
    // model.tree.build(raw_voxels);

    /* Process child nodes */
    model.children.reserve(node->children.size());
    for (uint32_t i = 0u; i < node->children.size(); ++i) {
        model.children.push_back(load_hierarchy(node->children[i].get()));
    }

    return model;
}

bool VoxelScene::load() { 
    const std::unique_ptr<vengi::Node> root = VengiParser::load(file_location);

    /* Traverse the vengi file node hierarchy */
    hierarchy = load_hierarchy(root.get());

    return true; 
}

void VoxelScene::unload() {
    hierarchy = {};
}

}  // namespace tmt
