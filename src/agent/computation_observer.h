// Purpose: Declares the IE-1 watch-only observer shared by H3 and
// H4: the types that let experimenter code watch a reasoner's
// charged evaluations, dead-status reads and heuristic values.
// Introduced in Stage 4 (IE-1, approved by the owner).
// Architecture: This is the Stage 4 IE-1 watch-only observer. It is
// attached to the StepPredictor (H3) and carried by the dead-square
// map (H4). The owner's words: "the chain can be replaced by the
// camera". A chain attached to a dog could pull it; a camera cannot.
// Ours cannot pull either, and that is enforced by the types below,
// not only promised: every function receives only const references,
// const pointers or copies, and every function returns void. Agent
// code never reads any value from an observer, and nothing an agent
// decides may depend on whether an observer is attached. An empty
// function is simply not called. Trap-status READS are observed, not
// whole analysis runs (D-040): running the analysis is not a use of
// a belief in a decision; reading one cell's status to prune, reject
// or estimate is. A shadow computation (see shadow.h) is the
// experimenter repeating one of these watched computations with one
// faulted element at its true value; it never uses the reasoner's
// budget and is never visible to the reasoner.

#pragma once

#include "model/prediction.h"
#include "model/world_model.h"
#include "world/action.h"
#include "world/position.h"
#include "world/world_state.h"

#include <functional>

namespace ai_hologram {

struct ComputationObserver {
    // After each CHARGED evaluation: the state, the action, the variant model used
    // (nullptr when the reasoner's own current model was used), and the result.
    std::function<void(const WorldState&, Action, const ActionModel*, const Prediction&)> onEvaluation;
    // Each read of one cell's dead status through readDeadStatus.
    std::function<void(Position, bool)> onDeadRead;
    // Each heuristic value computed by heuristic().
    std::function<void(const WorldState&, int)> onHeuristic;
};

}  // namespace ai_hologram
