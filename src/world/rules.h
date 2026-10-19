// Purpose: Declares movement outcomes and the pure action-application function.
// Introduced in Stage 1.
// Architecture: Rules calculate a new world state for predictions or the environment.

#pragma once

#include "world/action.h"
#include "world/world_state.h"

#include <string>

namespace ai_hologram {

enum class MoveOutcome {
    Moved,
    Pushed,
    Blocked,
    InvalidState
};

std::string outcomeName(MoveOutcome outcome);

struct ActionResult {
    WorldState state;
    MoveOutcome outcome;
};

// Returning a new state leaves the old one untouched, so later reasoning can
// use the same rules to predict an action without changing the true world.
ActionResult applyAction(const WorldState& current, Action action);

}  // namespace ai_hologram
