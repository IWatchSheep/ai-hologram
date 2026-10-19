// Purpose: Declares the environment that owns the true world state.
// Introduced in Stage 1.
// Architecture: Actions change the true state only through Environment::step.

#pragma once

#include "world/rules.h"

namespace ai_hologram {

// A class groups data with the operations that control it. Its private section
// protects the true state from being changed except through step().
class Environment {
public:
    // A constructor creates an Environment with the supplied initial world.
    // explicit prevents a WorldState from being converted into an Environment by accident.
    explicit Environment(const WorldState& initialState);

    ActionResult step(Action action);

    // const on this accessor means it cannot change the Environment; returning
    // a const reference also prevents callers from changing the world it exposes.
    const WorldState& state() const;

    // const means asking for the count does not change the Environment.
    int stepCount() const;

private:
    WorldState currentState_;
    int stepCount_ = 0;
};

}  // namespace ai_hologram
