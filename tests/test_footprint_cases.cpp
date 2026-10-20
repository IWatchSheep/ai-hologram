// Purpose: Checks footprint rows FP2-FP9, one case per disagreement-table row.
// Introduced in Stage 3.
// Architecture: Split from test_footprint.cpp so each test file stays under
// about 200 lines. Predicted states come from the belief, observed states
// from the true state, both through applyAction.

#include "agent/footprint.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <string>
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
    state.walls[static_cast<std::size_t>(cell.y)][static_cast<std::size_t>(cell.x)] = true;
}

void checkValid(TestCounter& counter, const WorldState& belief,
                const WorldState& trueState, const std::string& label) {
    check(counter, isValidWorld(belief) && isValidWorld(trueState),
          label + ": belief and true state are valid worlds");
}

// One expected differing cell: (wall, agent here, object here) in the
// predicted state, then the same three flags in the observed state.
struct ExpectedDiff {
    Position cell;
    bool predictedWall, predictedAgent, predictedObject;
    bool observedWall, observedAgent, observedObject;
};

void checkDiffs(TestCounter& counter, const FootprintComparison& comparison,
                const std::vector<ExpectedDiff>& expected, const std::string& label) {
    bool same = comparison.cells.size() == expected.size();
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (same && index < comparison.cells.size()) {
            const FootprintCellDifference& actual = comparison.cells[index];
            const ExpectedDiff& want = expected[index];
            same = actual.cell == want.cell && actual.predictedWall == want.predictedWall &&
                   actual.observedWall == want.observedWall &&
                   actual.predictedAgentHere == want.predictedAgent &&
                   actual.observedAgentHere == want.observedAgent &&
                   actual.predictedObjectHere == want.predictedObject &&
                   actual.observedObjectHere == want.observedObject &&
                   actual.wallDiffers == (want.predictedWall != want.observedWall) &&
                   actual.agentDiffers == (want.predictedAgent != want.observedAgent) &&
                   actual.objectDiffers == (want.predictedObject != want.observedObject);
        }
    }
    check(counter, same, label);
}

struct StepCase {
    ActionResult predicted{};
    ActionResult observed{};
    std::vector<Position> cells;
    FootprintComparison comparison{};
};

// The footprint is chosen from the belief before the action, as C1 does:
// it never re-checks the whole belief, only what this action can change.
StepCase runStep(const WorldState& belief, const WorldState& trueState, const Action action) {
    StepCase result;
    result.predicted = applyAction(belief, action);
    result.observed = applyAction(trueState, action);
    result.cells = computeFootprint(belief, action, result.observed.state, result.observed.outcome);
    result.comparison = compareFootprint(result.predicted.state, result.predicted.outcome,
                                         result.observed.state, result.observed.outcome, result.cells);
    return result;
}

void checkStep(TestCounter& counter, const StepCase& result, const std::vector<Position>& cells,
               const MoveOutcome predictedOutcome, const MoveOutcome observedOutcome,
               const std::vector<ExpectedDiff>& diffs, const std::string& label) {
    check(counter, result.cells == cells, label + ": footprint cells");
    check(counter, result.predicted.outcome == predictedOutcome &&
                       result.observed.outcome == observedOutcome &&
                       result.comparison.outcomesDiffer == (predictedOutcome != observedOutcome),
          label + ": outcomes");
    checkDiffs(counter, result.comparison, diffs, label + ": differences");
}

}  // namespace

void checkFP2(TestCounter& counter) {
    const WorldState belief = demoWorld();
    const WorldState trueState = belief;
    checkValid(counter, belief, trueState, "FP2");
    const StepCase result = runStep(belief, trueState, Action::North);
    checkStep(counter, result, {{3, 3}, {3, 2}}, MoveOutcome::Moved, MoveOutcome::Moved, {}, "FP2");
}

