#pragma once
#include "collision.hpp"


namespace tmt {
class ConstraintSolver {
   public:
    ConstraintSolver() { collisions.resize(10000); };

    void solve_velocities(const float time_step);
    void solve_positions(const float time_step);

    std::atomic<size_t> contact_index = 0;
    std::vector<Collision> collisions = {};
    int velocity_iterations = 16;
    int position_iterations = 4;
};
}  // namespace tmt
