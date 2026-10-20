// Purpose: Checks SH2-SH5 (IE-T2, IE-T3, IE-T4): the observer
// changes nothing, observed evaluations equal the budget counter,
// primary scoring is identical with and without the observer, and
// the demonstration opportunities are exactly the plan's values.
// Introduced in Stage 4.
// Architecture: SH2 attaches a recording observer by hand to agents
// run with runSingleAgent; SH3-SH5 use runWithFaults with observe
// true or false. A shadow computation (shadow.h) repeats a watched
// computation with the faulted element true; it never uses the
// reasoner's budget and is never visible to an agent. The observer
// can only watch ("the chain can be replaced by the camera", and
// ours cannot pull), which SH2 and SH4 prove: identical results.
// Trap-status READS are observed, not whole analysis runs (D-040).

#include "agent/computation_observer.h"
#include "agent/run_loop.h"
#include "experiment/fault_events.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
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

WorldState layoutD2() {
    WorldState world = layoutD();
    world.walls[3][4] = true;
    return world;
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

bool sameRun(const RunResult& left, const RunResult& right) {
    bool same = left.outcome == right.outcome;
    same = same && left.worldSteps == right.worldSteps;
    same = same && left.safeStops == right.safeStops;
    same = same && left.mismatches == right.mismatches;
    same = same && left.corrections == right.corrections;
    same = same && left.verified == right.verified;
    same = same && left.evaluations == right.evaluations;
    same = same && left.lines == right.lines;
    return same;
}

void checkSH2One(TestCounter& counter, const std::string& name,
                 const WorldState& trueWorld,
                 const std::optional<WorldState>& belief, const bool isC1,
                 int& observed, int& total) {
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    RunResult with;
    RunResult without;
    if (isC1) {
        SingleAgent agent(Observation{1, trueWorld, std::nullopt,
                                       std::nullopt},
                          kSettings, belief);
        agent.state.predictor.setObserver(&observer);
        agent.state.deadCache.observer = &observer;
        with = runSingleAgent(trueWorld, agent, kSettings);
        observed = recorder.evaluations;
        total = agent.totalEvaluations();
        agent.state.predictor.setObserver(nullptr);
        agent.state.deadCache.observer = nullptr;
        SingleAgent plain(Observation{1, trueWorld, std::nullopt,
                                      std::nullopt},
                          kSettings, belief);
        without = runSingleAgent(trueWorld, plain, kSettings);
    } else {
        NaiveAgent agent(Observation{1, trueWorld, std::nullopt,
                                     std::nullopt},
                         kSettings, belief);
        agent.state.predictor.setObserver(&observer);
        agent.state.deadCache.observer = &observer;
        with = runSingleAgent(trueWorld, agent, kSettings);
        observed = recorder.evaluations;
        total = agent.totalEvaluations();
        agent.state.predictor.setObserver(nullptr);
        agent.state.deadCache.observer = nullptr;
        NaiveAgent plain(Observation{1, trueWorld, std::nullopt,
                                     std::nullopt},
                         kSettings, belief);
        without = runSingleAgent(trueWorld, plain, kSettings);
    }
    check(counter, sameRun(with, without),
          "SH2/IE-T2: observer changes nothing, " + name);
    check(counter, observed == total,
          "SH3/IE-T3: observed equals totalEvaluations, " + name);
}

void checkSH2(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(world) && isValidWorld(walled),
          "SH2: layouts D and D2 are valid");
    int observed = 0;
    int total = 0;
    checkSH2One(counter, "C1 on D", world, std::nullopt, true, observed,
                total);
    checkSH2One(counter, "C1 on D2 belief D", walled, world, true, observed,
                total);
    checkSH2One(counter, "C0 on D", world, std::nullopt, false, observed,
                total);
    checkSH2One(counter, "C0 on D2 belief D", walled, world, false, observed,
                total);
}

}  // namespace

void runShadowRunTests(TestCounter& counter) {
    std::cout << "--- Shadow runs ---\n";
    checkSH2(counter);
    runShadowDemoTests(counter);
}

}  // namespace ai_hologram
