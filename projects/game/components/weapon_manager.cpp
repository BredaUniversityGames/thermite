#include "weapon_manager.hpp"

#include "weapon.hpp"
void game::WeaponManager::switch_to(WeaponType weapon_slot) {
    unsubscribe_weapon(current_weapon);
    current_weapon = weapon_slot;
    subscribe_weapon(current_weapon);
}
void game::WeaponManager::start() {
    subscribe_weapon(starting_slot);
}
void game::WeaponManager::update(const tmt::FrameData& time) {}
void game::WeaponManager::end() {}
// TODO this will be replaced by active disabled tags per weapon entity
void game::WeaponManager::subscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot);
    if (e == entt::null) return;

    auto* weapon = tmt::engine.ecs.get_registry().try_get<Weapon>(e);
    if (!weapon) return;

    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().connect<&Weapon::on_shoot>(weapon);
}

void game::WeaponManager::unsubscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot);
    if (e == entt::null) return;

    auto* weapon = tmt::engine.ecs.get_registry().try_get<Weapon>(e);
    if (!weapon) return;

    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().disconnect<&Weapon::on_shoot>(weapon);
}
