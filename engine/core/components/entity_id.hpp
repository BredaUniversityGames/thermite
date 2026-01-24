#pragma once
#include "engine/tools/uuid.hpp"
#include "engine/core/reflection.hpp"
#include <compare>

namespace tmt {
class EntityID {
   public:
    EntityID() : uuid(NULL_UUID) {}
    EntityID(const UUID& id) : uuid(id) {}

    operator const UUID&() const { return uuid; }
    auto operator<=>(const EntityID& other) const = default;

    const UUID& get_uuid() const { return uuid; }

   private:
    BEFRIEND_VISITABLE();
    UUID uuid;
};
}  // namespace tmt

namespace std {
template <>
struct hash<tmt::EntityID> {
    size_t operator()(const tmt::EntityID& uuid) const { return uuid.get_uuid().hash(); }
};
}  // namespace std

TMT_COMPONENT(tmt::EntityID, "EntityID", (uuid));
FMT_LOGGING(tmt::EntityID, "{}", obj.get_uuid());