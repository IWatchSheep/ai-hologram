// Purpose: Checks FR1, FR5 and FR6: injection reproduces Stage 3
// demonstration 2, a demonstration run repeated gives identical
// results, and a run whose object starts on the destination makes
// no decision and activates nothing.
// Introduced in Stage 4.
// Architecture: FR2-FR3 are in test_fault_run_demo.cpp and FR4/IE-T1
// in test_fault_run_empty.cpp, split so each file stays under about
// 200 lines. Every hand-built world is checked valid with check().

#include "experiment/fault_events.h"
#include "experiment/fault_injector.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {

void runFaultRunDemoTests(TestCounter& counter);
void runFaultRunEmptyTests(TestCounter& counter);

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
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A#O..D#",
         "#........#", "#........#", "#...##...#", "#........#",
         "#........#", "##########"});
    return *parsed.world;
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

FaultEvent handEvent(const std::string& id, int step, FaultType type) {
    FaultEvent event;
    event.id = id;
    event.step = step;
    event.type = type;
    return event;
}

void checkFR1(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(world) && isValidWorld(walled),
          "FR1: layouts D and D2 are valid");
    FaultEvent event = handEvent("demo-1", 1, FaultType::F1);
    event.direction = F1Direction::BlockedToFree;
    event.hasCell = true;
    event.cell = {4, 3};
    SingleAgent faulty(Observation{1, walled, std::nullopt, std::nullopt},
                       kSettings);
    const FaultRunResult injected =
        runWithFaults(walled, faulty, kSettings, {event});
    SingleAgent reference(Observation{1, walled, std::nullopt, std::nullopt},
                          kSettings, world);
    const RunResult expected = runSingleAgent(walled, reference, kSettings);
    bool ok = sameRun(injected.run, expected);
    ok = ok && injected.run.evaluations == 414;
    ok = ok && injected.activations.size() == 1;
    if (ok) {
        ok = injected.activations[0].status == ActivationStatus::Applied;
        ok = ok && injected.activations[0].applicationStep == 1;
        ok = ok && reportedAs(injected.activations[0]) ==
                       "Activated at its scheduled step";
    }
    check(counter, ok,
          "FR1: injected F1 gives exactly demonstration 2 (414 evals)");
}

FaultRunResult demoRunC1Once(int runIndex) {
    std::vector<RunKey> runs;
    std::vector<WorldState> layouts;
    for (std::uint64_t s = 500000; s <= 500003; ++s) {
        const WorldState world = generateLayout(s).world;
        for (int r = 1; r <= 2; ++r) {
            runs.push_back({s, r});
            layouts.push_back(world);
        }
    }
    const std::vector<std::vector<FaultEvent>> events =
        generateFaultEvents(runs, layouts, 4242);
    const std::size_t index = static_cast<std::size_t>(runIndex);
    const WorldState world = layouts[index];
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    return runWithFaults(world, agent, kSettings, events[index]);
}

bool sameActivations(const FaultRunResult& left,
                     const FaultRunResult& right) {
    if (left.activations.size() != right.activations.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.activations.size(); ++i) {
        const ActivationRecord& a = left.activations[i];
        const ActivationRecord& b = right.activations[i];
        bool same = a.eventId == b.eventId;
        same = same && a.status == b.status;
        same = same && a.scheduledStep == b.scheduledStep;
        same = same && a.applicationStep == b.applicationStep;
        same = same && a.reason == b.reason;
        if (!same) {
            return false;
        }
    }
    return true;
}

void checkFR5AndFR6(TestCounter& counter) {
    // Run 500002-2 is index 5 in layout-then-run-seed order.
    const FaultRunResult first = demoRunC1Once(5);
    const FaultRunResult second = demoRunC1Once(5);
    bool ok = sameRun(first.run, second.run);
    ok = ok && first.decisionSteps == second.decisionSteps;
    ok = ok && sameActivations(first, second);
    check(counter, ok,
          "FR5: run 500002-2 with C1 twice gives identical results");
    WorldState goal = layoutD();
    goal.object = goal.destination;
    check(counter, isValidWorld(goal),
          "FR6: layout D with object on destination is valid");
    FaultEvent event = handEvent("t", 1, FaultType::F2);
    event.order = {Action::North, Action::East, Action::South, Action::West};
    SingleAgent agent(Observation{1, goal, std::nullopt, std::nullopt},
                      kSettings);
    const FaultRunResult result =
        runWithFaults(goal, agent, kSettings, {event});
    bool ok6 = result.run.outcome == "Success";
    ok6 = ok6 && result.run.worldSteps == 0;
    ok6 = ok6 && result.decisionSteps == 0;
    ok6 = ok6 && result.activations.size() == 1;
    if (ok6) {
        ok6 = reportedAs(result.activations[0]) ==
              "Not activated: run ended before the scheduled step";
    }
    check(counter, ok6,
          "FR6: object on destination, no decision, event stays scheduled");
}

}  // namespace

void runFaultRunTests(TestCounter& counter) {
    std::cout << "--- Fault runs ---\n";
    checkFR1(counter);
    runFaultRunDemoTests(counter);
    runFaultRunEmptyTests(counter);
    checkFR5AndFR6(counter);
}

}  // namespace ai_hologram
