#pragma once
#include <vector>

namespace tmt {

// class GoapAction;
//
// class GoapActionRegistry {
//    public:
//     static GoapActionRegistry& instance() {
//         static GoapActionRegistry inst;
//         return inst;
//     }
//
//     void register_action(GoapAction* action) { actions[action->get_id()] = action; }
//
//     GoapAction* get(GoapActionId id) const {
//         auto it = actions.find(id);
//         return it != actions.end() ? it->second : nullptr;
//     }
//
//     const auto& get_all() const { return actions; }
//
//    private:
//     std::unordered_map<GoapActionId, GoapAction*> actions;
// };

class GoapActionRegistry {
   public:
    static GoapActionRegistry& instance() {
        static GoapActionRegistry inst;
        return inst;
    }

    void register_action(std::unique_ptr<GoapAction> action) { actions[action->get_id()] = std::move(action); }

    GoapAction* get(const std::string& id) {
        auto it = actions.find(id);
        return (it != actions.end()) ? it->second.get() : nullptr;
    }

    const auto& get_all() const { return actions; }

   private:
    std::unordered_map<std::string, std::unique_ptr<GoapAction>> actions;
};

}  // namespace tmt
