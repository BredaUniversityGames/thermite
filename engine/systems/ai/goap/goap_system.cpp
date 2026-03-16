#include "goap_system.hpp"
#include "core/logger.hpp"

#include "components/goap_agent_factory.hpp"
#include "components/goap_agent_type_ref.hpp"

#include "systems/ai/steering/components/steering_agent.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

#include <queue>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace tmt {

void Goap::on_start() {
    Log::info("Goap on_start");
    agent_types().load();

    auto& registry = engine.ecs.get_registry();

    // Build runtime agents from serialized scene data
    for (auto entity : engine.ecs.view<GoapAgentTypeRef>()) {
        auto& type_ref = registry.get<GoapAgentTypeRef>(entity);

        // Avoid double building if already exists
        if (registry.any_of<GoapAgent>(entity)) continue;

        GoapAgentFactory::spawn_agent_from_type(type_ref.type_id, entity);
        registry.emplace<SteeringAgent>(entity);
    }
}

/**
 * Main update loop called every frame.
 *
 * Iterates over all entities that have both:
 *   - GoapAgent
 *   - WorldState
 *
 * For each agent, run the GOAP lifecycle:
 *    1. Select a goal (if needed)
 *    2. Plan (if needed)
 *    3. Execute action(s)
 */
void Goap::on_update(const FrameData& time) {
    for (auto [entity, agent, world] : engine.ecs.view<GoapAgent, WorldState>().each()) {
        process_agent(entity, world, time.delta_time);

        if (agent.current_action && agent.current_action->is_running && agent.current_action->wants_fixed_update == false) {
            agent.current_action->on_tick(entity, time.delta_time);
        }
    }
}

void Goap::on_fixed_update(const FrameData& time) {
    for (auto [entity, agent] : engine.ecs.view<GoapAgent>().each()) {
        if (agent.current_action && agent.current_action->is_running && agent.current_action->wants_fixed_update == true) {
            agent.current_action->on_fixed_tick(entity, time.delta_time);
        }
    }
}

void Goap::on_end() {
    Log::info("Goap on_end");
}

// ------------------------------------------------------
// Main per-agent update
// ------------------------------------------------------
/**
 * Executes a full GOAP cycle for a single agent:
 *
 *  1. update_goal()   : choose what the agent wants now
 *  2. update_plan()   : generate a new plan when needed
 *  3. update_action() : tick the action currently running
 */
void Goap::process_agent(Entity entity, WorldState& ws, float /*dt*/) {
    auto& registry = engine.ecs.get_registry();
    auto& agent = registry.get<GoapAgent>(entity);

    update_goal(entity, agent, ws);
    if (agent.needs_replan) update_plan(entity, agent, ws);
    update_action(entity, agent, ws);
}

// ------------------------------------------------------
// Goal assignment
// ------------------------------------------------------
/**
 * Choose the highest priority relevant goal.
 *
 * Rules:
 *   - If agent already has an active, valid goal ? keep it.
 *   - Sort all available_goals by priority.
 *   - Choose the first goal whose conditions are not satisfied.
 *   - If all goals are satisfied ? pick lowest priority as fallback.
 */
