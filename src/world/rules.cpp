// Purpose: Implements the grid movement and pushing rules.
// Introduced in Stage 1.
// Architecture: This pure rules component serves both predictions and Environment.

#include "world/rules.h"

namespace ai_hologram {

std::string outcomeName(const MoveOutcome outcome) {
    switch (outcome) {
        case MoveOutcome::Moved:
            return "Moved";
        case MoveOutcome::Pushed:
            return "Pushed";
        case MoveOutcome::Blocked:
            return "Blocked";
        case MoveOutcome::InvalidState:
            return "InvalidState";
    }
    return "Unknown";
}

ActionResult applyAction(const WorldState& current, const Action action) {
    if (!isValidWorld(current)) {
        return {current, MoveOutcome::InvalidState};
    }

    const Position target = moved(current.agent, action);
    if (isWall(current, target)) {
        return {current, MoveOutcome::Blocked};
    }

    WorldState next = current;
    next.agent = target;
    if (target != current.object) {
        return {next, MoveOutcome::Moved};
    }

    const Position beyond = moved(current.object, action);
    if (isWall(current, beyond)) {
        return {current, MoveOutcome::Blocked};
    }

    next.object = beyond;
    return {next, MoveOutcome::Pushed};
}

}  // namespace ai_hologram
