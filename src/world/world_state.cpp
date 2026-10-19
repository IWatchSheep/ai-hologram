// Purpose: Implements comparisons and safe questions about a world state.
// Introduced in Stage 1.
// Architecture: These functions provide shared checks for world components.

#include "world/world_state.h"

namespace ai_hologram {

bool operator==(const WorldState& left, const WorldState& right) {
    return left.walls == right.walls &&
           left.agent == right.agent &&
           left.object == right.object &&
           left.destination == right.destination;
}

bool operator!=(const WorldState& left, const WorldState& right) {
    return !(left == right);
}

bool isInsideGrid(const Position position) {
    return position.x >= 0 && position.x < gridSize &&
           position.y >= 0 && position.y < gridSize;
}

bool isWall(const WorldState& state, const Position position) {
    if (!isInsideGrid(position)) {
        return true;
    }

    // The bounds check above makes this row-first array access safe.
    // Coordinates are int, but array indexes use std::size_t; convert only after bounds-checking.
    return state.walls[static_cast<std::size_t>(position.y)]
                      [static_cast<std::size_t>(position.x)];
}

bool isValidWorld(const WorldState& state) {
    if (!isInsideGrid(state.agent) ||
        !isInsideGrid(state.object) ||
        !isInsideGrid(state.destination)) {
        return false;
    }

    for (int index = 0; index < gridSize; ++index) {
        if (!state.walls[0][static_cast<std::size_t>(index)] ||
            !state.walls[static_cast<std::size_t>(gridSize - 1)]
                        [static_cast<std::size_t>(index)] ||
            !state.walls[static_cast<std::size_t>(index)][0] ||
            !state.walls[static_cast<std::size_t>(index)]
                        [static_cast<std::size_t>(gridSize - 1)]) {
            return false;
        }
    }

    return !isWall(state, state.agent) &&
           !isWall(state, state.object) &&
           !isWall(state, state.destination) &&
           state.agent != state.object;
}

bool isGoalReached(const WorldState& state) {
    return state.object == state.destination;
}

}  // namespace ai_hologram
