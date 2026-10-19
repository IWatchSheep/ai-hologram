// Purpose: Declares readable descriptions of state differences.
// Introduced in Stage 2.
// Architecture: Text descriptions turn comparison results into stable output lines.

#pragma once

#include "compare/state_difference.h"

#include <string>
#include <vector>

namespace ai_hologram {

std::vector<std::string> describeDifferences(const StateDifference& difference);

}  // namespace ai_hologram
