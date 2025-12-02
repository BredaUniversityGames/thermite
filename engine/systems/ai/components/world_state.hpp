#pragma once
#include "ecs.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace tmt {

// Represents a unique identifier for a world state fact.
struct fact_id {
    uint32_t id;  // hash of the fact name

    fact_id() : id(0) {}
    fact_id(const std::string& name) : id(std::hash<std::string> {}(name)) {}

    bool operator==(const fact_id& other) const { return id == other.id; }
};

// Represents the value of a fact.
// Can be extended to support multiple types. Currently supports boolean, integer, and float.
struct fact_value {
    enum class type { bool_type, int_type, float_type } value_type;

    union {
        bool bool_val;
        int int_val;
        float float_val;
    };

    fact_value() : value_type(type::bool_type), bool_val(false) {}
    fact_value(bool b) : value_type(type::bool_type), bool_val(b) {}
    fact_value(int i) : value_type(type::int_type), int_val(i) {}
    fact_value(float f) : value_type(type::float_type), float_val(f) {}
};

// A simple pair of fact_id and its value.
// Used in preconditions, effects, and when applying changes to the world state.
struct fact_pair {
    fact_id id;
    fact_value value;
};

// Container for the agent's knowledge of the world state.
struct world_state {
    std::unordered_map<uint32_t, fact_value> facts;

    // Apply a set of effects to the world state. Adds new facts if they don't exist, or updates existing ones.
    void apply(const std::vector<fact_pair>& effects) {
        for (const auto& e : effects) {
            facts[e.id.id] = e.value;
        }
    }

    // Checks whether the world state satisfies a given set of conditions.
    // Returns true if all facts in `conditions` exist in the world state and have matching values.
    bool satisfies(const std::vector<fact_pair>& conditions) const {
        for (const auto& cond : conditions) {
            auto it = facts.find(cond.id.id);
            if (it == facts.end()) return false;

            const auto& val = it->second;

            if (val.value_type == fact_value::type::bool_type && val.bool_val != cond.value.bool_val) return false;
        }
        return true;
    }

    // Helper to get a fact value safely.
    const fact_value* try_get(const fact_id& id) const {
        auto it = facts.find(id.id);
        if (it != facts.end()) return &it->second;
        return nullptr;
    }
};

}  // namespace tmt
