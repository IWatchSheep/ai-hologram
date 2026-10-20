// Purpose: Implements the mismatch investigation and the
// consequence check of plan 3.7.
// Introduced in Stage 3.
// Architecture: The only prediction made here is the investigation
// re-prediction, a variant prediction through the StepPredictor
// with the agent's CURRENT model (plan 3.8 table), costing one
// budgeted evaluation. Everything else is evidence lookups in the
// recorded history. Nothing here changes a belief: the decision
// step (sub-stage 3e) applies updates and creates records.
// Stage 4 IE-1 H4 addition: the checkConsequence read goes through readDeadStatus.

#include "agent/investigation.h"
#include "agent/footprint.h"
#include "agent/verification.h"

#include <cstddef>

namespace ai_hologram {
namespace {

// Finds the belief candidates of plan 3.7 A.2 by comparing the
// believed pre-action footprint contents with o_t-1, the recorded
// observation of the same moment. Walls come first, in footprint
// order (c0, c1, c2), then the object candidate.
std::vector<BeliefCandidate> findCandidates(
    const WorldState& beliefBefore, Action action,
    const Observation& before, const Observation& after) {
    const MoveOutcome observedOutcome =
        after.lastOutcome.value_or(MoveOutcome::Blocked);
    const std::vector<Position> cells =
        computeFootprint(beliefBefore, action, after.state, observedOutcome);
    std::vector<BeliefCandidate> candidates;
    for (const Position cell : cells) {
        if (isWall(beliefBefore, cell) == isWall(before.state, cell)) continue;
        BeliefCandidate candidate;
        candidate.element.kind = ElementKind::Wall;
        candidate.element.cell = cell;
        candidate.blocked = isWall(before.state, cell);
        candidates.push_back(candidate);
    }
    bool objectInFootprint = false;
    for (const Position cell : cells) {
        if (beliefBefore.object == cell || before.state.object == cell) {
            objectInFootprint = true;
        }
    }
    if (objectInFootprint && beliefBefore.object != before.state.object) {
        BeliefCandidate candidate;
        candidate.element.kind = ElementKind::Object;
        candidate.position = before.state.object;
        candidates.push_back(candidate);
    }
    return candidates;
}

// Applies every candidate to a copy of the belief before the
// action: the corrected belief of plan 3.7 A.3.
WorldState correctedBelief(WorldState belief,
                           const std::vector<BeliefCandidate>& candidates) {
    for (const BeliefCandidate& candidate : candidates) {
        if (candidate.element.kind == ElementKind::Wall) {
            const Position cell = candidate.element.cell;
            if (isInsideGrid(cell)) {
                belief.walls[static_cast<std::size_t>(cell.y)]
                            [static_cast<std::size_t>(cell.x)] =
                                candidate.blocked;
            }
        } else if (candidate.element.kind == ElementKind::Object) {
            belief.object = candidate.position;
        }
    }
    return belief;
}

// Runs each candidate's V1 history check (plan 3.8 table): walls
// against EVERY recorded observation (walls never change), the
// object only against o_t-1, because the object moves and earlier
// positions must never be compared with a later value.
bool historyChecksPass(const std::vector<BeliefCandidate>& candidates,
                       const std::vector<Observation>& history,
                       const Observation& before) {
    for (const BeliefCandidate& candidate : candidates) {
        if (candidate.element.kind == ElementKind::Wall &&
            !wallHistoryCheck(candidate.element.cell, candidate.blocked,
                              history)) {
            return false;
        }
        if (candidate.element.kind == ElementKind::Object &&
            !objectHistoryCheck(candidate.position, before)) {
            return false;
        }
    }
    return true;
}

// The history check of plan 3.7 B.3: no recorded observation shows
// the object at p with a later one showing it on the destination.
bool trapHistoryPasses(Position p, Position destination,
                       const std::vector<Observation>& history) {
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history[i].state.object != p) continue;
        for (std::size_t j = i + 1; j < history.size(); ++j) {
            if (history[j].state.object == destination) return false;
        }
    }
    return true;
}

