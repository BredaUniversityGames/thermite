#pragma once
#include "engine/core/reflection.hpp"
#include "engine/tools/serializer/glm.hpp"

namespace tmt {

struct EntityLink {
    EntityLink() = default;

    Entity connected_entity;
    glm::uvec3 connection_voxel = glm::uvec3(0);
};

struct VoxelConnection {
    std::vector<EntityLink> connections;
};

}  // namespace tmt
TMT_OBJECT(tmt::EntityLink, (connected_entity, connection_voxel));
TMT_COMPONENT(tmt::VoxelConnection, "VoxelConnection", (connections));