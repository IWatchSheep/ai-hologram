// Purpose: Checks the observation footprint logic and matching update rules.
// Introduced in Stage 3.
// Architecture: FP1, FP10, FP11 and FP12 live here; FP2-FP9 live in
// test_footprint_cases.cpp so each file stays under about 200 lines.

#include "agent/footprint.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

// The Stage 1 demo layout (plan 5.5): agent (3, 3), object (5, 3),
// destination (8, 3), interior walls (4, 6) and (5, 6). Call it D.
WorldState demoWorld() {
    const ParseResult parsed = parseWorld({
        "##########", "#........#", "#........#", "#..A.O..D#", "#........#",
        "#........#", "#...##...#", "#........#", "#........#", "##########"});
    return parsed.world.value_or(WorldState{});
}

void addWall(WorldState& state, const Position cell) {
    state.walls[static_cast<std::size_t>(cell.y)]
               [static_cast<std::size_t>(cell.x)] = true;
}

void checkFP1(TestCounter& counter) {
    WorldState belief = demoWorld();
    WorldState trueState = belief;
    check(counter, isValidWorld(belief) && isValidWorld(trueState),
          "FP1: belief and true state are valid worlds");
    // The exact fixed sequence of 30 actions from the follow-up prompt.
    const std::vector<Action> actions = {
        Action::East, Action::East, Action::East, Action::East, Action::East,
        Action::North, Action::West, Action::West, Action::West, Action::South,
        Action::South, Action::East, Action::East, Action::North, Action::North,
        Action::North, Action::North, Action::West, Action::West, Action::South,
        Action::South, Action::South, Action::South, Action::South, Action::South,
        Action::East, Action::East, Action::East, Action::West, Action::North};
    int moved = 0;
    int pushed = 0;
    int blocked = 0;
    for (const Action action : actions) {
        // Check only the footprint: the cells this action can change.
        // With no fault, that update keeps the whole belief correct.
        const ActionResult observed = applyAction(trueState, action);
        const std::vector<Position> cells =
            computeFootprint(belief, action, observed.state, observed.outcome);
        applyFootprintUpdate(belief, cells, observed.state);
        check(counter, belief == observed.state,
              "FP1: belief equals the true state after the footprint update");
        if (observed.outcome == MoveOutcome::Moved) {
            ++moved;
        } else if (observed.outcome == MoveOutcome::Pushed) {
            ++pushed;
        } else if (observed.outcome == MoveOutcome::Blocked) {
            ++blocked;
        }
        trueState = observed.state;
    }
    check(counter, moved == 23 && pushed == 3 && blocked == 4,
          "FP1: sequence has 23 Moved, 3 Pushed and 4 Blocked steps");
}

void checkFP10(TestCounter& counter) {
    const WorldState trueState = demoWorld();
    WorldState belief = demoWorld();
    addWall(belief, {2, 7});
    check(counter, isValidWorld(belief) && isValidWorld(trueState),
          "FP10: belief and true state are valid worlds");
    const ActionResult predicted = applyAction(belief, Action::East);
    const ActionResult observed = applyAction(trueState, Action::East);
    const std::vector<Position> cells = computeFootprint(
        belief, Action::East, observed.state, observed.outcome);
    const FootprintComparison comparison = compareFootprint(
        predicted.state, predicted.outcome, observed.state, observed.outcome,
        cells);
    check(counter, cells == std::vector<Position>{{3, 3}, {4, 3}} &&
                       !comparison.outcomesDiffer && comparison.cells.empty(),
          "FP10: comparison outside the wrong wall has no differences");
    applyFootprintUpdate(belief, cells, observed.state);
    check(counter, belief.agent == observed.state.agent &&
                       belief.object == observed.state.object,
          "FP10: update makes the agent and object equal the observed ones");
    bool wallsMatch = true;
    for (const Position cell : cells) {
        if (isWall(belief, cell) != isWall(observed.state, cell)) {
            wallsMatch = false;
        }
    }
    check(counter, wallsMatch,
          "FP10: every footprint cell's wall equals the observed one");
    check(counter, isWall(belief, {2, 7}),
          "FP10: wall at (2, 7) outside the footprint is still believed");
}

