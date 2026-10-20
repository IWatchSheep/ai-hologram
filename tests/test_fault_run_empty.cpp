// Purpose: Checks FR4/IE-T2 and IE-T1: with an empty event list a
// run with faults is identical to the Stage 3 run, the hook is
// emptied afterwards, and the Stage 3 demonstrations are unchanged
// with instrumentation disabled.
// Introduced in Stage 4.
// Architecture: Split off from test_fault_run.cpp so each test file
// stays under about 200 lines. IE-T1 compares with the AD1
// demonstration text, which holds the exact Stage 3 lines.

#include "demo/agent_demo.h"
#include "experiment/fault_run.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <sstream>
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

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

void checkFR4(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(world) && isValidWorld(walled),
          "FR4: layouts D and D2 are valid");
    {
        SingleAgent faulty(Observation{1, world, std::nullopt, std::nullopt},
                           kSettings);
        const FaultRunResult with = runWithFaults(world, faulty, kSettings, {});
        SingleAgent plain(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        const RunResult without = runSingleAgent(world, plain, kSettings);
        check(counter, sameRun(with.run, without) &&
                           !faulty.state.activationHook,
              "FR4/IE-T2: empty list, C1 on D identical, hook emptied");
    }
    {
        SingleAgent faulty(Observation{1, walled, std::nullopt, std::nullopt},
                           kSettings, world);
        const FaultRunResult with =
            runWithFaults(walled, faulty, kSettings, {});
        SingleAgent plain(Observation{1, walled, std::nullopt, std::nullopt},
                          kSettings, world);
        const RunResult without = runSingleAgent(walled, plain, kSettings);
        check(counter, sameRun(with.run, without) &&
                           !faulty.state.activationHook,
              "FR4/IE-T2: empty list, C1 on D2 with belief D identical");
    }
    {
        NaiveAgent faulty(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        const FaultRunResult with = runWithFaults(world, faulty, kSettings, {});
        NaiveAgent plain(Observation{1, world, std::nullopt, std::nullopt},
                         kSettings);
        const RunResult without = runSingleAgent(world, plain, kSettings);
        check(counter, sameRun(with.run, without) &&
                           !faulty.state.activationHook,
              "FR4/IE-T2: empty list, C0 on D identical, hook emptied");
    }
    {
        NaiveAgent faulty(Observation{1, walled, std::nullopt, std::nullopt},
                          kSettings, world);
        const FaultRunResult with =
            runWithFaults(walled, faulty, kSettings, {});
        NaiveAgent plain(Observation{1, walled, std::nullopt, std::nullopt},
                         kSettings, world);
        const RunResult without = runSingleAgent(walled, plain, kSettings);
        check(counter, sameRun(with.run, without) &&
                           !faulty.state.activationHook,
              "FR4/IE-T2: empty list, C0 on D2 with belief D identical");
    }
}

void checkIET1(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    // The AD1 demonstration text holds the exact Stage 3 lines: demo 1
    // lines 5..13 and demo 2 lines 16..44 of its 46 lines.
    std::ostringstream out;
    runAgentDemos(out);
    const std::vector<std::string> demo = splitLines(out.str());
    SingleAgent first(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    const RunResult demo1 = runSingleAgent(world, first, kSettings);
    bool ok = demo.size() == 46;
    ok = ok && demo1.evaluations == 19;
    if (ok) {
        const std::vector<std::string> expected(demo.begin() + 5,
                                                demo.begin() + 14);
        ok = demo1.lines == expected;
    }
    check(counter, ok,
          "IE-T1: C1 on D disabled gives 19 evals and Stage 3 lines");
    SingleAgent second(Observation{1, walled, std::nullopt, std::nullopt},
                       kSettings, world);
    const RunResult demo2 = runSingleAgent(walled, second, kSettings);
    bool ok2 = demo2.evaluations == 414;
    if (demo.size() == 46) {
        const std::vector<std::string> expected(demo.begin() + 16,
                                                demo.begin() + 45);
        ok2 = ok2 && demo2.lines == expected;
    } else {
        ok2 = false;
    }
    check(counter, ok2,
          "IE-T1: C1 on D2 belief D disabled gives 414 evals, Stage 3 lines");
}

}  // namespace

void runFaultRunEmptyTests(TestCounter& counter) {
    checkFR4(counter);
    checkIET1(counter);
}

}  // namespace ai_hologram
