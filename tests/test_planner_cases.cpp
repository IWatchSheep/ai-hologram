// Purpose: Checks the planner's shortest plans, determinism and that
// every examined state came from a budgeted evaluation.
// Introduced in Stage 3.
// Architecture: PL2, PL3, PL8 and PL9 (plan 6.5), split from
// test_planner.cpp so each file stays under about 200 lines. Every
// state used is checked valid with check().

#include "agent/heuristic.h"
#include "agent/planner.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

WorldState parseLayout(const std::vector<std::string>& rows) {
    return parseWorld(rows).world.value_or(WorldState{});
}

WorldState demoWorld() {  // layout D
    return parseLayout({"##########", "#........#", "#........#", "#..A.O..D#",
                        "#........#", "#........#", "#...##...#", "#........#",
                        "#........#", "##########"});
}

WorldState pl2World() {  // layout LP
    return parseLayout({"##########", "#....D##.#", "#.O......#", "#....A#..#",
                        "#.#......#", "#.....##.#", "#........#", "#........#",
                        "#........#", "##########"});
}

bool goalReached(const WorldState& state) {
    return state.object == state.destination;
}

std::vector<Action> actionsOf(const std::vector<PlanStep>& plan) {
    std::vector<Action> actions;
    for (const PlanStep& step : plan) {
        actions.push_back(step.action);
    }
    return actions;
}

bool noObjectOnDeadSquare(const SearchResult& result, const DeadSquareMap& map) {
    for (const DiscoveredState& discovered : result.discovered) {
        if (map.dead[static_cast<std::size_t>(discovered.state.object.y)]
                    [static_cast<std::size_t>(discovered.state.object.x)]) {
            return false;
        }
    }
    return true;
}

bool samePlan(const std::vector<PlanStep>& left,
              const std::vector<PlanStep>& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].action != right[i].action ||
            left[i].expected != right[i].expected) {
            return false;
        }
    }
    return true;
}

// Runs the main-goal search, optionally with an instrumented model.
// modelCalls counts the calls of that model (0 when none is used).
SearchResult runSearch(const WorldState& root, const int budget,
                       const WorldModel& model, StepPredictor& predictor,
                       const DeadSquareMap& map,
                       FreeComputationCounts& counts) {
    (void)model;   // the model is already inside the predictor
    (void)budget;  // the budget is already inside the predictor
    predictor.startStep();
    const std::function<int(const WorldState&)> estimate =
        [&map, &counts](const WorldState& state) {
            return heuristic(state, map, counts);
        };
    return search(root, map, &goalReached, estimate, predictor, 1);
}

}  // namespace

void checkPL2(TestCounter& counter) {
    const WorldState root = pl2World();
    check(counter, isValidWorld(root), "PL2: layout LP is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(root, {}, {}, counts);
    StepPredictor predictor{WorldModel{}, 1000000};
    const SearchResult result =
        runSearch(root, 1000000, WorldModel{}, predictor, map, counts);
    // A planner that accepted the goal when first generated returned
    // 11 actions on this layout; accepting it only when it is taken
    // from the queue (and improving known paths) returns 9.
    const std::vector<Action> expected{
        Action::West, Action::West, Action::West, Action::North, Action::West,
        Action::North, Action::East, Action::East, Action::East};
    check(counter, result.kind == SearchKind::Found &&
                       actionsOf(result.plan) == expected &&
                       predictor.usedThisStep() == 60 &&
                       result.improvedPaths == 1 && result.reopenedStates == 0,
          "PL2: LP gives the 9-action plan, 60 evaluations, 1 improved "
          "path, 0 reopened states");
    check(counter, noObjectOnDeadSquare(result, map),
          "PL3: no discovered state of PL2 has its object on a dead square");
}

void checkSmallLayout(TestCounter& counter) {
    // A tiny known case: the object needs exactly two pushes east, and
    // the agent already stands behind it, so 2 is the shortest length.
    const WorldState root = parseLayout(
        {"##########", "#........#", "#........#", "#..AO.D..#", "#........#",
         "#........#", "#........#", "#........#", "#........#", "##########"});
    check(counter, isValidWorld(root),
          "PL2: small two-push layout is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(root, {}, {}, counts);
    StepPredictor predictor{WorldModel{}, 200};
    const SearchResult result =
        runSearch(root, 200, WorldModel{}, predictor, map, counts);
    check(counter, result.kind == SearchKind::Found &&
                       actionsOf(result.plan) ==
                           std::vector<Action>{Action::East, Action::East},
          "PL2: the small layout's shortest plan has length 2 (East, East)");
    check(counter, noObjectOnDeadSquare(result, map),
          "PL3: no discovered state of the small search is on a dead square");
}

void checkPL8(TestCounter& counter) {
    const WorldState demo = demoWorld();
    const WorldState pl2 = pl2World();
    check(counter, isValidWorld(demo) && isValidWorld(pl2),
          "PL8: layouts D and LP are valid worlds");
    bool identical = true;
    for (const WorldState* root : {&demo, &pl2}) {
        FreeComputationCounts firstCounts;
        FreeComputationCounts secondCounts;
        const DeadSquareMap firstMap =
            computeDeadSquares(*root, {}, {}, firstCounts);
        const DeadSquareMap secondMap =
            computeDeadSquares(*root, {}, {}, secondCounts);
        StepPredictor firstPredictor{WorldModel{}, 1000000};
        StepPredictor secondPredictor{WorldModel{}, 1000000};
        const SearchResult first = runSearch(*root, 1000000, WorldModel{},
                                             firstPredictor, firstMap, firstCounts);
        const SearchResult second = runSearch(
            *root, 1000000, WorldModel{}, secondPredictor, secondMap, secondCounts);
        identical = identical && first.kind == second.kind &&
                    samePlan(first.plan, second.plan) &&
                    firstPredictor.usedThisStep() ==
                        secondPredictor.usedThisStep();
    }
    check(counter, identical,
          "PL8: PL1 and PL2 run twice give identical plans and counts");
}

void checkPL9(TestCounter& counter) {
    const WorldState demo = demoWorld();
    const WorldState pl2 = pl2World();
    check(counter, isValidWorld(demo) && isValidWorld(pl2),
          "PL9: layouts D and LP are valid worlds");
    // An instrumented model counts every call it receives. The planner
    // may only examine states the predictor produced, so the number of
    // calls must equal the number of charged evaluations exactly.
    bool allMatch = true;
    for (const WorldState* root : {&demo, &pl2}) {
        int modelCalls = 0;
        const ActionModel countingModel =
            [&modelCalls](const WorldState& state, const Action action) {
                ++modelCalls;
                return applyAction(state, action);
            };
        const WorldModel model{countingModel};
        FreeComputationCounts counts;
        const DeadSquareMap map = computeDeadSquares(*root, {}, {}, counts);
        StepPredictor predictor{model, 1000000};
        const SearchResult result =
            runSearch(*root, 1000000, model, predictor, map, counts);
        allMatch = allMatch && result.kind == SearchKind::Found &&
                   modelCalls == predictor.usedThisStep();
    }
    check(counter, allMatch,
          "PL9: instrumented model calls equal charged evaluations (PL1, PL2)");
}

}  // namespace ai_hologram
