// Purpose: Checks budgeted predictions from valid and invalid beliefs.
// Introduced in Stage 2.
// Architecture: These tests verify WorldModel and its action-model boundary.

#include "model/budget_counter.h"
#include "model/world_model.h"
#include "test_suites.h"

#include <cstddef>
#include <iostream>
#include <optional>

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

void checkDefaultPredictions(TestCounter& counter) {
    const WorldState state = validWorld();
    const Belief belief{state};
    const Action actions[] = {
        Action::North, Action::East, Action::South, Action::West
    };
    bool allMatch = isValidWorld(state);
    BudgetCounter budget(4);
    const WorldModel model;

    for (const Action action : actions) {
        const ActionResult expected = applyAction(state, action);
        const std::optional<Prediction> prediction =
            model.predict(belief, action, budget);
        allMatch = allMatch && prediction.has_value() &&
                   prediction->action == action &&
                   prediction->outcome == expected.outcome &&
                   prediction->state == expected.state;
    }
    check(counter, allMatch && budget.used() == 4,
          "WM1: Default predictions match the true rules for all four actions");
}

void checkOneEvaluationPerPrediction(TestCounter& counter) {
    const Belief belief{validWorld()};
    BudgetCounter budget(2);
    const WorldModel model;
    const auto prediction = model.predict(belief, Action::North, budget);
    check(counter, prediction.has_value() && budget.used() == 1 &&
                       budget.remaining() == 1,
          "WM2: Each successful prediction uses exactly one evaluation");
}

void checkExhaustedBudget(TestCounter& counter) {
    const Belief belief{validWorld()};
    const Belief original = belief;
    BudgetCounter budget(1);
    const WorldModel model;
    const auto first = model.predict(belief, Action::North, budget);
    const int usedBefore = budget.used();
    const auto second = model.predict(belief, Action::East, budget);
    check(counter, first.has_value() && !second.has_value() &&
                       budget.used() == usedBefore && belief.state == original.state,
          "WM3: An exhausted budget refuses prediction without changing state or count");
}

void checkBeliefIsUnchanged(TestCounter& counter) {
    const Belief belief{validWorld()};
    const WorldState original = belief.state;
    BudgetCounter budget(1);
    const WorldModel model;
    const auto prediction = model.predict(belief, Action::North, budget);
    check(counter, prediction.has_value() && belief.state == original,
          "WM4: Predicting never changes the belief");
}

void checkInvalidBelief(TestCounter& counter) {
    WorldState state = validWorld();
    state.agent = {10, 3};
    const Belief belief{state};
    BudgetCounter budget(1);
    const WorldModel model;
    const auto prediction = model.predict(belief, Action::North, budget);
    check(counter, !isValidWorld(belief.state) && prediction.has_value() &&
                       prediction->outcome == MoveOutcome::InvalidState &&
                       prediction->state == belief.state && budget.used() == 1,
          "WM5: An invalid belief predicts InvalidState unchanged and costs one evaluation");
}

void checkCustomActionModel(TestCounter& counter) {
    const Belief belief{validWorld()};
    const ActionModel blockedModel =
        [](const WorldState& state, Action) -> ActionResult {
            return {state, MoveOutcome::Blocked};
        };
    const WorldModel model(blockedModel);
    BudgetCounter budget(1);
    const auto prediction = model.predict(belief, Action::North, budget);
    const ActionResult trueResult = applyAction(belief.state, Action::North);
    check(counter, isValidWorld(belief.state) &&
                       trueResult.outcome == MoveOutcome::Moved &&
                       prediction.has_value() &&
                       prediction->outcome == MoveOutcome::Blocked &&
                       prediction->state == belief.state,
          "WM6: A supplied action model is used instead of the true rules");
}

void checkRecordedAction(TestCounter& counter) {
    const Belief belief{validWorld()};
    BudgetCounter budget(1);
    const WorldModel model;
    const auto prediction = model.predict(belief, Action::West, budget);
    check(counter, prediction.has_value() &&
                       prediction->action == Action::West,
          "WM7: A prediction records the requested action");
}

void checkEmptyModelFallback(TestCounter& counter) {
    const Belief belief{validWorld()};
    BudgetCounter emptyBudget(1);
    BudgetCounter defaultBudget(1);
    const WorldModel emptyModel(ActionModel{});
    const WorldModel defaultModel;
    const auto fallback =
        emptyModel.predict(belief, Action::North, emptyBudget);
    const auto expected =
        defaultModel.predict(belief, Action::North, defaultBudget);
    check(counter, fallback.has_value() && expected.has_value() &&
                       fallback->outcome == expected->outcome &&
                       fallback->state == expected->state &&
                       emptyBudget.used() == 1 && defaultBudget.used() == 1,
          "WM8: An empty action model safely falls back to the default rules");
}

}  // namespace

void runWorldModelTests(TestCounter& counter) {
    std::cout << "--- World model ---\n";
    checkDefaultPredictions(counter);
    checkOneEvaluationPerPrediction(counter);
    checkExhaustedBudget(counter);
    checkBeliefIsUnchanged(counter);
    checkInvalidBelief(counter);
    checkCustomActionModel(counter);
    checkRecordedAction(counter);
    checkEmptyModelFallback(counter);
}

}  // namespace ai_hologram