void Goap::update_goal(Entity entity, GoapAgent& agent, WorldState& ws) {
    // ------------------------------------------------------
    // Invalidate goal if no longer relevant
    // ------------------------------------------------------
    if (agent.has_goal() && !agent.active_goal.is_relevant(ws)) {
        if (show_logging) {
            Log::info("GOAP: Agent {} invalidating goal '{}' (no longer relevant)", entity, agent.active_goal.name);
        }

        agent.active_goal.valid = false;
    }

    // ------------------------------------------------------
    // Allow higher priority goals to interrupt
    // ------------------------------------------------------
    if (agent.has_goal()) {
        for (const auto& goal : agent.available_goals) {
            if (goal.priority > agent.active_goal.priority && goal.is_relevant(ws)) {
                if (show_logging) {
                    Log::info("GOAP: Agent {} switching goal '{}' -> '{}'", entity, agent.active_goal.name, goal.name);
                }

                agent.active_goal.valid = false;
                break;
            }
        }

        // If we still have a valid goal and plan -> keep pursuing it
        if (agent.has_goal() && !agent.needs_replan) {
            return;
        }
    }

    if (agent.available_goals.empty()) {
        Log::warn("GOAP Agent {} has no goals.", entity);
        return;
    }

    // ------------------------------------------------------
    // Sort goals by priority
    // ------------------------------------------------------
    std::sort(agent.available_goals.begin(), agent.available_goals.end(), [](const GoapGoal& a, const GoapGoal& b) { return a.priority > b.priority; });

    if (show_logging) {
        Log::info("GOAP: Agent {} evaluating {} goals", entity, (int)agent.available_goals.size());
    }

    // ------------------------------------------------------
    // Pick highest priority relevant goal
    // ------------------------------------------------------
    for (const auto& goal : agent.available_goals) {
        if (!goal.is_relevant(ws)) {
            if (show_logging) {
                Log::info("GOAP: Goal '{}' skipped (not relevant)", goal.name);
            }

            continue;
        }

        // ------------------------------------------------------
        // Skip goals that cannot currently produce a plan
        // ------------------------------------------------------
        if (!goal_has_valid_plan(goal, agent, ws)) {
            if (show_logging) Log::info("GOAP: Goal '{}' skipped (no valid plan)", goal.name);
            continue;
        }

        agent.active_goal = goal;
        agent.needs_replan = true;

        if (show_logging) {
            Log::info("GOAP Agent {} selected goal '{}' with priority {}", entity, goal.name, goal.priority);
        }

        return;
    }

    // ------------------------------------------------------
    // Fallback goal
    // ------------------------------------------------------
    agent.active_goal = agent.available_goals.back();
    agent.needs_replan = true;

    if (show_logging) {
        Log::info("GOAP Agent {} selected fallback goal '{}'", entity, agent.active_goal.name);
    }
}

/* helper for checking valid plan for picking goals */
bool Goap::goal_has_valid_plan(const GoapGoal& goal, GoapAgent& agent, const WorldState& ws) {
    GoapGoal old_goal = agent.active_goal;
    agent.active_goal = goal;

    // Use a simple A* search, exactly like update_plan(), but do NOT store the plan in agent.plan
    struct Node {
        GoapAction* action;
        WorldState state;
        Node* parent;
        float cost_so_far;
        Node(GoapAction* a, const WorldState& s, Node* p, float c) : action(a), state(s), parent(p), cost_so_far(c) {}
    };

    std::vector<Node> nodes;
    std::priority_queue<Node*, std::vector<Node*>, std::function<bool(Node*, Node*)>> open_list([](Node* a, Node* b) { return (a->cost_so_far) > (b->cost_so_far); });
    std::unordered_set<GoapAction*> closed;

    // Seed with all actions whose preconditions are satisfied in current world state
    for (GoapAction* action : agent.available_actions) {
        if (!action) continue;
        EffectiveGoapAction eff = build_effective_action(*action, action_overrides.find(action->get_id()));
        bool satisfied = true;
        for (auto& [fact, val] : eff.preconditions) {
            uint32_t id = (uint32_t)std::hash<std::string>()(fact);
            auto it = ws.facts.find(id);
            if (it == ws.facts.end() || it->second != val) {
                satisfied = false;
                break;
            }
        }
        if (satisfied) {
            nodes.emplace_back(action, ws, nullptr, eff.cost);
            open_list.push(&nodes.back());
        }
    }

    Node* goal_node = nullptr;
    while (!open_list.empty()) {
        Node* current = open_list.top();
        open_list.pop();
        if (!current) continue;
        if (closed.count(current->action)) continue;
        closed.insert(current->action);

        // Apply effects to get new state
        WorldState temp = current->state;
        current->action->apply_effects(temp);

        if (temp.satisfies(goal.desired_state)) {
            goal_node = current;
            break;
        }

        // Expand next actions
        for (GoapAction* next : agent.available_actions) {
            if (!next || closed.count(next)) continue;
            EffectiveGoapAction eff = build_effective_action(*next, action_overrides.find(next->get_id()));
            bool satisfied = true;
            for (auto& [fact, val] : eff.preconditions) {
                uint32_t id = (uint32_t)std::hash<std::string>()(fact);
                auto it = temp.facts.find(id);
                if (it == temp.facts.end() || it->second != val) {
                    satisfied = false;
                    break;
                }
            }
            if (satisfied) {
                nodes.emplace_back(next, temp, current, current->cost_so_far + eff.cost);
                open_list.push(&nodes.back());
            }
        }
    }

    agent.active_goal = old_goal;
    return goal_node != nullptr;
}

