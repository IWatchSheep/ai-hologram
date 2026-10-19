// Purpose: Stores the expected result of one action.
// Introduced in Stage 2.
// Architecture: WorldModel returns this value for later comparison with observations.

#pragma once

#include "world/action.h"
#include "world/rules.h"
#include "world/world_state.h"

namespace ai_hologram {

struct Prediction {
    Action action;
    MoveOutcome outcome;
    WorldState state;
};

}  // namespace ai_hologram
