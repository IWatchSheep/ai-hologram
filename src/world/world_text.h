// Purpose: Declares parsing and rendering for the plain-text grid format.
// Introduced in Stage 1.
// Architecture: Text layouts are the simple input and output for world states.

#pragma once

#include "world/world_state.h"

#include <optional>
#include <string>
#include <vector>

namespace ai_hologram {

struct ParseResult {
    // std::optional holds a world on success, or no value when parsing fails.
    std::optional<WorldState> world;
    std::string error;
};

// Reads exactly ten rows of ten characters and explains invalid input.
// std::vector is a resizable list; here it holds the world's text rows.
ParseResult parseWorld(const std::vector<std::string>& lines);

// Requires a valid world; reads walls only by its own 0-to-9 loop indexes.
std::vector<std::string> renderWorld(const WorldState& state);

}  // namespace ai_hologram
