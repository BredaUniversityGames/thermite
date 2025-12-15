#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

#include <deque>

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 86.25f, -5.0f));
    }

    //{ /* Voxel entity */
    //    voxel = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
    //    renderer.size = glm::uvec3(20u, 10u, 10u);
    //    transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
    //    transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
    //    transform.set_world_scale(glm::vec3(1.0f, 2.0f, 1.0f));
    //}

    { /* Voxel Physics Entity */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "dragon128.vengi"});
        auto voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
        renderer.resource = voxel_volume;

        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.voxels.size = glm::uvec3(20u, 20u, 20u);
        vb.width = 2.0f;
        vb.height = 2.0f;
        vb.depth = 2.0f;

        // Set physics voxel data
        for (size_t z = 0; z < 20; z++) {
            for (size_t y = 0; y < 20; y++) {
                for (size_t x = 0; x < 20; x++) {
                    glm::ivec3 normal = {};

                    int numb = 0;
                    if (x == 0 || x == 19) numb++;
                    if (y == 0 || y == 19) numb++;
                    if (z == 0 || z == 19) numb++;

                    if (x == 0)
                        normal.x = -1;
                    else if (x == 19)
                        normal.x = 1;

                    if (y == 0)
                        normal.y = -1;
                    else if (y == 19)
                        normal.y = 1;

                    if (z == 0)
                        normal.z = -1;
                    else if (z == 19)
                        normal.z = 1;

                    int index = 0;
                    if (numb == 3) {
                        for (size_t i = 19; i < 27; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::CORNER, (uint8_t)index});
                    } else if (numb == 2) {
                        for (size_t i = 7; i < 19; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::EDGE, (uint8_t)index});
                    } else if (numb == 1) {
                        for (size_t i = 1; i < 7; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::FACE, (uint8_t)index});
                    }
                    else
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::INSIDE, 0});
                }
            }
        }

        std::deque<uint32_t> queue {};
        while (!queue.empty()) {
            const uint32_t& current = queue[0];
            uint32_t x = current % 20;
            uint32_t y = (current % (20 * 20)) / 20;
            uint32_t z = current / (20 * 20);

            // add voxel neighbours to queue and propegate normal index
            for (int sign = 0; sign < 2; sign++) {
                for (size_t axis = 0; axis < 3; axis++) {
                    glm::uvec3 p = glm::uvec3(x, y, z);
                    p[(glm::uvec3::length_type)axis] += (sign * 2) - 1;

                    // If not in range continue
                    if (p.x >= 20 || p.y >= 20 || p.z >= 20) continue;

                    // If the voxel already has a normal index continue
                    uint32_t neighbour = p.x + 20 * p.y + 20 * 20 * p.z;
                    if (vb.voxels.data[neighbour].normal_index != 0) continue;

                    vb.voxels.data[neighbour].normal_index = vb.voxels.data[current].normal_index;

                    queue.push_back(neighbour);
                }
            }

            queue.pop_front();
        }

        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::DYNAMIC;
        //renderer.size = glm::uvec3(20u, 20u, 20u);
        transform.set_world_position(glm::vec3(0.0f, 85.0f, 10.0f));
    }

    /* Voxel Physics Entity */ //9998
    for (int i = 0; i < 0; i++) { 
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.voxels.size = glm::uvec3(10u, 10u, 10u);

        // Set physics voxel data
        for (size_t z = 0; z < 10; z++) {
            for (size_t y = 0; y < 10; y++) {
                for (size_t x = 0; x < 10; x++) {
                    glm::ivec3 normal = {};
                    int numb = 0;

                    if (x == 0 || x == 9) numb++;
                    if (y == 0 || y == 9) numb++;
                    if (z == 0 || z == 9) numb++;

                    if (x == 0)
                        normal.x = -1;
                    else if (x == 9)
                        normal.x = 1;

                    if (y == 0)
                        normal.y = -1;
                    else if (y == 9)
                        normal.y = 1;

                    if (z == 0)
                        normal.z = -1;
                    else if (z == 9)
                        normal.z = 1;

                    int index = 0;
                    if (numb == 3) {
                        for (size_t i = 19; i < 27; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::CORNER, (uint8_t)index});
                    } else if (numb == 2) {
                        for (size_t i = 7; i < 19; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::EDGE, (uint8_t)index});
                    } else if (numb == 1) {
                        for (size_t i = 1; i < 7; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::FACE, (uint8_t)index});
                    } else
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::INSIDE, 0});
                }
            }
        }

        std::deque<uint32_t> queue {};
        while (!queue.empty()) {
            const uint32_t& current = queue[0];
            uint32_t x = current % 10;
            uint32_t y = (current % (10 * 10)) / 10;
            uint32_t z = current / (10 * 10);

            // add voxel neighbours to queue and propegate normal index
            for (int sign = 0; sign < 2; sign++) {
                for (size_t axis = 0; axis < 3; axis++) {
                    glm::uvec3 p = glm::uvec3(x, y, z);
                    p[(glm::uvec3::length_type)axis] += (sign * 2) - 1;

                    // If not in range continue
                    if (p.x >= 10 || p.y >= 10 || p.z >= 10) continue;

                    // If the voxel already has a normal index continue
                    uint32_t neighbour = p.x + 10 * p.y + 10 * 10 * p.z;
                    if (vb.voxels.data[neighbour].normal_index != 0) continue;

                    vb.voxels.data[neighbour].normal_index = vb.voxels.data[current].normal_index;

                    queue.push_back(neighbour);
                }
            }

            queue.pop_front();
        }

        //vb.gravity = 9.81f;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::DYNAMIC;
        //renderer.size = glm::uvec3(10u, 10u, 10u);
        glm::vec3 random_pos = glm::vec3(static_cast<float>(rand() % 10000) / 10000.0f - 0.5f, static_cast<float>(rand() % 10000) / 10000.0f, static_cast<float>(rand() % 10000) / 10000.0f - 0.5f);

        transform.set_world_position(random_pos * 100.0f);
    }

    { /* Voxel Physics Entity */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.voxels.size = glm::uvec3(10u, 10u, 10u);

        // Set physics voxel data
        for (size_t z = 0; z < 10; z++) {
            for (size_t y = 0; y < 10; y++) {
                for (size_t x = 0; x < 10; x++) {
                    glm::ivec3 normal = {};
                    int numb = 0;

                    if (x == 0 || x == 9) numb++;
                    if (y == 0 || y == 9) numb++;
                    if (z == 0 || z == 9) numb++;

                    if (x == 0)
                        normal.x = -1;
                    else if (x == 9)
                        normal.x = 1;

                    if (y == 0)
                        normal.y = -1;
                    else if (y == 9)
                        normal.y = 1;

                    if (z == 0)
                        normal.z = -1;
                    else if (z == 9)
                        normal.z = 1;

                    int index = 0;
                    if (numb == 3) {
                        for (size_t i = 19; i < 27; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::CORNER, (uint8_t)index});
                    } else if (numb == 2) {
                        for (size_t i = 7; i < 19; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::EDGE, (uint8_t)index});
                    } else if (numb == 1) {
                        for (size_t i = 1; i < 7; i++) {
                            if (tmt::NORMAL_LUT[i] == normal) index = i;
                        }
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::FACE, (uint8_t)index});
                    } else
                        vb.voxels.data.push_back({tmt::PhysicsVoxelType::INSIDE, 0});
                }
            }
        }

        std::deque<uint32_t> queue {};
        while (!queue.empty()) {
            const uint32_t& current = queue[0];
            uint32_t x = current % 10;
            uint32_t y = (current % (10 * 10)) / 10;
            uint32_t z = current / (10 * 10);

            // add voxel neighbours to queue and propegate normal index
            for (int sign = 0; sign < 2; sign++) {
                for (size_t axis = 0; axis < 3; axis++) {
                    glm::uvec3 p = glm::uvec3(x, y, z);
                    p[(glm::uvec3::length_type)axis] += (sign * 2) - 1;

                    // If not in range continue
                    if (p.x >= 10 || p.y >= 10 || p.z >= 10) continue;

                    // If the voxel already has a normal index continue
                    uint32_t neighbour = p.x + 10 * p.y + 10 * 10 * p.z;
                    if (vb.voxels.data[neighbour].normal_index != 0) continue;

                    vb.voxels.data[neighbour].normal_index = vb.voxels.data[current].normal_index;

                    queue.push_back(neighbour);
                }
            }

            queue.pop_front();
        }

        vb.gravity = 2.0f;
        vb.type = tmt::VoxelBody::DYNAMIC;
        vb.density = 100.0f;
        //renderer.size = glm::uvec3(10u, 10u, 10u);
        transform.set_world_position(glm::vec3(-0.4f, 90.25f, 10.0f));
    }

    // constexpr float PRIM_RANGE = 128.0f;
    // for (int i = 0; i < 255; ++i) {
    //     auto entity = tmt::engine.ecs.create_entity();
    //     auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    //     auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //     renderer.size = glm::uvec3(10u + (rand() % 90u), 10u + (rand() % 90u), 10u + (rand() % 90u));
    //     float rx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float ry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float rz = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float rrx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
    //     float rry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
    //     transform.set_world_position(glm::vec3(rx, ry, rz));
    //     transform.set_world_rotation(glm::vec3(glm::radians(rrx), glm::radians(rry), 0.0f));
    //     transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    // auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    // transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    // transform.set_world_rotation(glm::vec3(sinf(time_passed), cosf(time_passed), 0.0f));
    // transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
