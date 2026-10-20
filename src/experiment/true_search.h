// Purpose: Declares the experimenter's breadth-first search over true
// states, which measures the shortest solution length of a world.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent file
// may include this header, and no agent ever calls this function.

#pragma once

#include "world/world_state.h"

#include <optional>

namespace ai_hologram {

// This is the experimenter's search over TRUE states (protocol Section
// 14, Layouts: solvability is "checked by the experimenter's
// breadth-first search over true states (never visible to agents)").
// It looks at the true world, which an agent is never allowed to see, so
// it must live here with the experimenter code. It is not a reasoner, so
// it uses no budget: no reasoner runs it and no evaluation is charged.
//
// Returns 0 if the object is already on the destination. Otherwise
// returns the number of world steps in a shortest solution, or no value
// if the world is invalid or the destination cannot be reached.
std::optional<int> shortestSolutionLength(const WorldState& world);

}  // namespace ai_hologram
