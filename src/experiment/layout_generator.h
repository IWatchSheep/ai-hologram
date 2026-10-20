// Purpose: Declares the seeded layout generator that makes the
// experiment's 10x10 worlds from a layout seed (plan 3.2).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent file
// may include this header, and no agent ever calls this function.

#pragma once

#include "world/world_state.h"

#include <cstdint>

namespace ai_hologram {

// The result of one generateLayout call. When found is false, no layout
// passed the acceptance tests within the attempt limit; attempts is then
// the limit (10000) and the other fields hold their default values.
struct GeneratedLayout {
    bool found = false;
    WorldState world{};
    int attempts = 0;
    int shortest = 0;
};

// Generates one layout from its seed, exactly as in plan 3.2. The same
// seed always gives the same layout, on any machine, because every
// random choice comes from SeededRandom in a fixed order of draws.
GeneratedLayout generateLayout(std::uint64_t layoutSeed);

}  // namespace ai_hologram
