// Purpose: Represents a reasoner's belief about the world.
// Introduced in Stage 2.
// Architecture: World models predict from this state instead of true world state.

#pragma once

#include "world/world_state.h"

namespace ai_hologram {

// A separate type prevents accidentally passing "what I think" as "what is".
struct Belief {
    WorldState state;
};

}  // namespace ai_hologram
