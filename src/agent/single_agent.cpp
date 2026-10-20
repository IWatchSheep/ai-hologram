// Purpose: Builds C1's state and runs the order of its decision step
// (plan 3.6), apart from the checking and choosing details split into
// single_agent_check.cpp and single_agent_choose.cpp.
// Introduced in Stage 3.
// Architecture: decide() follows plan 3.6 step by step. C1 never reads
// Environment; it knows the world only through Observations, and every
// prediction goes through state.predictor (the StepPredictor).
// Stage 4 IE-1 H1 adds the activation-hook calls in decide() below.

#include "agent/single_agent.h"
#include "agent/agent_log.h"

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

}  // namespace

SingleAgentState::SingleAgentState(WorldState startBelief,
                                   AgentSettings settingsValue,
                                   AgentActionModel model,
                                   std::vector<Position> notDead)
    : settings(settingsValue),
      belief(startBelief),
      notDeadOverrides(std::move(notDead)),
      agentModel(std::move(model)),
      // The predictor's model asks agentModel for its current model on
      // every call, so restoring an action later takes effect at once.
      predictor(WorldModel{ActionModel{[this](const WorldState& state,
                                              Action action) {
                  return agentModel.current()(state, action);
              }}},
                settingsValue.budgetPerStep) {
}

SingleAgent::SingleAgent(const Observation& first, AgentSettings settings,
                         std::optional<WorldState> startingBelief,
                         std::optional<ActionModel> baseModel,
                         std::vector<Position> notDeadOverrides)
    : state(beliefOrFirst(first, startingBelief), settings,
            modelOrDefault(baseModel), std::move(notDeadOverrides)) {
    state.history.push_back(first);  // o_1 is history[0] (plan 3.6 step 1)
}

const std::vector<std::string>& SingleAgent::logLines() const {
    return state.log;
}

void SingleAgent::addLog(const std::string& line) { state.log.push_back(line); }

int SingleAgent::currentReserve() const {
    int reserve = 1;  // the committed prediction always keeps one (3.4)
    for (const CorrectionRecord& record : state.corrections) {
        if (record.status == CorrectionStatus::Consistent &&
            record.element.kind != ElementKind::Trap) {
            ++reserve;  // one V2 alternative per pending correction
        }
    }
    return reserve;
}

const DeadSquareMap& SingleAgent::currentMap() {
    // The cache reruns the analysis only when walls or overrides changed.
    return state.deadCache.get(state.belief, state.notDeadOverrides,
                               state.deadOverrides, state.freeCounts);
}

CorrectionRecord* SingleAgent::findCorrection(int id) {
    for (CorrectionRecord& record : state.corrections) {
        if (record.id == id) return &record;
    }
    return nullptr;
}

CorrectionRecord& SingleAgent::addCorrectionRecord(CorrectionRecord record) {
    record.id = state.nextCorrectionId++;
    state.corrections.push_back(record);
    return state.corrections.back();
}

Decision SingleAgent::makeSafeStop(const std::string& reason) {
    ++state.safeStops;
    state.committed.has = false;  // a safe stop has no committed prediction
    addLog(safeStopLine(state.decisionStep, reason));
    return Decision{false, Action::North, reason};
}

void SingleAgent::applyCorrection(const CorrectionRecord& correction) {
    const Element& element = correction.element;
    if (element.kind == ElementKind::Wall && isInsideGrid(element.cell)) {
        state.belief.walls[static_cast<std::size_t>(element.cell.y)]
                          [static_cast<std::size_t>(element.cell.x)] =
                              correction.newBlocked;
    } else if (element.kind == ElementKind::Object) {
        state.belief.object = correction.newPosition;
    } else if (element.kind == ElementKind::ActionModel) {
        state.agentModel.restore(element.action);
        state.predictor.clearCache();
    } else if (element.kind == ElementKind::Trap) {
        bool listed = false;
        for (Position cell : state.deadOverrides) listed |= cell == element.cell;
        if (!listed) state.deadOverrides.push_back(element.cell);
        state.predictor.clearCache();
    }
}

void SingleAgent::revertCorrection(const CorrectionRecord& correction) {
    const Element& element = correction.element;
    if (element.kind == ElementKind::Wall && isInsideGrid(element.cell)) {
        state.belief.walls[static_cast<std::size_t>(element.cell.y)]
                          [static_cast<std::size_t>(element.cell.x)] =
                              correction.oldBlocked;
    } else if (element.kind == ElementKind::ActionModel) {
        state.agentModel.unrestore(element.action);
        state.predictor.clearCache();
    } else if (element.kind == ElementKind::Trap) {
        std::vector<Position> kept;
        for (Position cell : state.deadOverrides) {
            if (cell != element.cell) kept.push_back(cell);
        }
        state.deadOverrides = kept;
        state.predictor.clearCache();
    }
    // An object correction is only marked: its old value is in the past.
}

Decision SingleAgent::decide(const Observation& observation) {
    ++state.decisionStep;
    state.predictor.startStep();  // new budget B; the cache is cleared
    if (state.decisionStep != 1) {
        state.history.push_back(observation);
    }
    if (state.committed.has) checkCommitted(observation);  // step 2
    if (const auto stopped = retryDeferred()) {             // step 3
        // A step that ends early in step 3 still has its activation
        // point; otherwise an event scheduled for that step would
        // never be tried (plan 3.6, IE-1 H1).
        if (state.activationHook) {
            state.activationHook(state.decisionStep);
        }
        return *stopped;
    }
    continuePendingWork();  // step 3
    // Step 4: the fault activation point (protocol 11.3 step 4; plan
    // 3.6, IE-1 H1). Empty except in a run with faults, when the
    // experimenter's fault injector is called here once per step,
    // after step 3 and before planning. The agent reads nothing back.
    if (state.activationHook) {
        state.activationHook(state.decisionStep);
    }
    return chooseAndCommit();  // steps 5-7 in the prompt's order
}

void SingleAgent::finalCheck(const Observation& observation) {
    state.predictor.startStep();
    state.history.push_back(observation);
    if (state.committed.has) checkCommitted(observation);  // step 2 only
}

int SingleAgent::mismatchCount() const {
    return static_cast<int>(state.mismatches.size());
}

int SingleAgent::correctionCount() const {
    return static_cast<int>(state.corrections.size());
}

int SingleAgent::verifiedCount() const {
    int verified = 0;
    for (const CorrectionRecord& record : state.corrections) {
        if (record.status == CorrectionStatus::Verified) ++verified;
    }
    return verified;
}

int SingleAgent::safeStopCount() const { return state.safeStops; }

int SingleAgent::totalEvaluations() const {
    return state.predictor.usedTotal();
}

}  // namespace ai_hologram
