// Purpose: Checks FR2 and FR3: the demonstration experiment gives
// exactly the outcomes, steps and evaluations of plan 3.6, and its
// activation records are exactly as plan 3.6 lists, for C1 and C0.
// Introduced in Stage 4.
// Architecture: Split off from test_fault_run.cpp so each file stays
// under about 200 lines. Layouts come from generateLayout and events
// from generateFaultEvents (master seed 4242); every run uses a fresh
// agent built from its run's first observation of the true layout.

#include "experiment/fault_events.h"
#include "experiment/fault_injector.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

const AgentSettings kSettings{200, 5, 100, 3};

struct ExpectedRun {
    const char* c1Outcome;
    int c1World;
    int c1Decision;
    int c1Safe;
    int c1Evals;
    const char* c0Outcome;
    int c0World;
    int c0Evals;
};

const std::array<ExpectedRun, 8> kExpected = {{
    {"Success", 12, 12, 0, 270, "Success", 12, 152},
    {"Success", 12, 12, 0, 304, "Success", 12, 152},
    {"Step limit", 100, 100, 0, 9704, "Success", 16, 156},
    {"Success", 18, 18, 0, 469, "Success", 16, 156},
    {"Step limit", 100, 100, 0, 9406, "Step limit", 100, 9200},
    {"Step limit", 100, 100, 0, 9207, "Step limit", 100, 9200},
    {"Success", 16, 16, 0, 525, "Success", 16, 256},
    {"Step limit", 100, 100, 0, 9178, "Step limit", 100, 9000},
}};

struct DemoMaterial {
    std::vector<WorldState> layouts;
    std::vector<std::vector<FaultEvent>> events;
};

DemoMaterial demoMaterial() {
    DemoMaterial material;
    std::vector<RunKey> runs;
    for (std::uint64_t s = 500000; s <= 500003; ++s) {
        const WorldState world = generateLayout(s).world;
        for (int r = 1; r <= 2; ++r) {
            runs.push_back({s, r});
            material.layouts.push_back(world);
        }
    }
    material.events = generateFaultEvents(runs, material.layouts, 4242);
    return material;
}

std::string runName(int index) {
    return std::to_string(500000 + index / 2) + "-" +
           std::to_string(index % 2 + 1);
}

void checkFR2(TestCounter& counter, const DemoMaterial& material) {
    for (int i = 0; i < 8; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const WorldState& world = material.layouts[index];
        const ExpectedRun& wanted = kExpected[index];
        SingleAgent c1(Observation{1, world, std::nullopt, std::nullopt}, kSettings);
        const FaultRunResult r1 =
            runWithFaults(world, c1, kSettings, material.events[index]);
        bool ok1 = r1.run.outcome == wanted.c1Outcome;
        ok1 = ok1 && r1.run.worldSteps == wanted.c1World;
        ok1 = ok1 && r1.decisionSteps == wanted.c1Decision;
        ok1 = ok1 && r1.run.safeStops == wanted.c1Safe;
        ok1 = ok1 && r1.run.evaluations == wanted.c1Evals;
        if (!ok1) {
            std::cout << "FR2 C1 " << runName(i) << " got " << r1.run.outcome
                      << " world " << r1.run.worldSteps << " decision "
                      << r1.decisionSteps << " safe " << r1.run.safeStops
                      << " evals " << r1.run.evaluations << "\n";
        }
        check(counter, ok1,
              "FR2: run " + runName(i) + " C1 exact outcome and counts");
        NaiveAgent c0(Observation{1, world, std::nullopt, std::nullopt}, kSettings);
        const FaultRunResult r0 =
            runWithFaults(world, c0, kSettings, material.events[index]);
        bool ok0 = r0.run.outcome == wanted.c0Outcome;
        ok0 = ok0 && r0.run.worldSteps == wanted.c0World;
        ok0 = ok0 && r0.run.evaluations == wanted.c0Evals;
        if (!ok0) {
            std::cout << "FR2 C0 " << runName(i) << " got " << r0.run.outcome
                      << " world " << r0.run.worldSteps << " evals "
                      << r0.run.evaluations << "\n";
        }
        check(counter, ok0,
              "FR2: run " + runName(i) + " C0 exact outcome and counts");
    }
}

// The expected shape of one record: the reported text, the
// application step (0 when not applied) and the reason.
bool recordMatches(const ActivationRecord& record, bool isC1) {
    if (record.eventId == "500001-1-3") {
        if (isC1) {
            return record.status == ActivationStatus::Pending &&
                   record.reason == "the object has not moved yet" &&
                   reportedAs(record) ==
                       "Not activated: deferred, never applicable";
        }
        return record.status == ActivationStatus::Applied &&
               record.applicationStep == 15 && activationDelay(record) == 7 &&
               reportedAs(record) == "Activated after deferral";
    }
    if (record.eventId == "500002-2-3") {
        return record.status == ActivationStatus::Applied &&
               record.applicationStep == 12 && activationDelay(record) == 3 &&
               reportedAs(record) == "Activated after deferral";
    }
    if (record.eventId == "500003-1-1") {
        const int step = isC1 ? 9 : 11;
        const int delay = isC1 ? 8 : 10;
        return record.status == ActivationStatus::Applied &&
               record.applicationStep == step &&
               activationDelay(record) == delay &&
               reportedAs(record) == "Activated after deferral";
    }
    return record.status == ActivationStatus::Applied &&
           record.applicationStep == record.scheduledStep &&
           reportedAs(record) == "Activated at its scheduled step";
}

void checkFR3(TestCounter& counter, const DemoMaterial& material) {
    for (int i = 0; i < 8; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const WorldState& world = material.layouts[index];
        SingleAgent c1(Observation{1, world, std::nullopt, std::nullopt}, kSettings);
        const FaultRunResult r1 =
            runWithFaults(world, c1, kSettings, material.events[index]);
        bool ok1 = r1.activations.size() == 3;
        for (const ActivationRecord& record : r1.activations) {
            ok1 = ok1 && recordMatches(record, true);
        }
        check(counter, ok1,
              "FR3: run " + runName(i) + " C1 activation records exact");
        NaiveAgent c0(Observation{1, world, std::nullopt, std::nullopt}, kSettings);
        const FaultRunResult r0 =
            runWithFaults(world, c0, kSettings, material.events[index]);
        bool ok0 = r0.activations.size() == 3;
        for (const ActivationRecord& record : r0.activations) {
            ok0 = ok0 && recordMatches(record, false);
        }
        check(counter, ok0,
              "FR3: run " + runName(i) + " C0 activation records exact");
    }
}

}  // namespace

void runFaultRunDemoTests(TestCounter& counter) {
    const DemoMaterial material = demoMaterial();
    checkFR2(counter, material);
    checkFR3(counter, material);
}

}  // namespace ai_hologram