void checkFP11(TestCounter& counter) {
    WorldState beliefA = demoWorld();
    beliefA.agent = {4, 3};
    WorldState beliefB = demoWorld();
    beliefB.agent = {4, 3};
    beliefB.object = {5, 4};
    check(counter, isValidWorld(beliefA) && isValidWorld(beliefB),
          "FP11: both beliefs are valid worlds");
    const MoveOutcome outcomeA = applyAction(beliefA, Action::East).outcome;
    const MoveOutcome outcomeB = applyAction(beliefB, Action::East).outcome;
    const std::vector<Position> cells = computeFootprintForTwoPredictions(
        beliefA, outcomeA, beliefB, outcomeB, Action::East);
    const std::vector<Position> swapped = computeFootprintForTwoPredictions(
        beliefB, outcomeB, beliefA, outcomeA, Action::East);
    check(counter, outcomeA == MoveOutcome::Pushed &&
                       outcomeB == MoveOutcome::Moved &&
                       cells == std::vector<Position>{{4, 3}, {5, 3}, {6, 3}},
          "FP11: c2 is included when either belief predicts a push");
    check(counter, swapped == cells,
          "FP11: swapping the two predictions gives the same cells");
    const WorldState beliefC = beliefB;
    const MoveOutcome outcomeC = applyAction(beliefC, Action::East).outcome;
    const std::vector<Position> movedOnly = computeFootprintForTwoPredictions(
        beliefB, outcomeB, beliefC, outcomeC, Action::East);
    check(counter, movedOnly == std::vector<Position>{{4, 3}, {5, 3}},
          "FP11: two Moved predictions include only c0 and c1");
}

void checkFP12(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "FP12: true state is a valid world");
    const std::vector<Position> outside = {{-1, 3}, {10, 3}, {3, -1}, {3, 10}};
    const WorldState predicted = state;
    const WorldState observedState = state;
    const FootprintComparison comparison = compareFootprint(
        predicted, MoveOutcome::Moved, observedState, MoveOutcome::Moved, outside);
    bool allWalls = true;
    for (const Position cell : outside) {
        // Outside cells count as walls in both states, so they never differ.
        if (!isWall(predicted, cell) || !isWall(observedState, cell)) {
            allWalls = false;
        }
    }
    check(counter, !comparison.outcomesDiffer && comparison.cells.empty() &&
                       allWalls,
          "FP12: outside-grid cells are walls in both states, with no differences");
    WorldState copy = state;
    applyFootprintUpdate(copy, outside, state);
    check(counter, copy == state,
          "FP12: footprint update with outside cells changes nothing");
    WorldState belief = demoWorld();
    belief.agent = {1, 3};
    const WorldState trueState = belief;
    check(counter, isValidWorld(belief) && isValidWorld(trueState),
          "FP12: west-facing belief and true state are valid worlds");
    const ActionResult observed = applyAction(trueState, Action::West);
    const std::vector<Position> cells = computeFootprint(
        belief, Action::West, observed.state, observed.outcome);
    check(counter, cells == std::vector<Position>{{1, 3}, {0, 3}} &&
                       isWall(observed.state, {0, 3}),
          "FP12: west footprint is (1, 3), (0, 3), and (0, 3) is a wall");
}

}  // namespace

void checkFP2(TestCounter& counter);
void checkFP3(TestCounter& counter);
void checkFP4(TestCounter& counter);
void checkFP5(TestCounter& counter);
void checkFP6(TestCounter& counter);
void checkFP7(TestCounter& counter);
void checkFP8(TestCounter& counter);
void checkFP9(TestCounter& counter);

void runFootprintTests(TestCounter& counter) {
    std::cout << "--- Footprint ---\n";
    checkFP1(counter);
    checkFP2(counter);
    checkFP3(counter);
    checkFP4(counter);
    checkFP5(counter);
    checkFP6(counter);
    checkFP7(counter);
    checkFP8(counter);
    checkFP9(counter);
    checkFP10(counter);
    checkFP11(counter);
    checkFP12(counter);
}

}  // namespace ai_hologram
