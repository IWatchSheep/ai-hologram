// Purpose: Implements direction names and movement offsets.
// Introduced in Stage 1.
// Architecture: These helpers support the world model and later movement rules.

#include "world/action.h"

namespace ai_hologram {

// const prevents changing this local copy; it does not affect the caller's value.
std::string actionName(const Action action) {
    // switch selects a result based on which enum value was provided.
    switch (action) {
        case Action::North:
            return "North";
        case Action::East:
            return "East";
        case Action::South:
            return "South";
        case Action::West:
            return "West";
    }
    return "Unknown";
}

Position offsetFor(const Action action) {
    switch (action) {
        case Action::North:
            return {0, -1};
        case Action::East:
            return {1, 0};
        case Action::South:
            return {0, 1};
        case Action::West:
            return {-1, 0};
    }
    return {0, 0};
}

Position moved(const Position position, const Action action) {
    const Position offset = offsetFor(action);
    return {position.x + offset.x, position.y + offset.y};
}

}  // namespace ai_hologram
