#include "voxel_model.hpp"

#include "engine/tools/vengi_parser.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

VoxelModel load_hierarchy(vengi::Node* node) {
    Log::info("{} ({}{})", node->name, node->uuid[0], node->uuid[1]);

    VoxelModel model {};
    model.uuid[0] = node->uuid[0];
    model.uuid[1] = node->uuid[1];

    // RawVoxels raw_voxels {};
    // const vengi::Region& region = node->voxel_data->region;
    // raw_voxels.voxels.resize(region.width() * region.height() * region.depth());
    // raw_voxels.w = region.width();
    // raw_voxels.h = region.height();
    // raw_voxels.d = region.depth();
    // const glm::ivec3 min = region.lower;
    // for (int32_t z = 0; z < region.depth(); ++z) {
    //     for (int32_t y = 0; y < region.height(); ++y) {
    //         for (int32_t x = 0; x < region.width(); ++x) {
    //             const uint32_t i = (z - min.z) * region.height() * region.width() + (y - min.y) * region.width() + (x - min.x);
    //             const vengi::VoxelInformation& voxel = node->voxel_data->voxels[i];
    //             raw_voxels.voxels[i] = voxel.color_index;
    //         }
    //     }
    // }

    /* TODO: Only include Model type nodes */
    /* TODO: Load the voxel palette */
    
    // model.tree.build(raw_voxels);

    /* Process child nodes */
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

void VoxelScene::unload() {}

}  // namespace tmt
