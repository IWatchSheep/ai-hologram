// Purpose: Checks SH6 sites 4-6: the consequence check, the local
// check and the trap verification read sites (plan 6.6, D-040).
// Introduced in Stage 4.
// Architecture: Split from test_shadow_sites.cpp so each file stays
// under about 200 lines. The observer can only watch ("the chain
// can be replaced by the camera", and ours cannot pull: it receives
// only const data and returns void). What a shadow computation is,
// and why it never uses the reasoner's budget, is explained in
// shadow.h. Trap-status READS are observed, not whole analysis
// runs (D-040): each site below reports its reads through
// readDeadStatus, and every reported value equals the stored one.

#include "agent/investigation.h"
#include "agent/verification.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

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

void checkSite4(TestCounter& counter) {
    const WorldState world = layoutD();
    FreeComputationCounts counts;
    DeadSquareMap map = computeDeadSquares(world, {}, {}, counts);
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    map.observer = &observer;
    const std::vector<Observation> history{
        Observation{1, world, std::nullopt, std::nullopt}};
    const ConsequenceOutcome outcome =
        checkConsequence(world, map, history, counts);
    bool ok = outcome == ConsequenceOutcome::FullStateDeadEnd;
    ok = ok && recorder.cells.size() == 2;
    if (recorder.cells.size() == 2) {
        ok = ok && recorder.cells[0] == Position{5, 3};
        ok = ok && !recorder.values[0];
        ok = ok && recorder.cells[1] == Position{5, 2};
        ok = ok && !recorder.values[1];
    }
    ok = ok && valuesMatchMap(recorder, map);
    check(counter, ok, "SH6.4: consequence reads (5,3) then (5,2), both false");
}

void checkSite5(TestCounter& counter) {
    const WorldState world = layoutD();
    FreeComputationCounts counts;
    DeadSquareMap map = computeDeadSquares(world, {}, {}, counts);
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    map.observer = &observer;
    const bool notDead = localCheckSaysNotDead(world, map, {5, 3}, counts);
    bool ok = notDead;
    ok = ok && recorder.cells.size() == 1;
    if (recorder.cells.size() == 1) {
        ok = ok && recorder.cells[0] == Position{5, 2};
        ok = ok && !recorder.values[0];
    }
    ok = ok && valuesMatchMap(recorder, map);
    check(counter, ok, "SH6.5: local check reads exactly (5,2) false");
}

void checkSite6(TestCounter& counter) {
    const WorldState world = layoutD();
    FreeComputationCounts counts;
    DeadSquareMap map = computeDeadSquares(world, {}, {}, counts);
    Recorder recorder;
    ComputationObserver observer = observerFor(recorder);
    map.observer = &observer;
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    int anomalies = 0;
    const TrapV2Result result = trapV2Test(
        world, map, {5, 3}, predictor, ActionModel{&applyAction}, 1,
        anomalies);
    bool ok = result == TrapV2Result::Rejected;
    bool found = false;
    for (std::size_t i = 0; i < recorder.cells.size(); ++i) {
        if (recorder.cells[i] == Position{6, 3} && !recorder.values[i]) {
            found = true;
        }
    }
    ok = ok && found;
    ok = ok && valuesMatchMap(recorder, map);
    check(counter, ok, "SH6.6: trap V2 reads (6,3) false, Rejected");
}

}  // namespace

void runShadowSiteMoreTests(TestCounter& counter) {
    checkSite4(counter);
    checkSite5(counter);
    checkSite6(counter);
}

}  // namespace ai_hologram
