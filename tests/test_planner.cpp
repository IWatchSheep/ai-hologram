// Purpose: Checks the A* planner: budget limits, fallback, exhaustion,
// active-test goal and the already-solved root.
// Introduced in Stage 3.
// Architecture: PL1, PL4-PL7, PL10 and the search part of DS3 live
// here; PL2, PL3, PL8 and PL9 live in test_planner_cases.cpp so each
// file stays under about 200 lines. Every state is checked valid.

#include "agent/heuristic.h"
#include "agent/planner.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <cstdlib>
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

WorldState demoWithWallWorld() {  // layout D2: D plus a wall at (4, 3)
    return parseLayout({"##########", "#........#", "#........#", "#..A#O..D#",
                        "#........#", "#........#", "#...##...#", "#........#",
                        "#........#", "##########"});
}

bool goalReached(const WorldState& state) { return state.object == state.destination; }

std::vector<Action> actionsOf(const std::vector<PlanStep>& plan) {
    std::vector<Action> actions;
    for (const PlanStep& step : plan) actions.push_back(step.action);
    return actions;
}

bool noObjectOnDeadSquare(const SearchResult& result, const DeadSquareMap& map) {
    for (const DiscoveredState& found : result.discovered) {
        if (map.dead[static_cast<std::size_t>(found.state.object.y)]
                    [static_cast<std::size_t>(found.state.object.x)]) {
            return false;
        }
    }
    return true;
}

struct Run {
    SearchResult result;
    StepPredictor predictor;
    FreeComputationCounts counts;
    DeadSquareMap map;
};

Run runMainSearch(const WorldState& root, int budget, int reserve) {
    Run run{SearchResult{}, StepPredictor{WorldModel{}, budget},
            FreeComputationCounts{}, DeadSquareMap{}};
    run.map = computeDeadSquares(root, {}, {}, run.counts);
    run.predictor.startStep();
    const std::function<int(const WorldState&)> estimate =
        [&run](const WorldState& state) {
            return heuristic(state, run.map, run.counts);
        };
    run.result = search(root, run.map, &goalReached, estimate, run.predictor, reserve);
    return run;
}

void checkPL1(TestCounter& counter) {
    const WorldState root = demoWorld();
    check(counter, isValidWorld(root), "PL1: demo layout D is a valid world");
    Run run = runMainSearch(root, 200, 1);
    const std::vector<Action> east4{Action::East, Action::East, Action::East,
                                    Action::East};
    check(counter, run.result.kind == SearchKind::Found &&
                       actionsOf(run.result.plan) == east4 &&
                       run.predictor.usedThisStep() == 16,
          "PL1: D gives Found, plan East x4, using 16 evaluations");
    check(counter, !run.result.plan.empty() &&
                       run.result.plan.back().expected.object == root.destination,
          "PL1: the plan's last expected state has the object on D");
    check(counter, noObjectOnDeadSquare(run.result, run.map),
          "PL3: no discovered state of PL1 has its object on a dead square");
}

void checkPL4AndPL5(TestCounter& counter) {
    const WorldState root = demoWithWallWorld();
    check(counter, isValidWorld(root), "PL4/PL5: layout D2 is a valid world");
    const std::vector<Action> northEast{Action::North, Action::East};
    Run first = runMainSearch(root, 10, 1);
    check(counter, first.result.kind == SearchKind::OutOfBudget &&
                       first.predictor.usedThisStep() == 9 &&
                       first.predictor.remaining() == 1,
          "PL4: D2, B=10, reserve 1 stops with 9 used, 1 remaining");
    check(counter, actionsOf(fallback(first.result)) == northEast,
          "PL5: the fallback plan is North, East (reserve 1)");
    Run second = runMainSearch(root, 10, 3);
    check(counter, second.result.kind == SearchKind::OutOfBudget &&
                       second.predictor.usedThisStep() == 7 &&
                       second.predictor.remaining() == 3,
          "PL4: D2, B=10, reserve 3 stops with 7 used, 3 remaining");
    check(counter, actionsOf(fallback(second.result)) == northEast,
          "PL5: the fallback plan is North, East (reserve 3)");
    Run tiny = runMainSearch(root, 1, 1);
    check(counter, tiny.result.kind == SearchKind::OutOfBudget &&
                       tiny.result.discovered.size() == 1 &&
                       fallback(tiny.result).empty(),
          "PL5: B=1, reserve 1 discovers only the root; fallback is empty");
}

