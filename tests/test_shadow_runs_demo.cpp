// Purpose: Checks SH3, SH4 and SH5 on the demonstration
// experiment: observed evaluations equal the run's evaluations,
// primary scoring is identical with the observer disabled, and
// every opportunity step is exactly the plan's value.
// Introduced in Stage 4.
// Architecture: Split from test_shadow_runs.cpp so each file stays
// under about 200 lines. A shadow computation repeats a watched
// computation with the faulted element at its true value; it never
// uses the reasoner's budget and is never visible to an agent. The
// observer can only watch ("the chain can be replaced by the
// camera", and ours cannot pull), which SH4 proves here. Trap-
// status READS are observed, not whole analysis runs (D-040).

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

bool optEq(const std::optional<int>& left, const int wanted) {
    if (wanted < 0) {
        return !left.has_value();
    }
    return left.has_value() && left.value() == wanted;
}

bool sameScore(const RunScore& left, const RunScore& right) {
    bool same = left.claims == right.claims;
    same = same && left.falseAlarms == right.falseAlarms;
    same = same && left.wrongAccepted == right.wrongAccepted;
    same = same && left.justifiedSafeStops == right.justifiedSafeStops;
    same = same && left.mismatches == right.mismatches;
    same = same && left.mismatchesWithoutCause == right.mismatchesWithoutCause;
    same = same && left.labelsCorrect == right.labelsCorrect;
    same = same && left.labelsScored == right.labelsScored;
    return same;
}

bool samePrimary(const FaultRunResult& left, const FaultRunResult& right) {
    bool same = left.run.outcome == right.run.outcome;
    same = same && left.run.evaluations == right.run.evaluations;
    same = same && sameScore(left.score, right.score);
    if (left.eventScores.size() != right.eventScores.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.eventScores.size(); ++i) {
        const EventScore& a = left.eventScores[i];
        const EventScore& b = right.eventScores[i];
        same = same && a.strictStep == b.strictStep;
        same = same && a.mismatchStep == b.mismatchStep;
        same = same && a.endState == b.endState;
        same = same && a.endStep == b.endStep;
        same = same && a.correctingId == b.correctingId;
        same = same && a.correctionStatus == b.correctionStatus;
    }
    return same;
}

struct Demo {
    std::vector<WorldState> layouts;
    std::vector<std::vector<FaultEvent>> events;
};

Demo demoMaterial() {
    Demo demo;
    std::vector<RunKey> runs;
    for (std::uint64_t seed = 500000; seed <= 500003; ++seed) {
        const WorldState world = generateLayout(seed).world;
        for (int runSeed = 1; runSeed <= 2; ++runSeed) {
            runs.push_back({seed, runSeed});
            demo.layouts.push_back(world);
        }
    }
    demo.events = generateFaultEvents(runs, demo.layouts, 4242);
    return demo;
}

FaultRunResult runDemo(const Demo& demo, const int index, const bool isC1,
                       const bool observe) {
    const std::size_t at = static_cast<std::size_t>(index);
    const WorldState& world = demo.layouts[at];
    if (isC1) {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        return runWithFaults(world, agent, kSettings, demo.events[at],
                             observe);
    }
    NaiveAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                     kSettings);
    return runWithFaults(world, agent, kSettings, demo.events[at], observe);
}

std::string runName(const int index) {
    return std::to_string(500000 + index / 2) + "-" +
           std::to_string(index % 2 + 1);
}

WorldState layoutD2() {
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A#O..D#",
         "#........#", "#........#", "#...##...#", "#........#",
         "#........#", "##########"});
    return *parsed.world;
}

void checkDemo(TestCounter& counter) {
    const Demo demo = demoMaterial();
    const std::array<std::array<int, 3>, 8> wantC1 = {{
        {{-1, -1, -1}}, {{-1, 9, -1}}, {{4, 6, -1}}, {{4, 4, -1}},
        {{11, -1, 7}}, {{11, 11, 12}}, {{9, -1, -1}}, {{1, 10, 10}},
    }};
    const std::array<std::array<int, 3>, 8> wantC0 = {{
        {{-1, -1, -1}}, {{-1, -1, -1}}, {{-1, -1, -1}}, {{-1, -1, -1}},
        {{11, -1, -1}}, {{11, 11, -1}}, {{-1, -1, -1}}, {{1, 10, 10}},
    }};
    for (int index = 0; index < 8; ++index) {
        const std::size_t at = static_cast<std::size_t>(index);
        const FaultRunResult c1 = runDemo(demo, index, true, true);
        const FaultRunResult c0 = runDemo(demo, index, false, true);
        check(counter,
              c1.observedEvaluations == c1.run.evaluations &&
                  c0.observedEvaluations == c0.run.evaluations,
              "SH3/IE-T3: run " + runName(index) +
                  " observed equals evaluations");
        bool ok = c1.eventScores.size() == 3 && c0.eventScores.size() == 3;
        for (std::size_t e = 0; e < 3 && ok; ++e) {
            ok = ok && optEq(c1.eventScores[e].opportunityStep, wantC1[at][e]);
            ok = ok && optEq(c0.eventScores[e].opportunityStep, wantC0[at][e]);
        }
        if (!ok) {
            std::cout << "SH5 mismatch in run " << runName(index) << "\n";
        }
        check(counter, ok,
              "SH5: run " + runName(index) + " opportunities exact");
        const FaultRunResult c1Off = runDemo(demo, index, true, false);
        const FaultRunResult c0Off = runDemo(demo, index, false, false);
        check(counter,
              samePrimary(c1, c1Off) && samePrimary(c0, c0Off) &&
                  c1Off.observedEvaluations == 0 &&
                  c0Off.observedEvaluations == 0,
              "SH4/IE-T4: run " + runName(index) +
                  " primary scoring identical without observer");
    }
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(walled), "SH5: layout D2 is valid");
    FaultEvent event;
    event.id = "demo-1";
    event.step = 1;
    event.type = FaultType::F1;
    event.direction = F1Direction::BlockedToFree;
    event.hasCell = true;
    event.cell = {4, 3};
    SingleAgent agent(Observation{1, walled, std::nullopt, std::nullopt},
                      kSettings);
    const FaultRunResult sc1 = runWithFaults(walled, agent, kSettings,
                                             {event});
    bool ok = sc1.eventScores.size() == 1;
    if (ok) {
        ok = optEq(sc1.eventScores[0].opportunityStep, 1);
    }
    check(counter, ok, "SH5: SC1 run opportunityStep is 1");
}

}  // namespace

void runShadowDemoTests(TestCounter& counter) {
    checkDemo(counter);
}

}  // namespace ai_hologram
