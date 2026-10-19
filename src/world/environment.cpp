// Purpose: Implements the true world's state and attempted-action counter.
// Introduced in Stage 1.
// Architecture: Environment applies shared movement rules to its private state.

#include "world/environment.h"

namespace ai_hologram {

Environment::Environment(const WorldState& initialState)
    : currentState_(initialState) {}

ActionResult Environment::step(const Action action) {
    const ActionResult result = applyAction(currentState_, action);
    currentState_ = result.state;
    ++stepCount_;
    return result;
}

const WorldState& Environment::state() const {
    return currentState_;
}

int Environment::stepCount() const {
    return stepCount_;
}

}  // namespace ai_hologram