/**
 * Builds the runtime-effective version of a GOAP action.
 *
 * GOAP actions now have two layers of data:
 *   - The base action definition (registered in code)
 *   - The editor override data (loaded from saved JSON)
 *
 * This function merges them into a single EffectiveGoapAction:
 *   - The override is applied only when it exists and contains a valid field.
 *   - Base values act as the fallback default.
 *
 * This allows designers to modify values at runtime without affecting the
 * base registered action implementation.
 */
EffectiveGoapAction build_effective_action(const GoapAction& base, const GoapActionEditorData* override) {
    EffectiveGoapAction out;

    // --- Cost ---
    out.cost = (override && override->cost >= 0.f) ? override->cost : base.cost;

    // --- Preconditions ---
    out.preconditions = base.preconditions;
    if (override) {
        for (auto& [k, v] : override->preconditions) {
            out.preconditions[k] = v;
        }
    }

    // --- Effects ---
    out.effects = base.effects;
    if (override) {
        for (auto& [k, v] : override->effects) {
            out.effects[k] = v;
        }
    }

    return out;
}

/**
 * Builds a new plan to satisfy the current active goal.
 *
 * Process:
 *   - Interrupts any currently running action
 *   - Initialize open list with every action valid in current world state
 *   - Perform A* search
 *   - For each node: apply action effects and expand valid next actions
 *   - If goal satisfied -> reconstruct plan
 */
void Goap::update_plan(Entity entity, GoapAgent& agent, WorldState& ws) {
    /**
     * Internal search node used for A*.
     *
     * A node represents choosing 1 action in the action graph.
     * Links backwards to parent to build the full plan.
     */
    struct Node {
        GoapAction* action;
        float cost_so_far;
        float heuristic;
        Node* parent;

        Node(GoapAction* a, float cost, float h, Node* p) : action(a), cost_so_far(cost), heuristic(h), parent(p) {}

        float total_cost() const { return cost_so_far + heuristic; }
    };
    // Stop any running action before replanning
    if (agent.current_action) {
        agent.current_action->on_interrupt(entity);
        agent.current_action->is_running = false;
        agent.current_action = nullptr;
    }

    agent.clear_plan();

    // Keep all nodes inside a vector
    std::vector<Node> nodes;
    nodes.reserve(64);  // avoid vector reallocation -> prevents pointer invalidation

    using NodePtr = Node*;

    // Min-heap priority queue by total_cost
    auto cmp = [](NodePtr a, NodePtr b) { return a->total_cost() > b->total_cost(); };
    std::priority_queue<NodePtr, std::vector<NodePtr>, decltype(cmp)> open_list(cmp);

    // Closed set: storing only raw pointers to GoapAction
    // Tracks visited actions (prevents re-expansion loops)
    std::unordered_set<GoapAction*> closed;
    closed.reserve(agent.available_actions.size());

    // Log all available actions
    if (show_logging) {
        Log::info("GOAP: Agent {} has {} actions:", int(entity), int(agent.available_actions.size()));
        for (auto& action_ptr : agent.available_actions) {
            if (!action_ptr) continue;
            Log::info("  - {}", action_ptr->get_id());
        }
    }

    for (GoapAction* action : agent.available_actions) {
        if (!action) continue;

        const auto* override = action_overrides.find(action->get_id());
        EffectiveGoapAction effective = build_effective_action(*action, override);

        bool satisfied = true;
        for (auto& [fact, val] : effective.preconditions) {
            uint32_t fact_id = (uint32_t)std::hash<std::string>()(fact);
            auto it = ws.facts.find(fact_id);

            if (it == ws.facts.end() || it->second != val) {
                satisfied = false;
                break;
            }
        }

        if (satisfied) {
            nodes.emplace_back(action, effective.cost, 0.f, nullptr);
            open_list.push(&nodes.back());

            if (show_logging) Log::info("GOAP: Starting action '{}' satisfies preconditions, added to open list", action->get_id());
        } else {
            if (show_logging) Log::info("GOAP: Action '{}' does not satisfy preconditions", action->get_id());
        }
    }

    Node* goal_node = nullptr;
    int iteration = 0;

    // ------------------------------------------------------
    // A* search loop
    // ------------------------------------------------------
    while (!open_list.empty()) {
        Node* current = open_list.top();
        open_list.pop();
        iteration++;

        if (!current) continue;

        // Skip if already expanded
        if (closed.count(current->action)) continue;
        closed.insert(current->action);

        // Simulate effects, by applying this action's effects to get new world state
        WorldState temp = ws;
        current->action->apply_effects(temp);

        // Check if this satisfies the goal
        if (temp.satisfies(agent.active_goal.desired_state)) {
            goal_node = current;
            if (show_logging) {
                Log::info("GOAP: Goal satisfied by action '{}'", current->action->get_id());
            }
            break;
        }

        // Expand children: all actions whose preconditions match temp state
        for (GoapAction* next : agent.available_actions) {
            if (!next) continue;
            if (closed.count(next)) continue;

            const auto* override = action_overrides.find(next->get_id());
            EffectiveGoapAction effective = build_effective_action(*next, override);

            bool satisfied = true;
            for (auto& [fact, val] : effective.preconditions) {
                auto it = temp.facts.find((uint32_t)std::hash<std::string>()(fact));
                if (it == temp.facts.end() || it->second != val) {
                    satisfied = false;
                    break;
                }
            }

            if (satisfied) {
                float new_cost = current->cost_so_far + effective.cost;
                nodes.emplace_back(next, new_cost, 0.f, current);
                open_list.push(&nodes.back());
                if (show_logging) Log::info("GOAP: Adding child action '{}' to open list (cost: {})", next->get_id(), new_cost);
            } else {
                if (show_logging) Log::info("GOAP: Child action '{}' preconditions not satisfied at this node", next->get_id());
            }
        }
    }

    // Reconstruct plan
    if (!goal_node) {
        Log::warn("GOAP: planning FAILED for agent {} after {} iterations", entity, iteration);
        return;
    }

    std::vector<GoapAction*> plan;

    for (Node* n = goal_node; n; n = n->parent) plan.push_back(n->action);

    std::reverse(plan.begin(), plan.end());

    agent.plan = plan;
    agent.current_index = 0;
    agent.needs_replan = false;

    if (show_logging) {
        Log::info("GOAP: Plan created for agent {} with {} steps", entity, int(plan.size()));
        for (int i = 0; i < (int)plan.size(); ++i) {
            Log::info("  Step {}: {}", i, plan[i]->get_id());
        }
    }
}

