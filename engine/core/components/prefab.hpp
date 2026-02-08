#pragma once
#include "engine/core/io.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

using PrefabInstanceID = UUID;

struct Prefab {
    IO::FileLocation source_location {};
    tmt::Entity source_entity = entt::null; /* in Prefab */
    tmt::Entity root_entity = entt::null;   /* in Scene */
    PrefabInstanceID instance_id {NULL_UUID};

    bool operator==(const Prefab& other) const = default;
};

}  // namespace tmt

TMT_COMPONENT(tmt::Prefab, "Prefab", (source_location, source_entity, root_entity, instance_id));

namespace std {
template <>
struct hash<tmt::Prefab> {
    std::size_t operator()(const tmt::Prefab& prefab) const noexcept {
        size_t h1 = std::hash<tmt::IO::FileLocation> {}(prefab.source_location);
        size_t h2 = std::hash<tmt::Entity> {}(prefab.source_entity);
        size_t h3 = std::hash<tmt::Entity> {}(prefab.root_entity);
        size_t h4 = std::hash<tmt::PrefabInstanceID> {}(prefab.instance_id);
        return h1 ^ (h2 + 0x9e3779b9ull + (h1 << 6) + (h1 >> 2)) ^ (h3 + 0x9e3779b9ull + (h2 << 6) + (h2 >> 2)) ^ (h4 + 0x9e3779b9ull + (h3 << 6) + (h3 >> 2));
    }
};
}  // namespace std