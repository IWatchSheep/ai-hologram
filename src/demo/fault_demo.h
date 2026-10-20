// Purpose: Declares the Stage 4 demonstration of the measuring
// instrument on demonstration material only (plan Section 5).
// Introduced in Stage 4.
// Architecture: The demonstration runs C1 and C0 with runWithFaults
// from src/experiment/ and writes the results and CSV lines to a
// caller-provided stream. It is not an experiment: its numbers are
// never compared, and no pilot or confirmatory layout is used.

#pragma once

#include <iosfwd>

namespace ai_hologram {

// Writes the Stage 4 demonstration section (68 lines, the first one
// empty): the demonstration layout, two runs' fault events, the C1
// and C0 runs of runs 500000-2 and 500003-1, and their CSV lines.
void runFaultDemo(std::ostream& out);

}  // namespace ai_hologram
