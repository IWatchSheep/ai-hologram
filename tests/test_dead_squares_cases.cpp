// Purpose: Checks when the analysis reruns (cache), that it uses no
// model or budget, and the heuristic values.
// Introduced in Stage 3.
// Architecture: Split from test_dead_squares.cpp (DS6, DS7, DS8) so each
// test file stays under about 200 lines. Every state used is checked
// valid with check().

#include "agent/dead_squares.h"
#include "agent/heuristic.h"
#include "model/budget_counter.h"
#include "model/world_model.h"
#include "test_suites.h"
#include "world/world_text.h"

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

}  // namespace

void checkDS6(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "DS6: demo layout D is a valid world");
    FreeComputationCounts counts;
    DeadSquareCache cache;
    cache.get(belief, {}, {}, counts);
    check(counter, counts.analysisRuns == 1,
          "DS6: first use of the cache runs the analysis once");
    cache.get(belief, {}, {}, counts);
    check(counter, counts.analysisRuns == 1,
          "DS6: unchanged inputs do not rerun the analysis");

    WorldState movedAgent = belief;
    movedAgent.agent = {2, 3};
    check(counter, isValidWorld(movedAgent),
          "DS6: belief with only the agent moved is a valid world");
    cache.get(movedAgent, {}, {}, counts);
    check(counter, counts.analysisRuns == 1,
          "DS6: moving only the agent does not rerun the analysis");

    WorldState changedWall = belief;
    changedWall.walls[7][2] = true;  // add a believed wall at (2, 7)
    check(counter, isValidWorld(changedWall),
          "DS6: belief with one more wall is a valid world");
    cache.get(changedWall, {}, {}, counts);
    check(counter, counts.analysisRuns == 2,
          "DS6: changing one believed wall reruns the analysis exactly once");

    cache.get(changedWall, {{1, 1}}, {}, counts);
    check(counter, counts.analysisRuns == 3,
          "DS6: adding an override reruns the analysis exactly once");
    cache.get(changedWall, {{1, 1}}, {}, counts);
    check(counter, counts.analysisRuns == 3,
          "DS6: the same override again does not rerun the analysis");
}

void checkDS7(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "DS7: demo layout D is a valid world");
    // An instrumented model counts its own calls, and a budget counter
    // stands by: the free computations must touch neither. They cannot
    // even be passed a WorldModel or BudgetCounter (see the signatures
    // in dead_squares.h and heuristic.h); this test also shows that a
    // model that exists nearby is never called through any other route.
    int modelCalls = 0;
    const ActionModel countingModel =
        [&modelCalls](const WorldState& state, const Action action) {
            ++modelCalls;
            return applyAction(state, action);
        };
    const WorldModel model(countingModel);
    (void)model;  // built only to prove the calls below never use it
    BudgetCounter budget(10);

    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(belief, {}, {}, counts);
    const bool localCheck = localCheckSaysNotDead(belief, map, {5, 3}, counts);
    const int value = heuristic(belief, map, counts);
    check(counter, localCheck && value > 0,
          "DS7: local check and heuristic give their ordinary results");
    check(counter, budget.used() == 0 && modelCalls == 0,
          "DS7: map, local check and heuristic use no budget and no model");
}

void checkDS8(TestCounter& counter) {
    const WorldState demo = demoWorld();
    check(counter, isValidWorld(demo), "DS8: demo layout D is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(demo, {}, {}, counts);

    check(counter, heuristic(demo, map, counts) == 4,
          "DS8: agent (3, 3), object (5, 3) gives 4");
    WorldState closer = demo;
    closer.agent = {4, 3};
    check(counter, isValidWorld(closer), "DS8: agent (4, 3) state is valid");
    check(counter, heuristic(closer, map, counts) == 3,
          "DS8: agent (4, 3), object (5, 3) gives 3");

    WorldState onDeadSquare = demo;
    onDeadSquare.object = {1, 1};
    check(counter, isValidWorld(onDeadSquare),
          "DS8: object on (1, 1) state is valid");
    check(counter, heuristic(onDeadSquare, map, counts) == kInfiniteHeuristic,
          "DS8: object on dead square (1, 1) gives kInfiniteHeuristic");

    FreeComputationCounts overrideCounts;
    const DeadSquareMap overridden =
        computeDeadSquares(demo, {{1, 1}}, {}, overrideCounts);
    WorldState overriddenState = demo;
    overriddenState.agent = {5, 5};
    overriddenState.object = {1, 1};
    check(counter, isValidWorld(overriddenState),
          "DS8: overridden-cell state is valid");
    // (1, 1) has no pushDistance, so P falls back to the Manhattan
    // distance object->destination (9); walking adds 8 - 1 = 7.
    check(counter, heuristic(overriddenState, overridden, overrideCounts) == 16,
          "DS8: overridden (1, 1), agent (5, 5) gives 16 (9 + 7 fallback)");
    check(counter, counts.heuristicCalls == 3 &&
                       overrideCounts.heuristicCalls == 1,
          "DS8: heuristicCalls counts each call");
}

}  // namespace ai_hologram
