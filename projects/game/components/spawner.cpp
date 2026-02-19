#include "spawner.hpp"

#include "engine/tools/prefab_helper.hpp"
namespace game {

std::string_view Spawner::get_name() {
    return "Spawner";
}
void Spawner::start() {
    if (use_attached_entity_as_spawn_parent) {
        spawn_parent = entity;
    }
}
void Spawner::spawn() const {
    tmt::PrefabHelper::instantiate_prefab(prefab_location, spawn_parent);
}
void Spawner::spawn(const tmt::IO::FileLocation& location, const tmt::Entity& parent) {
    tmt::PrefabHelper::instantiate_prefab(location, parent);
}

}  // namespace game
