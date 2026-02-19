#include "weapon.hpp"

#include "engine/tools/fmt/glm.hpp"

void game::Weapon::start() {
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().connect<&Weapon::on_shoot>(this);
}
void game::Weapon::update(const tmt::FrameData& time) {
    elapsed_game_time = time.elapsed_time;
}
void game::Weapon::end() {
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().disconnect<&Weapon::on_shoot>(this);
}
void game::Weapon::on_shoot(const ShootEvent& e) {
    if (e.shooting_entity != shooting_entity) return;

    if (elapsed_game_time - lastShotTime < 1.f / fire_rate) {
        return;
    }
    lastShotTime = elapsed_game_time;

    const auto& tf = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    tmt::Log::info("{} entity shot from {} to {}", shooting_entity, tf.get_world_position(), tf.get_forward());
    tmt::engine.ecs.get_dispatcher().trigger(WeaponFiredEvent { .shooter_entity = shooting_entity, .origin = tf.get_world_position(), .direction = tf.get_forward() });
}
