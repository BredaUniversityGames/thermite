#include "chase_player.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void ChasePlayer::on_start(tmt::Entity enemy_entity) {
    for (const auto& [CamEntity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = CamEntity;
        break;
    }
}

void ChasePlayer::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    //if (enemy.audio_emitter != nullptr) {
    //    const tmt::AudioInstance3D instance = enemy.audio_emitter->play(enemy.sounds.sound_walk);
    //    instance.set_maximum_distance(enemy.aggro_range * 1.5f);  // Multiply be 1.5f to ensure the player can hear it even when at the edge of the range
    //}
    enemy.kite_player();
}

bool ChasePlayer::is_done(tmt::Entity) const {
    return false;
}
