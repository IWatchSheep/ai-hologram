// Purpose: Implements C1's action choice (plan 3.6 step 6, in the prompt's
// numbering) and the commit with its V2 bookkeeping (step 7).
// Introduced in Stage 3.
// Architecture: Split from single_agent.cpp for length. Every successor
// state here comes from the StepPredictor, never from the rules directly.
// Stage 4 IE-1 H4 addition: the objectDead read goes through readDeadStatus.

#include "agent/agent_log.h"
#include "agent/heuristic.h"
#include "agent/single_agent.h"
#include "world/rules.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace ai_hologram {
namespace {

int manhattan(Position first, Position second) {
    return std::abs(first.x - second.x) + std::abs(first.y - second.y);
}

bool objectDead(const WorldState& state, const DeadSquareMap& map) {
    return isInsideGrid(state.object) && readDeadStatus(map, state.object);
}
const std::array<Action, 4> kOrder = {Action::North, Action::East,
                                      Action::South, Action::West};

}  // namespace

std::optional<PlanStep> SingleAgent::chooseCautiousAction() {
    const DeadSquareMap& map = currentMap();
    const int reserve = currentReserve();
    for (const Action action : kOrder) {
        // One evaluation for the standard-rule variant, plus one more
        // if the current-model prediction is not already cached.
        const int need = state.predictor.isCached(state.belief, action) ? 1 : 2;
        if (state.predictor.remaining() - need < reserve) break;
        const auto current = state.predictor.predict(state.belief, action);
        if (!current.has_value()) break;
        const auto standard = state.predictor.predictVariant(
            state.belief, action, ActionModel{&applyAction});
        if (!standard.has_value()) break;
        if (!predictionsDiffer(*current, *standard) && !objectDead(current->state, map)) {
            return PlanStep{action, current->state};
        }
    }
    return std::nullopt;
}

bool SingleAgent::tryActiveTest(const DeadSquareMap& map, int reserve) {
    for (const CorrectionRecord& record : state.corrections) {
        if (record.status != CorrectionStatus::Consistent ||
            record.element.kind != ElementKind::Wall ||
            state.decisionStep - record.stepConsistent <
                state.settings.activeTestDelay) {
            continue;
        }
        if (tryTestForCorrection(record, map, reserve)) return true;
    }
    return false;
}

// Searches for a walk to one test cell of one wall correction
// (plan 3.8, active test): the goal is reaching a target cell and
// the estimate is the smallest manhattan distance to a target.
bool SingleAgent::tryTestForCorrection(const CorrectionRecord& record,
                                       const DeadSquareMap& map, int reserve) {
    const auto targets = activeTestTargets(state.belief, record.element.cell);
    if (targets.empty()) return false;
    const auto goal = [&targets](const WorldState& state) {
        for (const auto& target : targets) {
            if (state.agent == target.first) return true;
        }
        return false;
    };
    const auto estimate = [&targets, &map](const WorldState& state) {
        if (objectDead(state, map)) return kInfiniteHeuristic;
        int best = kInfiniteHeuristic;
        for (const auto& target : targets) {
            best = std::min(best, manhattan(state.agent, target.first));
        }
        return best;
    };
    const SearchResult result = search(state.belief, map, goal, estimate, state.predictor, reserve);
    if (result.kind != SearchKind::Found) return false;
    return finishActiveTest(record, targets, result, reserve);
}

// Adds the test move into the corrected cell to a found walk and
// makes it the plan (plan 3.8, active test). The test move's
// prediction is made only if it keeps the reserve (or is cached).
bool SingleAgent::finishActiveTest(
    const CorrectionRecord& record,
    const std::vector<std::pair<Position, Action>>& targets,
    const SearchResult& result, int reserve) {
    const WorldState end = result.plan.empty() ? state.belief : result.plan.back().expected;
    for (const auto& target : targets) {
        if (target.first != end.agent) continue;
        if (!state.predictor.isCached(end, target.second) &&
            state.predictor.remaining() <= reserve) {
            continue;
        }
        const auto test = state.predictor.predict(end, target.second);
        if (!test.has_value()) continue;
        state.plan = result.plan;
        state.plan.push_back(PlanStep{target.second, test->state});
        state.planKind = PlanKind::Test;
        addLog(planActiveTestLine(record.id, static_cast<int>(state.plan.size())));
        return true;
    }
    return false;
}

Decision SingleAgent::commitAction() {
    const Action action = state.plan.front().action;
    const auto predicted = state.predictor.predict(state.belief, action);
    if (!predicted.has_value()) {
        state.plan.clear();
        state.planKind = PlanKind::None;
        return makeSafeStop("no budget for a committed prediction");
    }
    state.plan.erase(state.plan.begin());
    if (state.plan.empty()) state.planKind = PlanKind::None;
    std::vector<int> tested;
    for (CorrectionRecord& record : state.corrections) {
        if (record.status != CorrectionStatus::Consistent || record.tested ||
            state.predictor.remaining() <= 0) {
            continue;
        }
        const auto alternative = alternativePrediction(
            record, state.belief, action, state.predictor,
            state.agentModel.current(), state.agentModel.base());
        if (alternative.has_value() && predictionsDiffer(*alternative, *predicted)) {
            record.tested = true;
            tested.push_back(record.id);
        }
    }
    state.committed.has = true;
    state.committed.prediction = *predicted;
    state.committed.beliefBefore = state.belief;  // the belief BEFORE acting
    state.committed.decisionStep = state.decisionStep;
    state.committed.testedCorrectionIds = tested;
    return Decision{true, action, ""};
}

Decision SingleAgent::chooseAndCommit() {
    // Step 1: cautious mode while V1 work is unfinished (plan 3.9).
    if (state.pendingV1.has_value()) {
        if (const auto cautious = chooseCautiousAction()) {
            addLog(planCautiousLine());
            state.plan = {*cautious};
            state.planKind = PlanKind::Goal;
            return commitAction();  // committed prediction (step 5)
        }
        return makeSafeStop("investigation unfinished, no cautious action");
    }
    const int reserve = currentReserve();
    const DeadSquareMap& map = currentMap();
    // Step 2: active test of an old-enough wall correction (plan 3.8).
    if (state.planKind != PlanKind::Test && tryActiveTest(map, reserve)) {
        return commitAction();  // committed prediction (step 5)
    }
    // Step 3: keep the plan if its next step still fits (plan 3.6).
    if (const auto kept = tryKeepCurrentPlan(map)) return *kept;
    // Step 4: main search, consequence check and fallback, in
    // single_agent_search.cpp; it commits its first action itself.
    return runMainSearch(map, reserve);
}

// Keeps the current plan when its next action still predicts the
// expected state and does not put the object on a dead square
// (plan 3.6, "keep the plan"). Returns a decision when the plan is
// kept or no committed prediction can be made; otherwise clears
// the plan and returns nothing, so the caller searches afresh.
std::optional<Decision> SingleAgent::tryKeepCurrentPlan(const DeadSquareMap& map) {
    if (state.plan.empty()) return std::nullopt;
    const auto predicted = state.predictor.predict(state.belief, state.plan.front().action);
    if (!predicted.has_value()) {
        state.plan.clear();
        state.planKind = PlanKind::None;
        return makeSafeStop("no budget for a committed prediction");
    }
    if (predicted->state == state.plan.front().expected && !objectDead(predicted->state, map)) {
        return commitAction();
    }
    state.plan.clear();
    state.planKind = PlanKind::None;
    return std::nullopt;
}

}  // namespace ai_hologram
