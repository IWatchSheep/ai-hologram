// Purpose: Declares the fixed scripted demonstration for the grid world.
// Introduced in Stage 1.
// Architecture: The demo uses the core world components and writes to a caller-provided stream.

#pragma once

#include <iosfwd>

namespace ai_hologram {

// A stream reference lets callers choose where output goes, including a test stream later.
void runScriptedDemo(std::ostream& out);

}  // namespace ai_hologram