void checkPL6AndDS3(TestCounter& counter) {
    // Layout L2: the object's region is sealed off from the agent, but
    // the object cell is not a dead square (relaxed pushes reach D).
    const WorldState root = parseLayout(
        {"##########", "#....#...#", "#.O..#.A.#", "#....#...#", "#D...#...#",
         "######...#", "#........#", "#........#", "#........#", "##########"});
    check(counter, isValidWorld(root), "PL6/DS3: layout L2 is a valid world");
    Run run = runMainSearch(root, 1000000, 1);
    check(counter, run.result.kind == SearchKind::Exhausted &&
                       run.predictor.usedThisStep() == 156 &&
                       run.result.discovered.size() == 39,
          "PL6/DS3: L2 exhausts with 156 evaluations, 39 states discovered");
    check(counter, noObjectOnDeadSquare(run.result, run.map),
          "PL3: no discovered state of PL6 has its object on a dead square");
}

void checkPL7(TestCounter& counter) {
    WorldState root = demoWithWallWorld();
    root.agent = {7, 1};  // hand-moved agent; the world stays valid
    check(counter, isValidWorld(root), "PL7: D2 with agent (7, 1) is valid");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(root, {}, {}, counts);
    const std::vector<Position> targets{{4, 4}, {3, 3}, {4, 2}};
    const auto goal = [&targets](const WorldState& state) {
        for (const Position t : targets) if (state.agent == t) return true;
        return false;
    };
    const auto walkHeuristic = [&map, &targets](const WorldState& state) {
        if (map.dead[static_cast<std::size_t>(state.object.y)]
                     [static_cast<std::size_t>(state.object.x)]) {
            return kInfiniteHeuristic;
        }
        int best = kInfiniteHeuristic;
        for (const Position t : targets) {
            const int d = std::abs(state.agent.x - t.x) + std::abs(state.agent.y - t.y);
            best = d < best ? d : best;
        }
        return best;
    };
    StepPredictor predictor{WorldModel{}, 1000000};
    predictor.startStep();
    const SearchResult result = search(root, map, goal, walkHeuristic, predictor, 1);
    const std::vector<Action> expected{Action::South, Action::West, Action::West,
                                       Action::West};
    check(counter, result.kind == SearchKind::Found &&
                       actionsOf(result.plan) == expected && !result.plan.empty() &&
                       result.plan.back().expected.agent == Position{4, 2} &&
                       predictor.usedThisStep() == 16,
          "PL7: active-test goal gives South, West, West, West to (4, 2), 16 used");
    check(counter, noObjectOnDeadSquare(result, map),
          "PL3: no discovered state of PL7 has its object on a dead square");
}

void checkPL10(TestCounter& counter) {
    // Layout LG: the object already sits on the destination ('*').
    const WorldState root = parseLayout(
        {"##########", "#........#", "#........#", "#..A....*#", "#........#",
         "#........#", "#...##...#", "#........#", "#........#", "##########"});
    check(counter, isValidWorld(root), "PL10: layout LG is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(root, {}, {}, counts);
    check(counter, heuristic(root, map, counts) == 4,
          "PL10: heuristic(root) is 4 even though the object is on D");
    Run run = runMainSearch(root, 200, 1);
    check(counter, run.result.kind == SearchKind::Found && run.result.plan.empty() &&
                       run.predictor.usedThisStep() == 0,
          "PL10: already-solved root is Found, empty plan, 0 evaluations");
}

}  // namespace

void checkPL2(TestCounter& counter);
void checkPL8(TestCounter& counter);
void checkPL9(TestCounter& counter);
void checkSmallLayout(TestCounter& counter);

void runPlannerTests(TestCounter& counter) {
    std::cout << "--- Planner ---\n";
    checkPL1(counter);
    checkPL2(counter);
    checkSmallLayout(counter);
    checkPL4AndPL5(counter);
    checkPL6AndDS3(counter);
    checkPL7(counter);
    checkPL8(counter);
    checkPL9(counter);
    checkPL10(counter);
}

}  // namespace ai_hologram