void checkFP3(TestCounter& counter) {
    const WorldState belief = demoWorld();
    WorldState trueState = demoWorld();
    addWall(trueState, {4, 3});
    checkValid(counter, belief, trueState, "FP3");
    const StepCase result = runStep(belief, trueState, Action::East);
    checkStep(counter, result, {{3, 3}, {4, 3}}, MoveOutcome::Moved, MoveOutcome::Blocked,
              {{{3, 3}, false, false, false, false, true, false},
               {{4, 3}, false, true, false, true, false, false}}, "FP3");
}

void checkFP4(TestCounter& counter) {
    WorldState belief = demoWorld();
    addWall(belief, {4, 3});
    const WorldState trueState = demoWorld();
    checkValid(counter, belief, trueState, "FP4");
    const StepCase result = runStep(belief, trueState, Action::East);
    checkStep(counter, result, {{3, 3}, {4, 3}}, MoveOutcome::Blocked, MoveOutcome::Moved,
              {{{3, 3}, false, true, false, false, false, false},
               {{4, 3}, true, false, false, false, true, false}}, "FP4");
}

void checkFP5(TestCounter& counter) {
    WorldState belief = demoWorld();
    belief.agent = {4, 3};
    addWall(belief, {6, 3});
    WorldState trueState = demoWorld();
    trueState.agent = {4, 3};
    checkValid(counter, belief, trueState, "FP5");
    const StepCase result = runStep(belief, trueState, Action::East);
    checkStep(counter, result, {{4, 3}, {5, 3}, {6, 3}}, MoveOutcome::Blocked, MoveOutcome::Pushed,
              {{{4, 3}, false, true, false, false, false, false},
               {{5, 3}, false, false, true, false, true, false},
               {{6, 3}, true, false, false, false, false, true}}, "FP5");
}

void checkFP6(TestCounter& counter) {
    WorldState belief = demoWorld();
    belief.agent = {4, 3};
    belief.object = {5, 4};
    WorldState trueState = demoWorld();
    trueState.agent = {4, 3};
    checkValid(counter, belief, trueState, "FP6");
    const StepCase result = runStep(belief, trueState, Action::East);
    checkStep(counter, result, {{4, 3}, {5, 3}, {6, 3}}, MoveOutcome::Moved, MoveOutcome::Pushed,
              {{{6, 3}, false, false, false, false, false, true}}, "FP6");
}

void checkFP7(TestCounter& counter) {
    WorldState belief = demoWorld();
    addWall(belief, {6, 3});
    belief.agent = {4, 3};
    belief.object = {5, 4};
    WorldState trueState = demoWorld();
    addWall(trueState, {6, 3});
    trueState.agent = {4, 3};
    checkValid(counter, belief, trueState, "FP7");
    const StepCase result = runStep(belief, trueState, Action::East);
    checkStep(counter, result, {{4, 3}, {5, 3}, {6, 3}}, MoveOutcome::Moved, MoveOutcome::Blocked,
              {{{4, 3}, false, false, false, false, true, false},
               {{5, 3}, false, true, false, false, false, true}}, "FP7");
}

void checkFP8(TestCounter& counter) {
    WorldState belief = demoWorld();
    belief.agent = {4, 3};
    WorldState trueState = demoWorld();
    trueState.agent = {4, 3};
    trueState.object = {5, 4};
    checkValid(counter, belief, trueState, "FP8");
    const StepCase result = runStep(belief, trueState, Action::East);
    checkStep(counter, result, {{4, 3}, {5, 3}, {6, 3}}, MoveOutcome::Pushed, MoveOutcome::Moved,
              {{{6, 3}, false, false, true, false, false, false}}, "FP8");
}

void checkFP9(TestCounter& counter) {
    const WorldState belief = demoWorld();
    WorldState trueState = demoWorld();
    trueState.agent = {3, 4};
    checkValid(counter, belief, trueState, "FP9");
    const StepCase result = runStep(belief, trueState, Action::North);
    checkStep(counter, result, {{3, 3}, {3, 2}}, MoveOutcome::Moved, MoveOutcome::Moved,
              {{{3, 3}, false, false, false, false, true, false},
               {{3, 2}, false, true, false, false, false, false}}, "FP9");
}

}  // namespace ai_hologram
