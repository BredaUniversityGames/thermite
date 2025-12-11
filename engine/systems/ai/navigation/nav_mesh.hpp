#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
namespace tmt {

enum VoxelType : uint8_t { Empty, Corner, Edge, Face, Inside };

struct NavVoxel {
    VoxelType type = VoxelType::Empty;
    uint8_t normal_index = 0;
};

class Node {
   public:
    glm::vec3 pos;
    int parent = 0;

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

    void generate_mesh(IO::FileLocation location);
    std::vector<glm::vec3> find_path(const int startingNodeID, const int endingNodeID);
    void inspect();
};
}  // namespace tmt