// Purpose: Declares the two C1 single-agent demonstrations (plan 5.4).
// Introduced in Stage 3.
// Architecture: The demos run C1 through runSingleAgent in the true
// world and write the run's log and result lines to a caller-provided
// stream, so the exact output is easy to redirect or test.

#pragma once

#include <iosfwd>

namespace ai_hologram {

// Runs demo 1 (the Stage 1 demo layout with a correct belief) and
// demo 2 (the same layout plus a wall at (4, 3) in the true world,
// with a hand-written starting belief without that wall).
void runAgentDemos(std::ostream& out);

}  // namespace ai_hologram