// Verifies all belief candidates together (plan 3.7 A.3): the free
// history checks first, then one variant re-prediction from the
// corrected belief. That optional re-prediction must keep the
// reserve, so it returns Deferred when it cannot be afforded.
InvestigationOutcome verifyCandidates(
    const WorldState& beliefBefore, Action action,
    const std::vector<BeliefCandidate>& candidates,
    const std::vector<Observation>& history, const Observation& before,
    const Observation& after, StepPredictor& predictor,
    const ActionModel& currentModel, int reserve) {
    if (!historyChecksPass(candidates, history, before) ||
        !isValidWorld(correctedBelief(beliefBefore, candidates))) {
        return InvestigationOutcome::NoCandidatePassed;
    }
    if (predictor.remaining() <= reserve) {
        return InvestigationOutcome::Deferred;
    }
    const WorldState corrected = correctedBelief(beliefBefore, candidates);
    const auto predicted = predictor.predictVariant(corrected, action, currentModel);
    if (!predicted.has_value()) return InvestigationOutcome::Deferred;
    const MoveOutcome observedOutcome = after.lastOutcome.value_or(MoveOutcome::Blocked);
    const std::vector<Position> cells = computeFootprint(corrected, action, after.state, observedOutcome);
    const FootprintComparison comparison = compareFootprint(
        predicted->state, predicted->outcome, after.state, observedOutcome, cells);
    if (!comparison.outcomesDiffer && comparison.cells.empty()) {
        return InvestigationOutcome::BeliefCandidatesConsistent;
    }
    return InvestigationOutcome::NoCandidatePassed;
}

}  // namespace

InvestigationResult investigateMismatch(
    const WorldState& beliefBefore, const Action action,
    const std::vector<Observation>& history, StepPredictor& predictor,
    const ActionModel& currentModel, const int reserve) {
    InvestigationResult result;
    if (history.size() < 2) return result;  // needs o_t-1 and o_t
    const Observation& before = history[history.size() - 2];
    const Observation& after = history[history.size() - 1];
    // A.1: the believed agent is never a fault target, so a
    // difference from the record is an anomaly (a possible bug).
    result.anomaly = beliefBefore.agent != before.state.agent;
    result.candidates = findCandidates(beliefBefore, action, before, after);
    if (result.candidates.empty()) {
        // A.4: the belief matched the record, so the action model
        // for this action is suspected. No evaluation is used.
        result.outcome = InvestigationOutcome::ModelSuspected;
        result.suspected = SourceLabel::S1;
        return result;
    }
    result.suspected = SourceLabel::S2;  // belief candidates (A.2)
    // A.3: verify all candidates TOGETHER (see verifyCandidates).
    result.outcome = verifyCandidates(
        beliefBefore, action, result.candidates, history, before, after,
        predictor, currentModel, reserve);
    if (result.outcome == InvestigationOutcome::NoCandidatePassed) {
        result.suspected = SourceLabel::S5;
    }
    return result;
}

ConsequenceOutcome checkConsequence(
    const WorldState& belief, const DeadSquareMap& map,
    const std::vector<Observation>& history, FreeComputationCounts& counts) {
    const Position p = belief.object;
    if (!isInsideGrid(p)) return ConsequenceOutcome::NotApplicable;
    const bool dead = readDeadStatus(map, p);
    // B: only a cell believed neither dead nor the destination can
    // be a surprise at all. (For use after an Exhausted search; the
    // search itself is the caller's, in sub-stage 3e.)
    if (dead || p == belief.destination) {
        return ConsequenceOutcome::NotApplicable;
    }
    if (localCheckSaysNotDead(belief, map, p, counts)) {
        return ConsequenceOutcome::FullStateDeadEnd;  // B.2: no claim
    }
    if (!trapHistoryPasses(p, belief.destination, history)) {
        return ConsequenceOutcome::NotApplicable;
    }
    return ConsequenceOutcome::Mismatch;  // B.3: trap(p) = dead, S1
}

}  // namespace ai_hologram
