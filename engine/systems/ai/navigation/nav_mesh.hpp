#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"

class VoxelVolume;

namespace tmt {

enum VoxelType : uint8_t { Empty, Outside, Inside };

struct NavVoxel {
    VoxelType type = VoxelType::Empty;
};

class Node {
   public:
    glm::vec3 local_pos;
    glm::vec3 world_pos;
    int parent = 0;

    uint32_t voxel_index = 0;

    float g = -1.0f;
    float h = 0.0f;
    int id = -1;

    std::vector<int> connectingNodes;
};

class NavMesh {
   public:
    NavMesh() = default;
    ~NavMesh() = default;

    std::vector<Node> nodes;
    std::vector<int> path;

    void generate_mesh(std::shared_ptr<tmt::VoxelVolume> voxel_volume, int lod_level = 1);
    std::vector<int> find_path(const int startingNodeID, const int endingNodeID);
    void inspect();
};
}  // namespace tmt