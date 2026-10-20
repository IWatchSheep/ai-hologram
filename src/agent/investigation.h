// Purpose: Declares the investigation of a prediction mismatch and
// the consequence-surprise check, exactly as in plan 3.7.
// Introduced in Stage 3.
// Architecture: These are building blocks only. investigateMismatch
// works out which belief elements could explain a mismatch and
// verifies them together; it returns the result and never changes a
// belief itself. The decision step (sub-stage 3e) applies the
// footprint update and creates the correction records.

#pragma once

#include "agent/dead_squares.h"
#include "agent/observation.h"
#include "agent/records.h"
#include "agent/step_predictor.h"
#include "model/world_model.h"
#include "world/action.h"
#include "world/position.h"
#include "world/world_state.h"

#include <vector>

namespace ai_hologram {

// One belief element that could explain a mismatch, with the value
// the record (observation o_t-1) says it had. For a Wall candidate,
// `blocked` is that value and element.cell is the cell. For an
// Object candidate, `position` is o_t-1's object position; it may
// lie outside the footprint, because an investigation may read any
// part of the history (plan 3.13). `blocked` is unused for objects
// and `position` is unused for walls.
struct BeliefCandidate {
    Element element{};
    bool blocked = false;
    Position position{0, 0};
};

// How an investigation ended. BeliefCandidatesConsistent: every
// candidate passed verification together. NoCandidatePassed: a
// check failed, suspected source S5. ModelSuspected: the belief
// matched the record, so the action model is suspected (S1).
// Deferred: the re-prediction would break the reserve, so nothing
// was predicted and the whole investigation is retried next step.
enum class InvestigationOutcome {
    BeliefCandidatesConsistent,
    NoCandidatePassed,
    ModelSuspected,
    Deferred
};

struct InvestigationResult {
    InvestigationOutcome outcome = InvestigationOutcome::ModelSuspected;
    std::vector<BeliefCandidate> candidates;
    SourceLabel suspected = SourceLabel::S5;
    bool anomaly = false;  // believed agent differs from o_t-1
};

// Investigates one mismatch (plan 3.7 A). The last two entries of
// `history` are o_t-1 (before the action) and o_t (after it);
// `beliefBefore` is the belief the action was predicted from, and
// `currentModel` is the agent's current action model, used for the
// one re-prediction. That re-prediction is a variant prediction
// through `predictor`, so it costs one budgeted evaluation; if
// remaining() <= reserve the result is Deferred and nothing is
// predicted. No other prediction is made here.
InvestigationResult investigateMismatch(const WorldState& beliefBefore,
                                        Action action,
                                        const std::vector<Observation>& history,
                                        StepPredictor& predictor,
                                        const ActionModel& currentModel,
                                        int reserve);

// How a consequence surprise (an Exhausted main search) is resolved
// (plan 3.7 B). NotApplicable: the object's cell is dead or is the
// destination, or the history contradicts a trap claim.
// FullStateDeadEnd: the local check holds, so the dead-square
// classification is consistent and no claim is made. Mismatch: the
// local check fails and the history agrees, so trap(p) = dead is
// claimed with suspected source S1 (recorded by the caller).
enum class ConsequenceOutcome { NotApplicable, FullStateDeadEnd, Mismatch };

// The consequence check of plan 3.7 B, for use after an Exhausted
// main search. It makes no predictions: the local check and the
// history lookups are free computations counted in `counts`.
ConsequenceOutcome checkConsequence(const WorldState& belief,
                                    const DeadSquareMap& map,
                                    const std::vector<Observation>& history,
                                    FreeComputationCounts& counts);

}  // namespace ai_hologram
