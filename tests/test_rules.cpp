// Purpose: Checks each defined movement outcome and its state changes.
// Introduced in Stage 1.
// Architecture: These checks verify the rules shared by predictions and Environment.

#include "test_suites.h"
#include "world/rules.h"

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

void checkFreeMoves(TestCounter& counter) {
    const WorldState state = validWorld();
    const Action actions[] = {
        Action::North, Action::East, Action::South, Action::West
    };
    const Position expected[] = {
        {3, 2}, {4, 3}, {3, 4}, {2, 3}
    };

    for (int index = 0; index < 4; ++index) {
        const ActionResult result = applyAction(state, actions[index]);
        check(counter, result.outcome == MoveOutcome::Moved &&
                           result.state.agent == expected[index] &&
                           result.state.object == state.object,
              "R1: A free move advances the agent one cell in each direction");
    }
}

void checkBlockingAndPushes(TestCounter& counter) {
    WorldState state = validWorld();
    state.walls[3][4] = true;
    const ActionResult moveBlocked = applyAction(state, Action::East);
    check(counter, moveBlocked.outcome == MoveOutcome::Blocked &&
                       moveBlocked.state == state,
          "R2: Moving into a wall leaves the whole state unchanged");

    state = validWorld();
    state.agent = {3, 3};
    state.object = {4, 3};
    const WorldState original = state;
    const ActionResult pushed = applyAction(state, Action::East);
    check(counter, pushed.outcome == MoveOutcome::Pushed &&
                       pushed.state.agent == Position{4, 3} &&
                       pushed.state.object == Position{5, 3},
          "R3: A push into a free cell moves the agent and object one cell");
    check(counter, state == original,
          "R9: Applying an action does not change its input state");

    state.walls[3][5] = true;
    const ActionResult pushBlocked = applyAction(state, Action::East);
    check(counter, pushBlocked.outcome == MoveOutcome::Blocked &&
                       pushBlocked.state == state &&
                       pushBlocked.state.agent == Position{3, 3} &&
                       pushBlocked.state.object == Position{4, 3},
          "R4: A push into a wall leaves both agent and object unchanged");
}

void checkGoalAndNoPull(TestCounter& counter) {
    WorldState state = validWorld();
    state.agent = {3, 3};
    state.object = {4, 3};
    state.destination = {5, 3};
    const ActionResult goalPush = applyAction(state, Action::East);
    check(counter, goalPush.outcome == MoveOutcome::Pushed &&
                       isGoalReached(goalPush.state),
          "R5: Pushing the object onto the destination reaches the goal");

    state = validWorld();
    state.agent = {3, 3};
    state.object = {4, 3};
    state.destination = {3, 4};
    const ActionResult destinationMove = applyAction(state, Action::South);
    check(counter, destinationMove.outcome == MoveOutcome::Moved &&
                       destinationMove.state.agent == state.destination &&
                       !isGoalReached(destinationMove.state),
          "R6: The agent may step onto the destination without reaching the goal");

    const ActionResult movingAway = applyAction(state, Action::West);
    check(counter, movingAway.outcome == MoveOutcome::Moved &&
                       movingAway.state.object == state.object,
          "R7: Moving away from the object does not pull it");
}

void checkWallsAndGoalContinuation(TestCounter& counter) {
    WorldState state = validWorld();
    const auto originalWalls = state.walls;
    const ActionResult movedResult = applyAction(state, Action::North);

    state.agent = {3, 3};
    state.object = {4, 3};
    const ActionResult pushedResult = applyAction(state, Action::East);
    state.walls[3][5] = true;
    const ActionResult blockedResult = applyAction(state, Action::East);
    check(counter, movedResult.state.walls == originalWalls &&
                       pushedResult.state.walls == originalWalls &&
                       blockedResult.state.walls == state.walls,
          "R8: Actions never change walls during moves, pushes, or blocks");

    state = validWorld();
    state.agent = {3, 3};
    state.object = {4, 3};
    state.destination = state.object;
    const ActionResult afterGoal = applyAction(state, Action::East);
    check(counter, afterGoal.outcome == MoveOutcome::Pushed &&
                       afterGoal.state.object == Position{5, 3} &&
                       !isGoalReached(afterGoal.state),
          "R10: Actions continue normally after the goal has been reached");
}

void checkInvalidState(TestCounter& counter) {
    WorldState state = validWorld();
    state.agent = {10, 3};
    const ActionResult result = applyAction(state, Action::North);
    check(counter, result.outcome == MoveOutcome::InvalidState &&
                       result.state == state,
          "R11: An invalid state returns InvalidState unchanged");
}

}  // namespace

void runRulesTests(TestCounter& counter) {
    std::cout << "--- World rules ---\n";
    check(counter, outcomeName(MoveOutcome::Moved) == "Moved" &&
                       outcomeName(MoveOutcome::Pushed) == "Pushed" &&
                       outcomeName(MoveOutcome::Blocked) == "Blocked" &&
                       outcomeName(MoveOutcome::InvalidState) == "InvalidState",
          "Outcome names match their readable labels");
    checkFreeMoves(counter);
    checkBlockingAndPushes(counter);
    checkGoalAndNoPull(counter);
    checkWallsAndGoalContinuation(counter);
    checkInvalidState(counter);
}

}  // namespace ai_hologram
