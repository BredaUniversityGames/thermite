#include "goap_action.hpp"
#include "core/logger.hpp"

namespace tmt {

// Default implementation: check that every precondition in the action
// is present in the world state and has the same boolean value.
// Returns false if a fact is missing or mismatched.
bool GoapAction::check_preconditions(const WorldState& ws) const {
    for (const auto& [key, want] : preconditions) {
        FactId fid(key);
        const FactValue* fv = ws.try_get(fid);
        if (!fv) {
            // fact missing
            return false;
        }

        if (fv->value_type != FactValue::Type::BOOL_TYPE) {
            // mismatch type -> treat as not satisfied
            return false;
        }

        if (fv->bool_val != want) {
            return false;
        }
    }

    return true;
}

// Helper to check agent;s world state
void print_world_state(const WorldState& ws) {
    Log::info("WorldState:");
    for (const auto& [id, val] : ws.facts) {
        std::string val_str;
        switch (val.value_type) {
            case FactValue::Type::BOOL_TYPE:
                val_str = val.bool_val ? "true" : "false";
                break;
            case FactValue::Type::INT_TYPE:
                val_str = std::to_string(val.int_val);
                break;
            case FactValue::Type::FLOAT_TYPE:
                val_str = std::to_string(val.float_val);
                break;
        }
        Log::info("  FactID {} = {}", id, val_str);
    }
}

// Default implementation: apply this action's effects to the provided world state.
// Converts unordered_map<string,bool> -> vector<FactPair> then calls WorldState::apply().
void GoapAction::apply_effects(WorldState& ws) const {
    std::vector<FactPair> effects_vec;
    effects_vec.reserve(effects.size());

    for (const auto& [key, val] : effects) {
        FactPair fp;
        fp.id = FactId(key);
        fp.value = FactValue(val);
        effects_vec.push_back(fp);
    }

    ws.apply(effects_vec);
    print_world_state(ws);
}

}  // namespace tmt
