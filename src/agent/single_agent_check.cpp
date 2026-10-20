// Purpose: Implements the CHECK part of C1's decision step (plan 3.6 step 2),
// the deferred-investigation retry, and waiting trap / pending V1 work.
// Introduced in Stage 3.
// Architecture: Split from single_agent.cpp for length. Every prediction
// here goes through the StepPredictor; no successor is made any other way.

#include "agent/agent_log.h"
#include "agent/footprint.h"
#include "agent/single_agent.h"

namespace ai_hologram {
namespace {

std::vector<Position> footprintOf(const WorldState& beliefBefore,
                                  Action action, const Observation& after) {
    return computeFootprint(beliefBefore, action, after.state,
                            after.lastOutcome.value_or(MoveOutcome::Blocked));
}

}  // namespace

void SingleAgent::checkCommitted(const Observation& observation) {
    const CommittedPrediction committed = state.committed;
    state.committed.has = false;
    const WorldState& beliefBefore = committed.beliefBefore;
    const Action action = committed.prediction.action;
    const MoveOutcome observed =
        observation.lastOutcome.value_or(MoveOutcome::Blocked);
    const std::vector<Position> cells =
        footprintOf(beliefBefore, action, observation);
    const FootprintComparison comparison = compareFootprint(
        committed.prediction.state, committed.prediction.outcome,
        observation.state, observed, cells);
    const bool matched = !comparison.outcomesDiffer && comparison.cells.empty();
    addLog(stepLine(committed.decisionStep, action,
                    committed.prediction.outcome, observed, matched));
    for (const int id : committed.testedCorrectionIds) {
        CorrectionRecord* record = findCorrection(id);
        if (record == nullptr || record->status != CorrectionStatus::Consistent) {
            continue;
        }
        if (matched) {
            record->status = CorrectionStatus::Verified;
            addLog(correctionVerifiedLine(id));
        } else {
            record->status = CorrectionStatus::Rejected;
            addLog(correctionRejectedLine(id));
            revertCorrection(*record);  // the old value comes back (3.8)
        }
    }
    if (matched) {
        applyFootprintUpdate(state.belief, cells, observation.state);
        return;
    }
    const InvestigationResult result = investigateMismatch(
        beliefBefore, action, state.history, state.predictor,
        state.agentModel.current(), currentReserve());
    MismatchRecord mismatch;
    mismatch.decisionStep = state.decisionStep;
    mismatch.action = action;
    mismatch.differences = comparison;
    mismatch.suspected = result.suspected;
    mismatch.anomaly = result.anomaly;
    for (const BeliefCandidate& candidate : result.candidates) {
        mismatch.candidates.push_back(candidate.element);
    }
    state.mismatches.push_back(mismatch);
    handleInvestigation(result, beliefBefore, action, observation, true,
                        state.history);
}

void SingleAgent::handleInvestigation(const InvestigationResult& result,
                                      const WorldState& beliefBefore,
                                      Action action, const Observation& after,
                                      bool logLines,
                                      const std::vector<Observation>& historyUsed) {
    if (logLines) {
        if (result.candidates.empty() &&
            result.outcome == InvestigationOutcome::ModelSuspected) {
            addLog(investigationModelLine(action));
        }
        for (const BeliefCandidate& candidate : result.candidates) {
            if (candidate.element.kind == ElementKind::Wall) {
                addLog(investigationWallLine(
                    candidate.element.cell,
                    isWall(beliefBefore, candidate.element.cell),
                    candidate.blocked));
            } else {
                addLog(investigationObjectLine(beliefBefore.object,
                                               candidate.position));
            }
        }
        if (result.anomaly) addLog(anomalyAgentLine());
    }
    const std::vector<Position> cells = footprintOf(beliefBefore, action, after);
    if (result.outcome == InvestigationOutcome::BeliefCandidatesConsistent) {
        for (const BeliefCandidate& candidate : result.candidates) {
            CorrectionRecord record;
            record.element = candidate.element;
            record.newBlocked = candidate.blocked;
            record.oldBlocked = isWall(beliefBefore, candidate.element.cell);
            record.newPosition = candidate.position;
            record.oldPosition = beliefBefore.object;
            record.status = CorrectionStatus::Consistent;
            record.stepConsistent = state.decisionStep;
            CorrectionRecord& added = addCorrectionRecord(record);
            applyCorrection(added);
            if (added.element.kind == ElementKind::Wall) {
                addLog(correctionWallLine(added.id, added.element.cell,
                                          added.newBlocked));
            } else {
                addLog(correctionObjectLine(added.id, added.newPosition));
            }
        }
        applyFootprintUpdate(state.belief, cells, after.state);
    } else if (result.outcome == InvestigationOutcome::NoCandidatePassed) {
        addLog(unexplainedLine());
        applyFootprintUpdate(state.belief, cells, after.state);
    } else if (result.outcome == InvestigationOutcome::ModelSuspected) {
        // Provisional update at once (D-025); V1 work starts now and
        // continues in step 3 of this or a later decision step.
        applyFootprintUpdate(state.belief, cells, after.state);
        state.pendingV1 = startModelV1(action, historyUsed);
    } else {  // Deferred: nothing is updated; it is retried next step.
        if (logLines) addLog(investigationDeferredLine());
        state.deferred.has = true;
        state.deferred.beliefBefore = beliefBefore;
        state.deferred.action = action;
        state.deferred.history = historyUsed;
    }
}

std::optional<Decision> SingleAgent::retryDeferred() {
    if (!state.deferred.has) return std::nullopt;
    const DeferredInvestigation deferred = state.deferred;
    const InvestigationResult result = investigateMismatch(
        deferred.beliefBefore, deferred.action, deferred.history,
        state.predictor, state.agentModel.current(), currentReserve());
    if (result.outcome == InvestigationOutcome::Deferred) {
        return makeSafeStop("budget too small for verification");
    }
    state.deferred.has = false;
    handleInvestigation(result, deferred.beliefBefore, deferred.action,
                        deferred.history.back(), false, deferred.history);
    return std::nullopt;
}

void SingleAgent::attemptTrapV2(int correctionId, Position cell) {
    CorrectionRecord* record = findCorrection(correctionId);
    if (record == nullptr || record->status != CorrectionStatus::Consistent) {
        state.waitingTrap.has = false;
        return;
    }
    const int anomaliesBefore = state.trapAnomalies;
    const TrapV2Result result = trapV2Test(
        state.belief, currentMap(), cell, state.predictor,
        state.agentModel.current(), currentReserve(), state.trapAnomalies);
    for (int i = anomaliesBefore; i < state.trapAnomalies; ++i) {
        addLog(anomalyInvalidLine());
    }
    if (result == TrapV2Result::Verified) {
        record->status = CorrectionStatus::Verified;
        addLog(correctionVerifiedLine(correctionId));
        state.waitingTrap.has = false;
    } else if (result == TrapV2Result::Rejected) {
        record->status = CorrectionStatus::Rejected;
        addLog(correctionRejectedLine(correctionId));
        revertCorrection(*record);
        state.waitingTrap.has = false;
    } else {
        state.waitingTrap.has = true;
        state.waitingTrap.correctionId = correctionId;
        state.waitingTrap.cell = cell;
    }
}

void SingleAgent::continuePendingWork() {
    if (state.waitingTrap.has) {
        attemptTrapV2(state.waitingTrap.correctionId, state.waitingTrap.cell);
    }
    if (!state.pendingV1.has_value()) return;
    const Action action = state.pendingV1->action;
    const V1Progress progress = continueModelV1(
        *state.pendingV1, state.history, state.predictor, currentReserve());
    if (progress == V1Progress::Done) {
        state.pendingV1.reset();
        CorrectionRecord record;
        record.element.kind = ElementKind::ActionModel;
        record.element.action = action;
        record.status = CorrectionStatus::Consistent;
        record.stepConsistent = state.decisionStep;
        CorrectionRecord& added = addCorrectionRecord(record);
        applyCorrection(added);  // restore the action, clear the cache
        addLog(correctionModelLine(added.id, action));
    } else if (progress == V1Progress::Failed) {
        state.pendingV1.reset();
        addLog(unexplainedLine());
    }
}

}  // namespace ai_hologram
