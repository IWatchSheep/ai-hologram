// Purpose: Implements the main-search part of C1's choice (plan 3.6
// step 5 in the prompt's numbering): the main search, the
// consequence check after an Exhausted search, and the fallback.
// Introduced in Stage 3.
// Architecture: Split from single_agent_choose.cpp for length. Every
// successor state here comes from the StepPredictor through the
// planner; no successor is made from the rules directly.

#include "agent/agent_log.h"
#include "agent/heuristic.h"
#include "agent/single_agent.h"

namespace ai_hologram {

// Runs the main search with the main goal (plan 3.6 step 5, in the
// prompt's numbering): Found becomes the plan, Exhausted first runs
// the consequence check, and the fallback is used otherwise.
Decision SingleAgent::runMainSearch(const DeadSquareMap& map, int reserve) {
    const auto goal = [](const WorldState& state) { return state.object == state.destination; };
    const auto estimate = [this, &map](const WorldState& candidate) {
        return heuristic(candidate, map, state.freeCounts);
    };
    const SearchResult result = search(state.belief, map, goal, estimate, state.predictor, reserve);
    if (result.kind == SearchKind::Found) {
        if (result.plan.empty()) {
            return makeSafeStop("belief shows goal reached");
        }
        state.plan = result.plan;
        state.planKind = PlanKind::Goal;
        addLog(planSearchLine(static_cast<int>(state.plan.size())));
    } else {
        if (result.kind == SearchKind::Exhausted) handleExhaustedConsequence(map);
        if (const auto stopped = chooseFallbackPlan(result)) return *stopped;
    }
    const auto predicted = state.predictor.predict(state.belief, state.plan.front().action);
    if (!predicted.has_value()) {
        state.plan.clear();
        state.planKind = PlanKind::None;
        return makeSafeStop("no budget for a committed prediction");
    }
    return commitAction();
}

// Handles a possible consequence surprise after an Exhausted main
// search (plan 3.7 B): logs the surprise, then either the
// full-state dead-end line or the trap correction and its V2 test.
void SingleAgent::handleExhaustedConsequence(const DeadSquareMap& map) {
    const Position p = state.belief.object;
    const ConsequenceOutcome consequence = checkConsequence(state.belief, map, state.history, state.freeCounts);
    if (consequence == ConsequenceOutcome::FullStateDeadEnd) {
        addLog(consequenceSurpriseLine(p));
        addLog(fullStateDeadEndLine());
    } else if (consequence == ConsequenceOutcome::Mismatch) {
        addLog(consequenceSurpriseLine(p));
        addLog(consequenceMismatchLine(p));
        CorrectionRecord record;
        record.element.kind = ElementKind::Trap;
        record.element.cell = p;
        record.status = CorrectionStatus::Consistent;
        record.stepConsistent = state.decisionStep;
        CorrectionRecord& added = addCorrectionRecord(record);
        applyCorrection(added);
        addLog(correctionTrapLine(added.id, p));
        attemptTrapV2(added.id, p);
    }
}

// Uses the planner fallback after OutOfBudget or Exhausted (plan
// 3.5 and 3.6): an empty fallback safe-stops, otherwise the best
// path found becomes the plan and nothing is returned.
std::optional<Decision> SingleAgent::chooseFallbackPlan(const SearchResult& result) {
    const std::vector<PlanStep> best = fallback(result);
    if (best.empty()) {
        return makeSafeStop(result.kind == SearchKind::OutOfBudget ? "budget too small to plan" : "no safe action");
    }
    state.plan = best;
    state.planKind = PlanKind::Fallback;
    addLog(planFallbackLine(static_cast<int>(state.plan.size())));
    return std::nullopt;
}

}  // namespace ai_hologram