// ------------------------------------------------------
// Action execution
// ------------------------------------------------------
/**
 * Executes the current action in the plan.
 *
 * Logic:
 *   - If no current action -> start next one
 *   - If preconditions break -> interrupt and force replanning
 *   - Tick action logic
 *   - If finished:
 *       - apply effects
 *       - move to next action
 *       - if at end -> mark plan as complete
 */
void Goap::update_action(Entity entity, GoapAgent& agent, WorldState& ws) {
    if (agent.plan.empty()) {
        if (show_logging) {
            Log::info("GOAP No plan for agent {}", entity);
        }
        return;
    }

    // Pick new action if none running
    GoapAction* action = agent.current_action;

    if (!action) {
        action = agent.plan[agent.current_index];
        agent.current_action = action;

        if (show_logging) {
            Log::info("GOAP {} START (index = {})", action->get_id(), agent.current_index);
        }
        action->is_running = true;
        action->on_start(entity);
    }

    // If world state changed and now invalidates preconditions -> interrupt
    if (!action->check_preconditions(ws)) {
        if (show_logging) {
            Log::info("GOAP {} INTERRUPT (preconditions failed)", action->get_id());
        }
        action->on_interrupt(entity);

        action->is_running = false;
        agent.current_action = nullptr;
        agent.plan.clear();
        agent.needs_replan = true;
        return;
    }

    // NO LONGER TICKING ACTION HERE

    // Check if action is completed
    if (action->is_done(entity)) {
        if (show_logging) {
            Log::info("GOAP {} FINISHED", action->get_id());
        }

        action->apply_effects(ws);
        action->on_finished(entity);
        action->is_running = false;

        agent.current_index++;
        agent.current_action = nullptr;

        // Check if plan is completed
        if (agent.current_index >= (int)agent.plan.size()) {
            if (show_logging) {
                Log::info("GOAP Plan completed for agent {}", entity);
            }
            agent.needs_replan = true;
        }
    }
}

}  // namespace tmt
