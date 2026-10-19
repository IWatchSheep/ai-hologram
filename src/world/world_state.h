// Purpose: Declares the grid world and functions that inspect its state.
// Introduced in Stage 1.
// Architecture: This is the shared representation used by world components.

#pragma once

#include "world/position.h"

#include <array>

namespace ai_hologram {

constexpr int gridSize = 10;

struct WorldState {
    // Each inner array is a row, so walls[y][x] means row y, column x.
    // This row-first order matches how a grid is read from top to bottom.
    // Empty braces value-initialize the walls to false and positions to (0, 0).
    std::array<std::array<bool, gridSize>, gridSize> walls{};
    Position agent{};
    Position object{};
    Position destination{};
};

// Equality lets a test compare all positions and wall cells at once.
bool operator==(const WorldState& left, const WorldState& right);
bool operator!=(const WorldState& left, const WorldState& right);

// Returns whether both coordinates are valid grid indexes.
bool isInsideGrid(Position position);

// Treats an outside position as a wall and never indexes walls out of bounds.
bool isWall(const WorldState& state, Position position);

// Checks the single approved definition of a valid world state.
bool isValidWorld(const WorldState& state);

// Returns whether the object is on the destination.
bool isGoalReached(const WorldState& state);

}  // namespace ai_hologram
