// Purpose: Declares the shadow computations of plan 3.9: the
// shadow of one input, whether one observed evaluation was an
// effective use, and the shadow dead-square map.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. A
// shadow computation repeats a computation the reasoner performed,
// with one faulted element set to its true value. It is the camera,
// never the chain (computation_observer.h): these are pure
// functions, they call the model function directly, so they never
// use any reasoner's budget, and no agent ever sees a shadow or
// reads anything back from one. Shadow computations are never
// observed themselves, so they cannot recurse. Trap-status READS
// are observed, not whole analysis runs (D-040): running the
// analysis is not a use in a decision, reading one cell's status
// is, so the shadow for a read is a whole shadow MAP compared one
// cell at a time.

#pragma once

#include "agent/dead_squares.h"
#include "experiment/fault_event.h"
#include "experiment/scorer.h"
#include "model/prediction.h"
#include "model/world_model.h"
#include "world/action.h"
#include "world/world_state.h"

#include <optional>

namespace ai_hologram {

// The shadow of `state` for one event (plan 3.9, D-039): the same
// state with only that event's element at its true value, or no
// value when the shadow would change nothing (or the fault type
// has no shadow input). For F2/F5 the object is changed only when
// the state still holds the reasoner's believed object position.
std::optional<WorldState> shadowInput(const FaultEvent& event,
                                      const WorldState& state,
                                      const ScoredView& view,
                                      const WorldState& trueLayout,
                                      Position trueObject);

// True when one charged evaluation was an effective use of one
// event: the result differs from the shadow's result, by outcome
// or by footprint contents. An invalid shadow world counts as
// effective. F4 evaluations are never effective here (its shadow
// is a map, not an input); F3 compares with the standard rule.
bool evaluationEffective(const FaultEvent& event, const WorldState& state,
                         Action action, const ActionModel& modelUsed,
                         const Prediction& result, const ScoredView& view,
                         const WorldState& trueLayout, Position trueObject);

// The shadow dead-square map for F1 and F4 (plan 3.9): the analysis
// of the belief with the wall at its true value, or without one
// occurrence of the event's "not dead" override. No value for the
// other types, or when the belief already has the true wall.
std::optional<DeadSquareMap> shadowMap(const FaultEvent& event,
                                       const ScoredView& view,
                                       const WorldState& trueLayout);

}  // namespace ai_hologram
