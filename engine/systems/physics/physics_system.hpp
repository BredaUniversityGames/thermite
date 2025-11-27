#pragma once
#include "core/system.hpp"
#include "core/components/transform.hpp"

#include "collision.hpp"
#include "constraint_solver.hpp"
#include "components/rigidbody.hpp"
#include "components/voxel_collider.hpp"

namespace tmt {

class Physics : public ISystem {
   public:
    Physics() = default;
    // Inherited via ISystem
    std::string get_name() override { return "Physics System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

   private:
    float simulation_time_accum = 0.0f;
    const int max_contacts = 256;
    bool simulating = true;
    ConstraintSolver solver = {};

    void apply_velocities() const;

    bool is_colliding(Transform& transform_a, const VoxelCollider& vox_a, Transform& transform_b);

    void generate_voxel_constraints_range(const int index, const int size);

    bool sat_early_out(const RigidBody& rb_a, const VoxelCollider& vox_a, const RigidBody& rb_b, const VoxelCollider& vox_b);

    void get_overlap(const RigidBody& rb_a, const VoxelCollider& vox_a, const RigidBody& rb_b, const VoxelCollider& vox_b, glm::ivec3& min, glm::ivec3& max);

    bool is_separated(const glm::vec3& axis, const Box& vertices_a, const Box& vertices_b, float& overlap) const;

    void check_neighbors(const VoxelCollider& vox_a, const VoxelCollider& vox_b, const RigidBody& rb_a, const RigidBody& rb_b, const size_t x, const size_t y, const size_t z, Collision& coll)
        const;
};

}  // namespace tmt
