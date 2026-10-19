// Purpose: Checks complete state comparisons and their text descriptions.
// Introduced in Stage 2.
// Architecture: These tests verify comparison results using valid world fixtures.

#include "compare/difference_text.h"
#include "compare/state_difference.h"
#include "test_suites.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ai_hologram {
namespace {

WorldState validWorld() {
    WorldState state{};
    for (int index = 0; index < gridSize; ++index) {
        const std::size_t cell = static_cast<std::size_t>(index);
        state.walls[0][cell] = true;
        state.walls[static_cast<std::size_t>(gridSize - 1)][cell] = true;
        state.walls[cell][0] = true;
        state.walls[cell][static_cast<std::size_t>(gridSize - 1)] = true;
    }
    state.agent = {3, 3};
    state.object = {6, 6};
    state.destination = {7, 7};
    return state;
}

void checkValidFixtures(TestCounter& counter, const WorldState& predicted,
                        const WorldState& observed, const char* testName) {
    check(counter, isValidWorld(predicted) && isValidWorld(observed),
          std::string(testName) + ": both fixtures are valid worlds");
}
void checkIdenticalStates(TestCounter& counter) {
    WorldState state = validWorld();
    const WorldState original = state;
    checkValidFixtures(counter, state, state, "CM1");
    const StateDifference difference = compareStates(state, state);
    check(counter, !hasDifferences(difference) && !difference.agent.differs &&
                       !difference.object.differs &&
                       !difference.destination.differs &&
                       difference.walls.empty() && state == original,
          "CM1: Identical states have no differences");
}
void checkAgentDifference(TestCounter& counter) {
    const WorldState predicted = validWorld();
    WorldState observed = predicted;
    observed.agent = {4, 3};
    checkValidFixtures(counter, predicted, observed, "CM2");
    const StateDifference difference = compareStates(predicted, observed);
    check(counter, difference.agent.differs &&
                       difference.agent.predicted == Position{3, 3} &&
                       difference.agent.observed == Position{4, 3} &&
                       !difference.object.differs &&
                       !difference.destination.differs &&
                       difference.walls.empty(),
          "CM2: Only the agent difference is reported");
}
void checkObjectDifference(TestCounter& counter) {
    const WorldState predicted = validWorld();
    WorldState observed = predicted;
    observed.object = {5, 6};
    checkValidFixtures(counter, predicted, observed, "CM3");
    const StateDifference difference = compareStates(predicted, observed);
    check(counter, !difference.agent.differs && difference.object.differs &&
                       difference.object.predicted == Position{6, 6} &&
                       difference.object.observed == Position{5, 6} &&
                       !difference.destination.differs &&
                       difference.walls.empty(),
          "CM3: Only the object difference is reported");
}
void checkDestinationDifference(TestCounter& counter) {
    const WorldState predicted = validWorld();
    WorldState observed = predicted;
    observed.destination = {8, 7};
    checkValidFixtures(counter, predicted, observed, "CM4");
    const StateDifference difference = compareStates(predicted, observed);
    check(counter, !difference.agent.differs && !difference.object.differs &&
                       difference.destination.differs &&
                       difference.destination.predicted == Position{7, 7} &&
                       difference.destination.observed == Position{8, 7} &&
                       difference.walls.empty(),
          "CM4: Only the destination difference is reported");
}
void checkWallDifferences(TestCounter& counter) {
    WorldState predicted = validWorld();
    WorldState observed = predicted;
    predicted.walls[4][4] = true;
    checkValidFixtures(counter, predicted, observed, "CM5 blocked/free");
    const StateDifference blockedDifference =
        compareStates(predicted, observed);
    const bool firstDirection =
        blockedDifference.walls.size() == 1 &&
        blockedDifference.walls[0].cell == Position{4, 4} &&
        blockedDifference.walls[0].predictedBlocked &&
        !blockedDifference.walls[0].observedBlocked;

    std::swap(predicted, observed);
    checkValidFixtures(counter, predicted, observed, "CM5 free/blocked");
    const StateDifference freeDifference = compareStates(predicted, observed);
    const bool reverseDirection =
        freeDifference.walls.size() == 1 &&
        freeDifference.walls[0].cell == Position{4, 4} &&
        !freeDifference.walls[0].predictedBlocked &&
        freeDifference.walls[0].observedBlocked;
    check(counter, firstDirection && reverseDirection,
          "CM5: One wall difference has the correct cell and direction");
}
void checkMultipleDifferencesInOrder(TestCounter& counter) {
    WorldState predicted = validWorld();
    WorldState observed = predicted;
    observed.agent = {4, 3};
    // Different rows and columns make row-first order distinguishable from column-first.
    predicted.walls[4][5] = true;
    predicted.walls[5][4] = true;
    checkValidFixtures(counter, predicted, observed, "CM6");
    const StateDifference difference = compareStates(predicted, observed);
    const bool wallsAreOrdered =
        difference.walls.size() == 2 &&
        difference.walls[0].cell == Position{5, 4} &&
        difference.walls[1].cell == Position{4, 5};
    check(counter, difference.agent.differs && wallsAreOrdered &&
                       !difference.object.differs &&
                       !difference.destination.differs,
          "CM6: Agent and wall differences are all reported in order");
}
void checkDifferentOutcomes(TestCounter& counter) {
    const WorldState state = validWorld();
    checkValidFixtures(counter, state, state, "CM7");
    const Prediction prediction{Action::East, MoveOutcome::Moved, state};
    const ActionResult observed{state, MoveOutcome::Blocked};
    const PredictionComparison comparison =
        comparePrediction(prediction, observed);
    check(counter, outcomesDiffer(comparison) && !matches(comparison),
          "CM7: Different outcomes are reported and do not match");
}
void checkMatchingPrediction(TestCounter& counter) {
    const WorldState state = validWorld();
    checkValidFixtures(counter, state, state, "CM8");
    const Prediction prediction{Action::East, MoveOutcome::Moved, state};
    const ActionResult observed{state, MoveOutcome::Moved};
    const PredictionComparison comparison =
        comparePrediction(prediction, observed);
    check(counter, !outcomesDiffer(comparison) && matches(comparison),
          "CM8: Equal outcomes and states match");
}
void checkStateDifferenceWithEqualOutcomes(TestCounter& counter) {
    const WorldState predicted = validWorld();
    WorldState observed = predicted;
    observed.agent = {4, 3};
    checkValidFixtures(counter, predicted, observed, "CM9");
    const Prediction prediction{Action::East, MoveOutcome::Moved, predicted};
    const ActionResult observedResult{observed, MoveOutcome::Moved};
    const PredictionComparison comparison =
        comparePrediction(prediction, observedResult);
    check(counter, !outcomesDiffer(comparison) &&
                       hasDifferences(comparison.state) && !matches(comparison),
          "CM9: Equal outcomes with different states do not match");
}
void checkDifferenceText(TestCounter& counter) {
    WorldState predicted = validWorld();
    WorldState observed = predicted;
    predicted.object = {5, 6};
    predicted.destination = {8, 7};
    observed.agent = {4, 3};
    observed.object = {6, 6};
    observed.destination = {7, 7};
    predicted.walls[4][4] = true;
    checkValidFixtures(counter, predicted, observed, "CM10");
    const StateDifference difference = compareStates(predicted, observed);
    const std::vector<std::string> expected{
        "  - agent: predicted (3, 3), observed (4, 3)",
        "  - object: predicted (5, 6), observed (6, 6)",
        "  - destination: predicted (8, 7), observed (7, 7)",
        "  - wall at (4, 4): predicted blocked, observed free"};
    check(counter, describeDifferences(difference) == expected,
          "CM10: Difference descriptions exactly match the required lines");
}

}  // namespace

void runCompareTests(TestCounter& counter) {
    std::cout << "--- Comparison ---\n";
    checkIdenticalStates(counter);
    checkAgentDifference(counter);
    checkObjectDifference(counter);
    checkDestinationDifference(counter);
    checkWallDifferences(counter);
    checkMultipleDifferencesInOrder(counter);
    checkDifferentOutcomes(counter);
    checkMatchingPrediction(counter);
    checkStateDifferenceWithEqualOutcomes(counter);
    checkDifferenceText(counter);
}

}  // namespace ai_hologram
