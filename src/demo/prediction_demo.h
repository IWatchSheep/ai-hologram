// Purpose: Declares the prediction and comparison demonstrations.
// Introduced in Stage 2.
// Architecture: The demos connect beliefs, the world model, observations, and text output.

#pragma once

#include <iosfwd>

namespace ai_hologram {

// The caller-provided stream keeps the demo output easy to redirect or test.
void runPredictionDemos(std::ostream& out);

}  // namespace ai_hologram
