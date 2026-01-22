#pragma once
#include <random>
#include "uuid_v4.h"
#include "engine/core/reflection.hpp"

namespace tmt {

using UUID = UUIDv4::UUID;

class UUIDGenerator : public UUIDv4::UUIDGenerator<std::mt19937_64> {
   public:
    UUIDGenerator() : UUIDv4::UUIDGenerator<std::mt19937_64>() {}
    UUIDGenerator(uint64_t seed) : UUIDv4::UUIDGenerator<std::mt19937_64>(seed) {}

    static UUID generate() {
        static UUIDGenerator generator;
        return generator.getUUID();
    }
};

}  // namespace tmt

TMT_COMPONENT_NAME(tmt::UUID, "UUID");