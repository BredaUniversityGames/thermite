#include "fabrik_ik.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"

namespace tmt {
namespace AnimConstraints {
namespace FABRIKIK {

bool apply_pole_constraint(const glm::vec3& a, glm::vec3& joint, const glm::vec3& b, float len_a, float len_b, const glm::vec3& pole_dir, float strength = 1.0f) {
    constexpr float eps = 0.000001f;

    glm::vec3 ab = b - a;
    float d = glm::length(ab);

    if (d < eps) return false;

    glm::vec3 axis = ab / d;

    // If the two lengths cannot form a triangle with distance d,
    // the bend circle is invalid or collapsed.
    if (d > len_a + len_b) return false;

    if (d < std::abs(len_a - len_b)) return false;

    // Distance from a to the center of the bend circle along a->b.
    float x = (len_a * len_a - len_b * len_b + d * d) / (2.0f * d);

    float h2 = len_a * len_a - x * x;

    if (h2 <= eps) return false;

    float h = std::sqrt(h2);

    glm::vec3 circle_center = a + axis * x;

    // Desired radial direction from pole.
    glm::vec3 desired_radial = pole_dir - axis * glm::dot(pole_dir, axis);

    if (glm::length2(desired_radial) < eps) return false;

    desired_radial = glm::normalize(desired_radial);

    // Current radial direction. Used for soft pole strength.
    glm::vec3 current_radial = joint - circle_center;
    current_radial -= axis * glm::dot(current_radial, axis);

    if (glm::length2(current_radial) < eps) {
        current_radial = desired_radial;
    } else {
        current_radial = glm::normalize(current_radial);
    }

    strength = glm::clamp(strength, 0.0f, 1.0f);

    glm::vec3 final_radial = glm::mix(current_radial, desired_radial, strength);

    if (glm::length2(final_radial) < eps)
        final_radial = desired_radial;
    else
        final_radial = glm::normalize(final_radial);

    joint = circle_center + final_radial * h;

    return true;
}

bool apply_bend_angle_bias(std::vector<glm::vec3>& positions, int joint_index, float target_angle, float min_angle, float max_angle, const glm::vec3& pole_dir, float strength) {
    constexpr float eps = 0.000001f;

    const int n = static_cast<int>(positions.size());

    if (joint_index <= 0 || joint_index >= n - 1) return false;

    glm::vec3 joint_pos = positions[joint_index];

    glm::vec3 prev_dir = positions[joint_index] - positions[joint_index - 1];
    glm::vec3 next_dir = positions[joint_index + 1] - positions[joint_index];

    if (glm::length2(prev_dir) < eps || glm::length2(next_dir) < eps) return false;

    prev_dir = glm::normalize(prev_dir);
    next_dir = glm::normalize(next_dir);

    float current_angle = std::acos(glm::clamp(glm::dot(prev_dir, next_dir), -1.0f, 1.0f));

    strength = glm::clamp(strength, 0.0f, 1.0f);

    float desired_angle = glm::mix(current_angle, target_angle, strength);
    desired_angle = glm::clamp(desired_angle, min_angle, max_angle);

    // Build a bend direction from the stable pole direction.
    glm::vec3 radial = pole_dir - prev_dir * glm::dot(pole_dir, prev_dir);

    if (glm::length2(radial) < eps) return false;

    radial = glm::normalize(radial);

    glm::vec3 desired_next_dir = glm::normalize(prev_dir * std::cos(desired_angle) + radial * std::sin(desired_angle));

    glm::quat delta = glm::rotation(next_dir, desired_next_dir);

    // Rotate the whole downstream part of the chain around this joint.
    // This preserves downstream bone lengths.
    for (int i = joint_index + 1; i < n; ++i) {
        positions[i] = joint_pos + delta * (positions[i] - joint_pos);
    }

    return true;
}

bool apply_bend_angle_limit(std::vector<glm::vec3>& positions, int joint_index, float min_angle, float max_angle, const glm::vec3& pole_dir, float strength) {
    constexpr float eps = 0.000001f;

    const int n = static_cast<int>(positions.size());

    if (joint_index <= 0 || joint_index >= n - 1) return false;

    glm::vec3 joint_pos = positions[joint_index];

    glm::vec3 prev_dir = positions[joint_index] - positions[joint_index - 1];
    glm::vec3 next_dir = positions[joint_index + 1] - positions[joint_index];

    if (glm::length2(prev_dir) < eps || glm::length2(next_dir) < eps) return false;

    prev_dir = glm::normalize(prev_dir);
    next_dir = glm::normalize(next_dir);

    float current_angle = std::acos(glm::clamp(glm::dot(prev_dir, next_dir), -1.0f, 1.0f));

    float clamped_angle = glm::clamp(current_angle, min_angle, max_angle);

    // If already inside limits, do nothing.
    if (std::abs(clamped_angle - current_angle) < glm::radians(0.1f)) return false;

    strength = glm::clamp(strength, 0.0f, 1.0f);

    float desired_angle = glm::mix(current_angle, clamped_angle, strength);

    glm::vec3 radial = pole_dir - prev_dir * glm::dot(pole_dir, prev_dir);

    if (glm::length2(radial) < eps) return false;

    radial = glm::normalize(radial);

    glm::vec3 desired_next_dir = glm::normalize(prev_dir * std::cos(desired_angle) + radial * std::sin(desired_angle));

    glm::quat delta = glm::rotation(next_dir, desired_next_dir);

    for (int i = joint_index + 1; i < n; ++i) {
        positions[i] = joint_pos + delta * (positions[i] - joint_pos);
    }

    return true;
}

glm::quat make_aim_up_rotation(glm::vec3 world_aim, glm::vec3 world_up_hint, glm::vec3 local_aim, glm::vec3 local_up) {
    constexpr float eps = 0.000001f;

    if (glm::length2(world_aim) < eps || glm::length2(local_aim) < eps) return glm::identity<glm::quat>();

    world_aim = glm::normalize(world_aim);
    local_aim = glm::normalize(local_aim);

    // Project up vectors so they are perpendicular to the aim axis.
    world_up_hint -= world_aim * glm::dot(world_up_hint, world_aim);
    local_up -= local_aim * glm::dot(local_up, local_aim);

    if (glm::length2(world_up_hint) < eps) {
        world_up_hint = glm::vec3(0, 1, 0);
        world_up_hint -= world_aim * glm::dot(world_up_hint, world_aim);

        if (glm::length2(world_up_hint) < eps) {
            world_up_hint = glm::vec3(1, 0, 0);
            world_up_hint -= world_aim * glm::dot(world_up_hint, world_aim);
        }
    }

    if (glm::length2(local_up) < eps) {
        local_up = glm::vec3(0, 0, 1);
        local_up -= local_aim * glm::dot(local_up, local_aim);

        if (glm::length2(local_up) < eps) {
            local_up = glm::vec3(1, 0, 0);
            local_up -= local_aim * glm::dot(local_up, local_aim);
        }
    }

    world_up_hint = glm::normalize(world_up_hint);
    local_up = glm::normalize(local_up);

    glm::vec3 world_right = glm::normalize(glm::cross(world_up_hint, world_aim));
    world_up_hint = glm::normalize(glm::cross(world_aim, world_right));

    glm::vec3 local_right = glm::normalize(glm::cross(local_up, local_aim));
    local_up = glm::normalize(glm::cross(local_aim, local_right));

    glm::mat3 world_basis;
    world_basis[0] = world_right;
    world_basis[1] = world_up_hint;
    world_basis[2] = world_aim;

    glm::mat3 local_basis;
    local_basis[0] = local_right;
    local_basis[1] = local_up;
    local_basis[2] = local_aim;

    glm::mat3 rot = world_basis * glm::transpose(local_basis);

    return glm::normalize(glm::quat_cast(rot));
}

FABRIKSolverOutput tmt::AnimConstraints::FABRIKIK::solve_fabrik(const FABRIKSolverInput& input) {
    if (input.chain_entities.size() < 2) {
        tmt::Log::warn("[AnimConstraints] Insufficient entities passed to solve_fabrik, must be 2 or higher");
        return FABRIKSolverOutput {};
    }

    std::vector<glm::vec3> initial_chain_positions;
    initial_chain_positions.resize(input.chain_entities.size());

    std::vector<glm::quat> initial_chain_rotations;
    initial_chain_rotations.resize(input.chain_entities.size());

    std::vector<glm::vec3> chain_positions;
    chain_positions.resize(input.chain_entities.size());

    std::vector<float> distances;
    distances.resize(input.chain_entities.size() - 1);

    std::vector<glm::quat> solved_world_rotations;
    solved_world_rotations.resize(input.chain_entities.size() - 1);

    FABRIKSolverOutput output {};
    output.chain_rotations.resize(input.chain_entities.size() - 1);

    float sum = 0.f;
    for (unsigned int i = 0; i < input.chain_entities.size(); i++) {
        const auto& chain_transform = tmt::engine.ecs.get_component<tmt::Transform>(input.chain_entities[i]);
        chain_positions[i] = chain_transform.get_world_position();
        initial_chain_positions[i] = chain_positions[i];
        initial_chain_rotations[i] = chain_transform.get_world_rotation();

        if (i > 0) {
            distances[i - 1] = glm::length(chain_positions[i - 1] - chain_positions[i]);
            sum += distances[i - 1];
        }
    }
    glm::vec3 root = chain_positions[0];
    glm::vec3 target = input.effector;

    float stretch_slack = 0.2f;  // world units, tune this
    float max_reach = std::max(0.0f, sum - stretch_slack);

    glm::vec3 root_to_target = target - root;
    float target_dist = glm::length(root_to_target);

    glm::vec3 solver_target = target;

    if (target_dist > max_reach && target_dist > 0.000001f) {
        glm::vec3 dir = root_to_target / target_dist;
        solver_target = root + dir * max_reach;
    }

    /*for (int i = 1; i < static_cast<int>(chain_positions.size()) - 1; ++i) {
         glm::vec3 pole = {};
         derive_pole_dir(initial_chain_positions[i - 1], initial_chain_positions[i], initial_chain_positions[i + 1], pole);
         pole_dirs[i - 1] = pole;
    }*/

    // if(glm::length(chain_positions[0] - input.effector) > sum)
    //{
    //     //target unreachable
    //     glm::vec3 dir = input.effector - chain_positions[0];

    //    if (glm::length2(dir) > 0.000001f) {
    //        dir = glm::normalize(dir);

    //        for (int i = 1; i < chain_positions.size(); ++i) {
    //            chain_positions[i] = chain_positions[i - 1] + dir * distances[i - 1];

    //             auto isnan = glm::isnan(chain_positions[i]);
    //            assert(!isnan.x && !isnan.y && !isnan.z);
    //        }
    //    }
    //} else
    //{
    // target reachable
    glm::vec3 initial_root = chain_positions[0];
    float goal_diff = glm::length(chain_positions[chain_positions.size() - 1] - solver_target);

    int iter = 0;
    constexpr int MAX_ITERATIONS = 1000;
    while (goal_diff > 0.01f && iter < MAX_ITERATIONS) {
        chain_positions[chain_positions.size() - 1] = solver_target;

        // backwards pass
        for (int i = static_cast<int>(chain_positions.size()) - 2; i >= 0; i--) {
            glm::vec3 v = glm::normalize(chain_positions[i + 1] - chain_positions[i]) * distances[i];
            auto isnan = glm::isnan(v);
            assert(!isnan.x && !isnan.y && !isnan.z);

            chain_positions[i] = chain_positions[i + 1] - v;

            isnan = glm::isnan(chain_positions[i]);
            assert(!isnan.x && !isnan.y && !isnan.z);
        }

        // forwards pass
        chain_positions[0] = initial_root;
        for (int i = 0; i < chain_positions.size() - 1; i++) {
            glm::vec3 v = glm::normalize(chain_positions[i + 1] - chain_positions[i]) * distances[i];
            chain_positions[i + 1] = chain_positions[i] + v;

            auto isnan = glm::isnan(chain_positions[i + 1]);
            assert(!isnan.x && !isnan.y && !isnan.z);
        }

        for (int i = 1; i < static_cast<int>(chain_positions.size()) - 1; ++i) {
            apply_pole_constraint(chain_positions[i - 1], chain_positions[i], chain_positions[i + 1], distances[i - 1], distances[i], input.poles[i - 1], 0.1f);
        }

        for (int i = 1; i < static_cast<int>(chain_positions.size()) - 1; ++i) {
            apply_bend_angle_limit(chain_positions, i,  glm::radians(5.f), glm::radians(360.f), input.poles[i - 1], input.angle_strengths[i-1]);
        }

        goal_diff = glm::length(chain_positions[chain_positions.size() - 1] - solver_target);
        ++iter;
    }

    for (int i = 0; i < chain_positions.size() - 1; i++) {
        glm::vec3 initial_dir = glm::normalize(initial_chain_positions[i + 1] - initial_chain_positions[i]);
        glm::vec3 solved_dir = glm::normalize(chain_positions[i + 1] - chain_positions[i]);

        //glm::quat delta = glm::rotation(initial_dir, solved_dir);

       if (i == chain_positions.size() - 2) {
            glm::vec3 rig_up_world = input.rig_world_rotation * glm::vec4(0.f, 0.f, 1.f, 0.f);

            //auto& last_transform = tmt::engine.ecs.get_component<tmt::Transform>(input.chain_entities[input.chain_entities.size()-1]);

            solved_world_rotations[i] = make_aim_up_rotation(solved_dir, rig_up_world, input.local_aim, input.local_up);
        } else {
            glm::quat delta = glm::rotation(initial_dir, solved_dir);

            solved_world_rotations[i] = glm::normalize(delta * initial_chain_rotations[i]);
        }
    }

    glm::quat parent_world_rot = glm::quat_identity<float, glm::packed_highp>();
    auto& root_transform = tmt::engine.ecs.get_component<tmt::Transform>(input.chain_entities[0]);
    if (root_transform.has_parent()) {
        auto& parent_transform = tmt::engine.ecs.get_component<tmt::Transform>(root_transform.get_parent());
        parent_world_rot = parent_transform.get_world_rotation();
    }
    for (int i = 0; i < chain_positions.size() - 1; i++) {
        output.chain_rotations[i] = glm::normalize(glm::inverse(parent_world_rot) * solved_world_rotations[i]);
        parent_world_rot = solved_world_rotations[i];
    }

    for (int i = 1; i < static_cast<int>(chain_positions.size()) - 1; ++i) {
        tmt::engine.polyline.use_line_width(2.f);
        tmt::engine.polyline.use_color({0.f, 0.f, 1.f, 1.f});
        tmt::engine.polyline.draw_arrow(chain_positions[i], input.poles[i - 1], 2.f);
    }

    return output;
}

bool derive_pole_dir(const glm::vec3& a, const glm::vec3& joint, const glm::vec3& b, glm::vec3& out_dir) {
    constexpr float eps = 0.000001f;

    glm::vec3 ab = b - a;

    if (glm::length2(ab) < eps) return false;

    glm::vec3 axis = glm::normalize(ab);

    glm::vec3 projected = a + axis * glm::dot(joint - a, axis);

    glm::vec3 radial = joint - projected;

    if (glm::length2(radial) < eps) return false;

    out_dir = glm::normalize(radial);
    return true;
}

}  // namespace FABRIKIK
}  // namespace AnimConstraints
}  // namespace tmt