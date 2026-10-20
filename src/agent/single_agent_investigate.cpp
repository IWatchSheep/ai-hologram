// Purpose: Implements the part of C1's check that acts on an
// investigation's result (plan 3.7): logging the investigation
// lines, creating the belief corrections, and applying the
// investigation outcome to the belief.
// Introduced in Stage 3.
// Architecture: Split from single_agent_check.cpp for length. It is
// called from the check in single_agent_check.cpp; no successor
// state is made here, and belief changes use the footprint update
// or a correction record.

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

void SingleAgent::handleInvestigation(const InvestigationResult& result,
                                      const WorldState& beliefBefore,
                                      Action action,
                                      const Observation& after,
                                      bool logLines,
                                      const std::vector<Observation>& historyUsed) {
    if (logLines) {
        logInvestigationLines(result, beliefBefore, action);
    }
    const std::vector<Position> cells = footprintOf(beliefBefore, action, after);
    if (result.outcome == InvestigationOutcome::BeliefCandidatesConsistent) {
        createBeliefCorrections(result, beliefBefore);
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
        if (logLines) {
            addLog(investigationDeferredLine());
        }
        state.deferred.has = true;
        state.deferred.beliefBefore = beliefBefore;
        state.deferred.action = action;
        state.deferred.history = historyUsed;
    }
}

// Writes the investigation lines of plan 3.7 A.5, in order: one line per
// candidate (or the model-suspicion line), then the anomaly line.
void SingleAgent::logInvestigationLines(const InvestigationResult& result,
                                        const WorldState& beliefBefore,
                                        Action action) {
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
    if (result.anomaly) {
        addLog(anomalyAgentLine());
    }
}

// Creates one CONSISTENT correction per verified belief candidate,
// applies it and logs it, in candidate order (plan 3.7 A.3/A.5).
void SingleAgent::createBeliefCorrections(const InvestigationResult& result,
                                          const WorldState& beliefBefore) {
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
}

}  // namespace ai_hologram
