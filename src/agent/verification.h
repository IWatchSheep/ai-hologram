// Purpose: Declares the verification checks of plan 3.8: the V1
// history checks, the model(a) replay work, the trap checks, the
// V2 alternative predictions and the active-test target cells.
// Introduced in Stage 3.
// Architecture: These are building blocks only. They report whether
// a candidate or correction passes; they never change a belief.
//
// Which model each verification prediction uses (plan 3.8 table):
// - the investigation re-prediction uses the agent's CURRENT model;
// - the V1 replays for model(a) use the STANDARD rule (applyAction);
// - an alternative prediction for a wall or object correction uses
//   the CURRENT model on the belief with the OLD value put back;
// - an alternative for a model(a) correction uses the OLD model;
// - the trap V2 test uses the CURRENT model.
// Every one of those predictions is a variant prediction made
// through the StepPredictor, so each costs one budgeted evaluation.
//
// Why the object check must be time-aware: the object moves. Its
// position in an old observation is where it WAS, not where it is,
// so a corrected object position is compared only with o_t-1, the
// observation of the same moment it refers to. Walls never move or
// change, so a wall value is checked against EVERY observation.

#pragma once

#include "agent/dead_squares.h"
#include "agent/observation.h"
#include "agent/records.h"
#include "agent/step_predictor.h"
#include "model/prediction.h"
#include "model/world_model.h"
#include "world/action.h"
#include "world/position.h"
#include "world/world_state.h"

#include <optional>
#include <utility>
#include <vector>

namespace ai_hologram {

// V1 for a wall: `blocked` must equal the cell's wall value in EVERY
// recorded observation, because walls never change (plan 3.8).
bool wallHistoryCheck(Position cell, bool blocked,
                      const std::vector<Observation>& history);

// V1 for the object (time-aware): `value` must equal the object's
// position in `before`, which is o_t-1, the observation just before
// the action. Earlier observations are never compared (plan 3.8).
bool objectHistoryCheck(Position value, const Observation& before);

// The unfinished V1 work for a model(a) correction (plan 3.8).
// `pendingIndices` holds the history indexes k still to replay, in
// history order; a replay predicts action `action` from
// history[k-1] with the standard rule and must reproduce history[k].
struct ModelV1Work {
    Action action = Action::North;
    std::vector<int> pendingIndices;
};

// Collects the indexes k whose recorded lastAction is `action`, in
// history order. This only reads the history; it predicts nothing.
ModelV1Work startModelV1(Action action,
                         const std::vector<Observation>& history);

// How the model V1 replays are getting on. Done: every replay
// passed. Pending: the reserve stopped the work before a replay, so
// it continues at a later step. Failed: a replay did not reproduce
// the recorded transition.
enum class V1Progress { Done, Pending, Failed };

// Replays the pending transitions with the STANDARD rule, passed to
// predictVariant (never the current model). Each replay costs one
// variant evaluation; the work stops (Pending) before a replay if
// spending one would break the reserve (plan 3.4 and 3.8).
V1Progress continueModelV1(ModelV1Work& work,
                           const std::vector<Observation>& history,
                           StepPredictor& predictor, int reserve);

// V1 for a trap claim (plan 3.8): the local check FAILS at `p` (the
// stored "not dead" status contradicts the analysis rule), and the
// history check of plan 3.7 B.3 passes (no recorded observation
// shows the object at `p` with a later one on the destination).
bool trapV1Check(const WorldState& belief, const DeadSquareMap& map,
                 Position p, const std::vector<Observation>& history,
                 FreeComputationCounts& counts);

// The result of the trap V2 test. Waiting means the reserve would
// be broken, so no prediction was made and the test is retried at
// a later step while the correction stays CONSISTENT.
enum class TrapV2Result { Verified, Rejected, Waiting };

// The model-based V2 test of the claim "p is a dead square"
// (plan 3.8). For each direction d (North, East, South, West) whose
// standing cell p-d is believed free, it builds the hypothetical
// state (belief walls and destination, agent at p-d, object at p)
// and predicts the push d with the CURRENT model, as a variant.
// A hypothetical state that is not a valid world is skipped and
// adds 1 to `anomalies`. The test is made all at once only if
// remaining() - k >= reserve for the k valid hypotheticals;
// otherwise it returns Waiting and predicts nothing. VERIFIED if
// no predicted push moves the object onto a cell that is not dead.
TrapV2Result trapV2Test(const WorldState& belief, const DeadSquareMap& map,
                        Position p, StepPredictor& predictor,
                        const ActionModel& currentModel, int reserve,
                        int& anomalies);

// The V2 alternative prediction for one correction (plan 3.8).
// Returns no prediction, and spends no evaluation, when the
// correction is not relevant to this action (wall cell c is c1 or
// c2; the object's old or currently believed position is c1 or c2;
// model(b) is relevant only when b is the action), or when the
// alternative belief is not a valid world. Wall and object
// alternatives put the element back to its OLD value and predict
// with the CURRENT model; a model alternative predicts the same
// belief with the OLD model.
std::optional<Prediction> alternativePrediction(
    const CorrectionRecord& correction, const WorldState& belief,
    Action action, StepPredictor& predictor,
    const ActionModel& currentModel, const ActionModel& oldModel);

// True when two predictions differ in outcome, agent position or
// object position: exactly the differences that make a committed
// prediction a discriminating V2 test (plan 3.8).
bool predictionsDiffer(const Prediction& first, const Prediction& second);

// The active-test target cells for a wall correction at `cell`
// (plan 3.8): for each direction d (North, East, South, West), the
// cell n = cell - d from which move d enters the cell, kept when n
// is believed free and is not the believed object cell. Each pair
// is (target cell, move into the corrected cell).
std::vector<std::pair<Position, Action>> activeTestTargets(
    const WorldState& belief, Position cell);

}  // namespace ai_hologram
