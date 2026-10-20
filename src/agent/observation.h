// Purpose: Stores the observation delivered to an agent at each decision step.
// Introduced in Stage 3.
// Architecture: The run loop builds this from Environment and the agent keeps a history.

#pragma once

#include "world/action.h"
#include "world/rules.h"
#include "world/world_state.h"

#include <optional>

namespace ai_hologram {

struct Observation {
    int decisionStep = 0;
    WorldState state{};
    std::optional<Action> lastAction;
    std::optional<MoveOutcome> lastOutcome;
};

}  // namespace ai_hologram
