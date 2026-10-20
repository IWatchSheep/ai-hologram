// Purpose: Implements C0, the naive single agent (plan 3.11).
// Introduced in Stage 3.
// Architecture: C0 shares C1's StepPredictor, planner, main heuristic
// and dead-square analysis, and every prediction goes through the
// StepPredictor. It never checks a prediction against an observation,
// so there is no investigation, correction or safe stop here.
// Stage 4 IE-1 H1 adds the activation-hook call in decide() below.

#include "agent/naive_agent.h"
#include "agent/heuristic.h"

#include <array>
#include <utility>

namespace ai_hologram {
namespace {

WorldState beliefOrFirst(const Observation& first,
                         const std::optional<WorldState>& starting) {
    return starting.has_value() ? *starting : first.state;
}

AgentActionModel modelOrDefault(const std::optional<ActionModel>& base) {
    return base.has_value() ? AgentActionModel(*base) : AgentActionModel();
}

// The fixed order in which C0 tries actions when it has no plan.
const std::array<Action, 4> kOrder = {Action::North, Action::East,
                                      Action::South, Action::West};

}  // namespace

NaiveAgentState::NaiveAgentState(WorldState startBelief,
                                 AgentSettings settingsValue,
                                 AgentActionModel model)
    : settings(settingsValue),
      belief(startBelief),
      agentModel(std::move(model)),
      // The predictor's model asks agentModel for its current model
      // on every call, exactly as for C1 (see single_agent.cpp).
      predictor(WorldModel{ActionModel{[this](const WorldState& state,
                                              Action action) {
                  return agentModel.current()(state, action);
              }}},
                settingsValue.budgetPerStep) {
}

NaiveAgent::NaiveAgent(const Observation& first, AgentSettings settings,
                       std::optional<WorldState> startingBelief,
                       std::optional<ActionModel> baseModel)
    : state(beliefOrFirst(first, startingBelief), settings,
            modelOrDefault(baseModel)) {
}

PlanStep NaiveAgent::firstMovingOrNorth() {
    for (const Action action : kOrder) {
        const auto predicted = state.predictor.predict(state.belief, action);
        if (predicted.has_value() &&
            (predicted->outcome == MoveOutcome::Moved ||
             predicted->outcome == MoveOutcome::Pushed)) {
            return PlanStep{action, predicted->state};
        }
    }
    // No action is predicted Moved or Pushed (every one is Blocked,
    // or the budget cannot pay for a prediction). C0 has no safe
    // stops (D-026), so it takes North and expects nothing to change.
    return PlanStep{Action::North, state.belief};
}

void NaiveAgent::planFromBelief() {
    const DeadSquareMap& map = state.deadCache.get(
        state.belief, state.notDeadOverrides, state.deadOverrides,
        state.freeCounts);
    const auto goal = [](const WorldState& state) {
        return state.object == state.destination;
    };
    const auto estimate = [this, &map](const WorldState& candidate) {
        return heuristic(candidate, map, state.freeCounts);
    };
    // C0 has no mandatory predictions to protect, so the reserve is 0.
    const SearchResult result =
        search(state.belief, map, goal, estimate, state.predictor, 0);
    ++state.searches;
    if (result.kind == SearchKind::Found && !result.plan.empty()) {
        state.plan = result.plan;
        return;
    }
    // An empty Found plan (the belief already shows the goal) counts
    // as "no plan", like an empty fallback, so both fall through here.
    state.plan = fallback(result);
    if (state.plan.empty()) {
        state.plan = {firstMovingOrNorth()};
    }
}

Decision NaiveAgent::decide(const Observation& /*observation*/) {
    // The observation argument is deliberately never read, not even
    // the first time decide() is called after construction: C0 built
    // its belief from the first observation in its constructor and
    // never reads any observation after that (Phase B 11.8). This is
    // what "plans once, never checks predictions" means: with a wrong
    // belief, C0's belief drifts from the world and is never noticed.
    ++state.decisionStep;
    state.predictor.startStep();  // a new step budget of B (plan 3.11)
    // The fault activation point (plan 3.6, IE-1 H1): C0 never checks
    // or investigates, so its activation point is at the start of its
    // step, before it plans or takes its next plan action. Empty
    // except in a run with faults. The agent reads nothing back.
    if (state.activationHook) {
        state.activationHook(state.decisionStep);
    }
    if (state.plan.empty()) {
        // Plans only at the first decision and whenever its plan is
        // used up; executing a plan below makes no new predictions.
        planFromBelief();
    }
    // Executing means taking the next plan action and setting the
    // belief to that action's STORED expected state. There is no new
    // prediction for this, so it costs nothing (Phase B 11.8).
    const PlanStep next = state.plan.front();
    state.plan.erase(state.plan.begin());
    state.belief = next.expected;
    return Decision{true, next.action, ""};
}

int NaiveAgent::totalEvaluations() const {
    return state.predictor.usedTotal();
}

int NaiveAgent::searchCount() const { return state.searches; }

}  // namespace ai_hologram
