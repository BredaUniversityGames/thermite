#include "mining_tool.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "game_input.hpp"
#include "engine/core/logger.hpp"

using namespace game;

void MiningTool::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;
    mining_time_since_last_mine_action += time.delta_time;

    if (input.is_action_pressed(action::SHOOT)) {
        if (mining_time_since_last_mine_action >= mining_cooldown)
        {
            mine();
            mining_time_since_last_mine_action = 0;
        }
    }
}

void MiningTool::mine() {
    
    // Create a ray from center of the viewport and traces it

    const tmt::Ray ray_cast = tmt::engine.renderer.render_view.pixel_ray(tmt::engine.renderer.viewport_size());
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);


    if (hit.miss() == false) {
        tmt::Log::info("test");
        auto* resource = tmt::engine.ecs.get_component<tmt::VoxelBody>(hit.entity).resource.resource.get();
        resource->blas->subtract(stencil.resource.get(), hit.coord);
        resource->set_dirty();
    }
    
}