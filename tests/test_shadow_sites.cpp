// Purpose: Checks SH6: each of the six dead-status read sites is
// observed through readDeadStatus, with every reported value equal
// to the map's stored status (plan 6.6, D-040).
// Introduced in Stage 4.
// Architecture: A recording observer is attached to a hand-built
// map, or to a C1 agent's dead-square cache. The observer can only
// watch ("the chain can be replaced by the camera", and ours
// cannot pull: it receives only const data and returns void), so
// the computations below are unchanged. What a shadow computation
// is, and why it never uses the reasoner's budget, is explained in
// shadow.h. Trap-status READS are observed, not whole analysis
// runs (D-040): this test proves each of the six read sites reports.

#include "agent/heuristic.h"
#include "agent/investigation.h"
#include "agent/planner.h"
#include "agent/single_agent.h"
#include "agent/verification.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

const AgentSettings kSettings{200, 5, 100, 3};

WorldState layoutD() {
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A.O..D#",
         "#........#", "#........#", "#...##...#", "#........#",
         "#........#", "##########"});
    return *parsed.world;
}

struct Recorder {
    std::vector<Position> cells;
    std::vector<bool> values;
    std::vector<int> heuristics;
    int evaluations = 0;
};

ComputationObserver observerFor(Recorder& recorder) {
    ComputationObserver observer;
    observer.onEvaluation = [&recorder](const WorldState&, Action,
                                        const ActionModel*,
                                        const Prediction&) {
        ++recorder.evaluations;
    };
    observer.onDeadRead = [&recorder](const Position cell, const bool value) {
        recorder.cells.push_back(cell);
        recorder.values.push_back(value);
    };
    observer.onHeuristic = [&recorder](const WorldState&, const int value) {
        recorder.heuristics.push_back(value);
    };
    return observer;
}

bool valuesMatchMap(const Recorder& recorder, const DeadSquareMap& map) {
    if (recorder.cells.size() != recorder.values.size()) {
        return false;
    }
    for (std::size_t i = 0; i < recorder.cells.size(); ++i) {
        const Position cell = recorder.cells[i];
        if (!isInsideGrid(cell)) {
            return false;
        }
        const bool stored = map.dead[static_cast<std::size_t>(cell.y)]
                                     [static_cast<std::size_t>(cell.x)];
        if (stored != recorder.values[i]) {
            return false;
        }
    }
    return true;
}

DeadSquareMap trueMap(const WorldState& world, FreeComputationCounts& counts) {
    return computeDeadSquares(world, {}, {}, counts);
}

void checkSite1(TestCounter& counter) {
    const WorldState world = layoutD();
    check(counter, isValidWorld(world), "SH6.1: layout D is valid");
    FreeComputationCounts counts;
    DeadSquareMap map = trueMap(world, counts);
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    map.observer = &observer;
    const int value = heuristic(world, map, counts);
    bool ok = value == 4;
    ok = ok && recorder.cells.size() == 1;
    ok = ok && recorder.heuristics.size() == 1;
    if (recorder.cells.size() == 1) {
        ok = ok && recorder.cells[0] == Position{5, 3};
        ok = ok && !recorder.values[0];
    }
    if (recorder.heuristics.size() == 1) {
        ok = ok && recorder.heuristics[0] == 4;
    }
    ok = ok && valuesMatchMap(recorder, map);
    check(counter, ok, "SH6.1: heuristic reads (5,3) false, value 4");
}

void checkSite2(TestCounter& counter) {
    const WorldState world = layoutD();
    FreeComputationCounts counts;
    DeadSquareMap map = trueMap(world, counts);
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    map.observer = &observer;
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    const auto goal = [](const WorldState& state) {
        return state.object == state.destination;
    };
    const auto estimate = [](const WorldState&) {
        return 0;
    };
    const SearchResult result = search(world, map, goal, estimate,
                                       predictor, 1);
    bool ok = result.kind == SearchKind::Found;
    ok = ok && !recorder.cells.empty();
    ok = ok && recorder.heuristics.empty();
    ok = ok && valuesMatchMap(recorder, map);
    check(counter, ok, "SH6.2: planner pruning reads, no heuristic value");
}

void checkSite3(TestCounter& counter) {
    const WorldState world = layoutD();
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    agent.state.predictor.startStep();
    agent.state.deadCache.observer = &observer;
    const auto pick = agent.chooseCautiousAction();
    agent.state.deadCache.observer = nullptr;
    bool ok = pick.has_value();
    bool found = false;
    for (std::size_t i = 0; i < recorder.cells.size(); ++i) {
        if (recorder.cells[i] == Position{5, 3} && !recorder.values[i]) {
            found = true;
        }
    }
    ok = ok && found;
    check(counter, ok, "SH6.3: cautious choice reads (5,3) false");
}

}  // namespace

void runShadowSiteTests(TestCounter& counter) {
    std::cout << "--- Shadow read sites ---\n";
    checkSite1(counter);
    checkSite2(counter);
    checkSite3(counter);
    runShadowSiteMoreTests(counter);
}

}  // namespace ai_hologram
