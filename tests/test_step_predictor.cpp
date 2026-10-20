// Purpose: Checks the step predictor's budget, cache and variant rules.
// Introduced in Stage 3.
// Architecture: SP1-SP6 (plan 6.4). Every state used is checked valid
// with check(). SP7 needs the decision step, so it is not part of 3c.

#include "agent/step_predictor.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>

namespace ai_hologram {
namespace {

// The Stage 1 demo layout D: agent (3, 3), object (5, 3), destination
// (8, 3), interior walls (4, 6) and (5, 6).
WorldState demoWorld() {
    const ParseResult parsed = parseWorld({
        "##########", "#........#", "#........#", "#..A.O..D#", "#........#",
        "#........#", "#...##...#", "#........#", "#........#", "##########"});
    return parsed.world.value_or(WorldState{});
}

void checkSP1(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "SP1: demo layout D is a valid world");
    StepPredictor predictor(WorldModel{}, 10);
    predictor.startStep();
    const auto first = predictor.predict(state, Action::East);
    check(counter, first.has_value() && predictor.usedThisStep() == 1 &&
                       predictor.remaining() == 9,
          "SP1: an uncached prediction costs exactly one evaluation");
    check(counter, predictor.isCached(state, Action::East),
          "SP1: the prediction is cached after the miss");
    const auto second = predictor.predict(state, Action::East);
    check(counter, second.has_value() && first.has_value() &&
                       second->state == first->state &&
                       second->outcome == first->outcome &&
                       predictor.usedThisStep() == 1,
          "SP1: repeating it in the same step is free and identical");
}

void checkSP2(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "SP2: demo layout D is a valid world");
    StepPredictor predictor(WorldModel{}, 10);
    predictor.startStep();
    predictor.predict(state, Action::East);
    predictor.startStep();
    check(counter, !predictor.isCached(state, Action::East),
          "SP2: startStep() clears the cache");
    const auto again = predictor.predict(state, Action::East);
    check(counter, again.has_value() && predictor.usedThisStep() == 1,
          "SP2: the same prediction costs again in a new step");

    // Same positions and action, but one wall differs: the cached result
    // was computed under other walls, so it must not be reused.
    WorldState changedWalls = state;
    changedWalls.walls[7][2] = true;  // add a believed wall at (2, 7)
    check(counter, isValidWorld(changedWalls),
          "SP2: belief with one more wall is a valid world");
    check(counter, !predictor.isCached(changedWalls, Action::East),
          "SP2: changed walls are not served from the cache");
    const int usedBefore = predictor.usedThisStep();
    const auto changed = predictor.predict(changedWalls, Action::East);
    check(counter, changed.has_value() &&
                       predictor.usedThisStep() == usedBefore + 1,
          "SP2: changed walls clear the cache, so predicting costs again");

    predictor.clearCache();
    check(counter, !predictor.isCached(changedWalls, Action::East),
          "SP2: clearCache() empties the cache (model/override changes)");
}

void checkSP3(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "SP3: demo layout D is a valid world");
    StepPredictor predictor(WorldModel{}, 5);
    predictor.startStep();
    predictor.predict(state, Action::East);
    predictor.startStep();
    check(counter, predictor.remaining() == 5 && predictor.usedThisStep() == 0 &&
                       predictor.usedTotal() == 1,
          "SP3: unused budget is lost; each step starts with exactly B");
}

void checkSP4(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "SP4: demo layout D is a valid world");
    StepPredictor predictor(WorldModel{}, 1);
    predictor.startStep();
    const auto first = predictor.predict(state, Action::East);
    const auto refused = predictor.predict(state, Action::South);
    check(counter, first.has_value() && !refused.has_value() &&
                       predictor.usedThisStep() == 1 &&
                       predictor.remaining() == 0 &&
                       !predictor.isCached(state, Action::South),
          "SP4: an exhausted budget returns nothing and changes nothing");
    const auto variantRefused = predictor.predictVariant(
        state, Action::South, ActionModel{&applyAction});
    check(counter, !variantRefused.has_value() &&
                       predictor.usedThisStep() == 1,
          "SP4: a variant prediction is also refused when exhausted");
}

void checkSP5(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "SP5: demo layout D is a valid world");
    // This different model always reports Blocked, so a variant result
    // can never be mistaken for the ordinary (Moved) prediction.
    const ActionModel blockedModel =
        [](const WorldState& current, Action) -> ActionResult {
        return {current, MoveOutcome::Blocked};
    };
    StepPredictor predictor(WorldModel{}, 10);
    predictor.startStep();
    const auto ordinary = predictor.predict(state, Action::East);
    const auto variant = predictor.predictVariant(state, Action::East, blockedModel);
    check(counter, ordinary.has_value() &&
                       ordinary->outcome == MoveOutcome::Moved &&
                       variant.has_value() &&
                       variant->outcome == MoveOutcome::Blocked &&
                       predictor.usedThisStep() == 2,
          "SP5: a variant ignores the cache and always costs one");
    const auto variantAgain =
        predictor.predictVariant(state, Action::East, blockedModel);
    check(counter, variantAgain.has_value() && predictor.usedThisStep() == 3,
          "SP5: a repeated variant costs one again (never cached)");
    const auto cached = predictor.predict(state, Action::East);
    check(counter, cached.has_value() &&
                       cached->outcome == MoveOutcome::Moved &&
                       predictor.usedThisStep() == 3,
          "SP5: variants never fill the cache; the ordinary entry stays");
    const auto southVariant =
        predictor.predictVariant(state, Action::South, blockedModel);
    check(counter, southVariant.has_value() && predictor.usedThisStep() == 4 &&
                       !predictor.isCached(state, Action::South),
          "SP5: a variant on a new action leaves no cache entry behind");
}

void checkSP6(TestCounter& counter) {
    const WorldState state = demoWorld();
    check(counter, isValidWorld(state), "SP6: demo layout D is a valid world");
    // An instrumented model counts its own calls. Both the ordinary
    // model and the variant model below are this same counting model,
    // so every model call anywhere must match a charged evaluation.
    int modelCalls = 0;
    const ActionModel countingModel =
        [&modelCalls](const WorldState& current, const Action action) {
            ++modelCalls;
            return applyAction(current, action);
        };
    StepPredictor predictor(WorldModel{countingModel}, 10);
    predictor.startStep();
    predictor.predict(state, Action::East);   // miss: one call
    predictor.predict(state, Action::East);   // hit: free, no call
    predictor.predictVariant(state, Action::East, countingModel);  // one call
    check(counter, modelCalls == 2 && predictor.usedThisStep() == 2 &&
                       predictor.usedTotal() == 2,
          "SP6: instrumented model calls equal charged evaluations");
}

}  // namespace

void runStepPredictorTests(TestCounter& counter) {
    std::cout << "--- Step predictor ---\n";
    checkSP1(counter);
    checkSP2(counter);
    checkSP3(counter);
    checkSP4(counter);
    checkSP5(counter);
    checkSP6(counter);
}

}  // namespace ai_hologram
