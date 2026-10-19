// Purpose: Checks that Environment owns its state and counts attempted actions.
// Introduced in Stage 1.
// Architecture: These checks verify that the true world uses the shared rules.

#include "test_suites.h"
#include "world/environment.h"

#include <iostream>

namespace ai_hologram {
namespace {

WorldState validWorld() {
    WorldState state{};
    for (int index = 0; index < gridSize; ++index) {
        state.walls[0][static_cast<std::size_t>(index)] = true;
        state.walls[static_cast<std::size_t>(gridSize - 1)]
                   [static_cast<std::size_t>(index)] = true;
        state.walls[static_cast<std::size_t>(index)][0] = true;
        state.walls[static_cast<std::size_t>(index)]
                   [static_cast<std::size_t>(gridSize - 1)] = true;
    }
    state.agent = {3, 3};
    state.object = {6, 6};
    state.destination = {7, 7};
    return state;
}

void checkInitialState(TestCounter& counter) {
    const WorldState initial = validWorld();
    const Environment environment(initial);
    check(counter, environment.state() == initial &&
                       environment.stepCount() == 0,
          "E1: A new environment has its initial state and zero steps");
}

void checkAttemptCounting(TestCounter& counter) {
    WorldState initial = validWorld();
    initial.walls[2][4] = true;
    Environment environment(initial);
    const ActionResult movedResult = environment.step(Action::North);
    const bool firstAttemptCounted = environment.stepCount() == 1;
    const ActionResult blockedResult = environment.step(Action::East);
    check(counter, movedResult.outcome == MoveOutcome::Moved &&
                       firstAttemptCounted &&
                       blockedResult.outcome == MoveOutcome::Blocked &&
                       environment.stepCount() == 2,
          "E2: Each attempted action counts, including a blocked action");
}

void checkSharedRules(TestCounter& counter) {
    WorldState initial = validWorld();
    initial.agent = {3, 2};
    initial.object = {4, 2};
    initial.destination = {7, 2};
    Environment environment(initial);
    WorldState expected = initial;
    const Action actions[] = {
        Action::East, Action::East, Action::East, Action::East
    };

    for (const Action action : actions) {
        expected = applyAction(expected, action).state;
        environment.step(action);
    }
    check(counter, environment.state() == expected &&
                       environment.stepCount() == 4,
          "E3: Environment matches applying the same actions by hand");
}

void checkInvalidInitialState(TestCounter& counter) {
    WorldState initial = validWorld();
    initial.agent = {10, 3};
    Environment environment(initial);
    const ActionResult result = environment.step(Action::North);
    check(counter, result.outcome == MoveOutcome::InvalidState &&
                       result.state == initial &&
                       environment.state() == initial &&
                       environment.stepCount() == 1,
          "E4: Invalid states remain unchanged and still count attempted actions");
}

}  // namespace

void runEnvironmentTests(TestCounter& counter) {
    std::cout << "--- Environment ---\n";
    checkInitialState(counter);
    checkAttemptCounting(counter);
    checkSharedRules(counter);
    checkInvalidInitialState(counter);
}

}  // namespace ai_hologram
