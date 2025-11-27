#pragma once
#include "glm/vec3.hpp"
#include "core/components/transform.hpp"
#include "systems/physics/physics_voxel_data.hpp"
#include "rigidbody.hpp"

namespace tmt {

struct Box {
    glm::vec3 vertex[8] {};
};

class VoxelCollider {
    bool is_dirty = false;
    // std::function<void(SeparationCallbackData)> separation_callback;

   public:
    VoxelCollider() = default;
    // VoxelCollider(const PhysicsVoxelData& voxel_data) : voxels(voxel_data) {
    //     width = voxel_data.get_size().x * VOXEL_SIZE;
    //     height = voxel_data.get_size().y * VOXEL_SIZE;
    //     depth = voxel_data.get_size().z * VOXEL_SIZE;
    // };

    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
    // std::shared_ptr<VoxelInstance> voxel_instance;

    PhysicsVoxelData voxels = {};

    uint32_t get_vox_index(uint32_t x, uint32_t y, uint32_t z) const { return x + (y * voxels.get_size().x) + (z * voxels.get_size().x * voxels.get_size().y); }

    /* Remove the voxels */
    void remove_voxel(const glm::uvec3& pos) { remove_voxel(pos.x, pos.y, pos.z); }
    void remove_voxel(uint32_t x, uint32_t y, uint32_t z) { remove_voxel(get_vox_index(x, y, z)); }
    void remove_voxel(uint32_t index) {
        voxels.set_data()[index].normal_index = 0;
        voxels.set_data()[index].type = PhysicsVoxelType::Empty;
        is_dirty = true;
    }

    bool is_loaded() const { return !voxels.get_data().empty(); }

    // returns the width, height and depth as a glm::vec3
    const glm::vec3 get_dimensions() const { return {width, height, depth}; }

    const std::array<glm::vec3, 2> get_world_aabb(const glm::vec3& position, const glm::quat& rotation) const;
    const std::array<glm::vec3, 2> get_world_aabb_fast(const glm::vec3& position, const glm::quat& rotation) const;

    const std::array<glm::vec3, 3> get_axes(const glm::quat& rotation) const;
    const std::array<glm::vec3, 3> get_axes(Transform& transform) const;
    const std::array<glm::vec3, 3> get_axes(const RigidBody& rb) const;
    const tmt::Box get_local_bounds() const;
    const tmt::Box get_world_bounds(Transform& transform) const;
    const tmt::Box get_world_bounds(const RigidBody& rb) const;
    const std::array<std::pair<glm::vec3, glm::vec3>, 12> get_world_edges(Transform& transform) const;
    const std::array<std::pair<glm::vec3, glm::vec3>, 12> get_world_edges(const RigidBody& rb) const;

    //// Callback on the separation of an object from destruction.
    //// Returns the original entity and the entities that are generated from it.
    // void set_separation_callback(std::function<void(SeparationCallbackData)> callback_function) {
    //     separation_callback = callback_function;
    // };
};
}  // namespace tmt

// VISITABLE_STRUCT(tmt::VoxelCollider, voxel_instance, instance_tag);