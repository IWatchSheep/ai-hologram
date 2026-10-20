// Purpose: Declares C0, the naive single agent used only as a
// reference for C1 (plan 3.11, protocol Phase B 11.8).
// Introduced in Stage 3.
// Architecture: C0 is built from the same plain structs and free
// functions as C1 (D-019): the same StepPredictor (limit B), the same
// planner, main heuristic and dead-square analysis. It differs only
// in what it does with observations: it never reads any observation
// after the first, so a wrong belief is never noticed or corrected.
// Stage 4 IE-1 H1 adds the activation hook below (fault activation point).

#pragma once

#include "agent/agent_model.h"
#include "agent/dead_squares.h"
#include "agent/observation.h"
#include "agent/planner.h"
#include "agent/single_agent.h"
#include "agent/step_predictor.h"

#include <functional>
#include <optional>
#include <vector>

namespace ai_hologram {

// All of C0's state, as one plain public struct (D-019), so tests can
// build and inspect any state directly. The predictor's model asks
// this state's AgentActionModel for its current model on every call,
// just as for C1; because that model refers back to this state,
// copying is not allowed.
struct NaiveAgentState {
    NaiveAgentState(WorldState startBelief, AgentSettings settings,
                    AgentActionModel model);
    NaiveAgentState(const NaiveAgentState&) = delete;
    NaiveAgentState& operator=(const NaiveAgentState&) = delete;

    AgentSettings settings;
    WorldState belief;
    DeadSquareCache deadCache;
    FreeComputationCounts freeCounts;
    std::vector<Position> notDeadOverrides;
    std::vector<Position> deadOverrides;
    AgentActionModel agentModel;
    StepPredictor predictor;
    std::vector<PlanStep> plan;
    int decisionStep = 0;
    int searches = 0;
    // The fault activation point (protocol 11.3 step 4; plan 3.6, IE-1 H1).
    // Empty in every agent run except a run with faults (Stage 4):
    // then the experimenter's fault injector is called here once per
    // decision step. The agent never reads anything from it.
    std::function<void(int)> activationHook;
};

class NaiveAgent {
public:
    // C0 exists only as a naive reference: it plans from its belief
    // and then follows the plan blindly. Without a starting belief,
    // the belief is the first observation's state. A hand-written
    // starting belief and a base model are supplied by tests and by
    // the demonstration only; this is not fault injection.
    NaiveAgent(const Observation& first, AgentSettings settings,
               std::optional<WorldState> startingBelief = std::nullopt,
               std::optional<ActionModel> baseModel = std::nullopt);
    NaiveAgent(const NaiveAgent&) = delete;
    NaiveAgent& operator=(const NaiveAgent&) = delete;

    // Always returns an action: C0 makes no checks, investigations,
    // corrections, claims, log lines or safe stops (plan 3.11).
    Decision decide(const Observation& observation);

    int totalEvaluations() const;
    int searchCount() const;  // how many times `search` was called

    NaiveAgentState state;

private:
    // Searches (reserve 0) and then the fallback from the current
    // belief; if neither gives an action, keeps one no-plan action.
    void planFromBelief();
    // The first action (North, East, South, West) predicted Moved or
    // Pushed from the belief, or North with the belief itself as the
    // expected state when no action qualifies (D-026).
    PlanStep firstMovingOrNorth();
};

}  // namespace ai_hologram
