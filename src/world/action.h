// Purpose: Declares the four possible movement directions and their helpers.
// Introduced in Stage 1.
// Architecture: World actions use these directions to describe one-cell moves.

#pragma once

#include "world/position.h"

#include <string>

namespace ai_hologram {

// enum class keeps these direction names grouped and avoids accidental mixing.
enum class Action {
    North,
    East,
    South,
    West
};

// Returns the readable name of a direction.
std::string actionName(Action action);

// Returns the one-cell change for a direction.
Position offsetFor(Action action);

// Returns a new position one cell away in the given direction.
Position moved(Position position, Action action);

}  // namespace ai_hologram
