// Purpose: Checks FI7 and FI8: deferred F5 activation in the
// demonstration experiment (A4), runs that end before an event's
// step or where a fault is not applicable, and the record helpers.
// Introduced in Stage 4.
// Architecture: Split off from test_fault_injector_more.cpp so each
// test file stays under about 200 lines. FI7 uses demonstration
// runs with C1; FI8 uses layout D. The true world is never touched
// by the injector: only the agents' beliefs and reasoning change.

#include "experiment/fault_events.h"
#include "experiment/fault_injector.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <cstdint>
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

FaultEvent handEvent(const std::string& id, int step, FaultType type) {
    FaultEvent event;
    event.id = id;
    event.step = step;
    event.type = type;
    return event;
}

// Runs one demonstration run with C1 and returns its fault result.
FaultRunResult demoRunC1(std::uint64_t seed, int runSeed) {
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
    const std::size_t index =
        static_cast<std::size_t>((seed - 500000) * 2 + (runSeed - 1));
    const WorldState world = layouts[index];
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    return runWithFaults(world, agent, kSettings, events[index]);
}

const ActivationRecord* findRecord(const FaultRunResult& result,
                                   const std::string& id) {
    for (const ActivationRecord& record : result.activations) {
        if (record.eventId == id) {
            return &record;
        }
    }
    return nullptr;
}

void checkFI7(TestCounter& counter) {
    const FaultRunResult deferred = demoRunC1(500003, 1);
    const ActivationRecord* first = findRecord(deferred, "500003-1-1");
    bool firstOk = first != nullptr;
    if (firstOk) {
        firstOk = first->status == ActivationStatus::Applied;
        firstOk = firstOk && first->applicationStep == 9;
        firstOk = firstOk && activationDelay(*first) == 8;
        firstOk = firstOk && reportedAs(*first) == "Activated after deferral";
    }
    check(counter, firstOk,
          "FI7: 500003-1-1 applied at step 9, delay 8, after deferral (A4)");
    const FaultRunResult pending = demoRunC1(500001, 1);
    const ActivationRecord* second = findRecord(pending, "500001-1-3");
    bool secondOk = second != nullptr;
    if (secondOk) {
        secondOk = second->status == ActivationStatus::Pending;
        secondOk = secondOk && second->reason == "the object has not moved yet";
        secondOk = secondOk &&
                   reportedAs(*second) ==
                       "Not activated: deferred, never applicable";
    }
    check(counter, secondOk,
          "FI7: 500001-1-3 stays pending, object has not moved yet (A4)");
}

void checkFI8(TestCounter& counter) {
    const WorldState world = layoutD();
    {
        FaultEvent event = handEvent("t", 10, FaultType::F2);
        event.order = {Action::North, Action::East, Action::South,
                       Action::West};
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        const FaultRunResult result =
            runWithFaults(world, agent, kSettings, {event});
        bool ok = result.run.outcome == "Success";
        ok = ok && result.run.worldSteps == 4;
        ok = ok && result.activations.size() == 1;
        if (ok) {
            ok = reportedAs(result.activations[0]) ==
                 "Not activated: run ended before the scheduled step";
        }
        check(counter, ok,
              "FI8: step-10 event in a 4-step run ends before its step");
    }
    {
        FaultEvent event = handEvent("t", 1, FaultType::F1);
        event.hasCell = true;
        event.cell = {3, 3};
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        const FaultRunResult result =
            runWithFaults(world, agent, kSettings, {event});
        bool ok = result.run.outcome == "Success";
        ok = ok && result.run.worldSteps == 4;
        ok = ok && result.run.evaluations == 19;
        ok = ok && result.activations.size() == 1;
        if (ok) {
            ok = reportedAs(result.activations[0]) ==
                 "Not activated: not applicable at the scheduled step";
            ok = ok && result.activations[0].reason ==
                           "the wall would be under the believed agent";
        }
        check(counter, ok,
              "FI8: F1 not applicable at step 1, run identical (19 evals)");
    }
}

void checkRecordHelpers(TestCounter& counter) {
    ActivationRecord applied;
    applied.scheduledStep = 5;
    applied.applicationStep = 5;
    applied.status = ActivationStatus::Applied;
    bool ok = activationDelay(applied) == 0;
    ok = ok && reportedAs(applied) == "Activated at its scheduled step";
    applied.applicationStep = 8;
    ok = ok && activationDelay(applied) == 3;
    ok = ok && reportedAs(applied) == "Activated after deferral";
    ActivationRecord scheduled;
    ok = ok && activationDelay(scheduled) == 0;
    ok = ok && reportedAs(scheduled) ==
                   "Not activated: run ended before the scheduled step";
    ActivationRecord refused;
    refused.status = ActivationStatus::NotApplicable;
    ok = ok && reportedAs(refused) ==
                   "Not activated: not applicable at the scheduled step";
    ActivationRecord pending;
    pending.status = ActivationStatus::Pending;
    ok = ok && activationDelay(pending) == 0;
    ok = ok && reportedAs(pending) ==
                   "Not activated: deferred, never applicable";
    check(counter, ok,
          "FI8: reportedAs and activationDelay for every status");
}

}  // namespace

void runFaultInjectorRunTests(TestCounter& counter) {
    checkFI7(counter);
    checkFI8(counter);
    checkRecordHelpers(counter);
}

}  // namespace ai_hologram
