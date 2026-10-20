// Purpose: Checks CSV1-CSV4: the CSV headers, the demonstration
// runs' and events' rows, the empty fields of an event that was not
// activated (D-044), and the file-writing helper.
// Introduced in Stage 4.
// Architecture: The rows are built by running the same demonstration
// runs as the fault demonstration, with layouts and fault events
// from generateLayout and generateFaultEvents (master seed 4242).

#include "experiment/csv_output.h"
#include "experiment/fault_events.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

const AgentSettings kSettings{200, 5, 100, 3};

struct BuiltRun {
    std::string condition;
    std::uint64_t layoutSeed = 0;
    int runSeed = 0;
    std::vector<FaultEvent> events;
    FaultRunResult result;
};

struct Material {
    std::vector<WorldState> layouts;
    std::vector<std::vector<FaultEvent>> events;
};

Material demoMaterial() {
    Material material;
    std::vector<RunKey> runs;
    for (std::uint64_t seed = 500000; seed <= 500003; ++seed) {
        const WorldState world = generateLayout(seed).world;
        for (int runSeed = 1; runSeed <= 2; ++runSeed) {
            runs.push_back({seed, runSeed});
            material.layouts.push_back(world);
        }
    }
    material.events = generateFaultEvents(runs, material.layouts, 4242);
    return material;
}

BuiltRun buildRun(const Material& material, const std::size_t index,
                  const bool isC1) {
    BuiltRun built;
    built.condition = isC1 ? "C1" : "C0";
    built.events = material.events[index];
    built.layoutSeed = built.events[0].layoutSeed;
    built.runSeed = built.events[0].runSeed;
    const WorldState& world = material.layouts[index];
    if (isC1) {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        built.result = runWithFaults(world, agent, kSettings, built.events,
                                     true);
    } else {
        NaiveAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                         kSettings);
        built.result = runWithFaults(world, agent, kSettings, built.events,
                                     true);
    }
    return built;
}

std::vector<BuiltRun> buildDemoRuns(const Material& material) {
    std::vector<BuiltRun> runs;
    runs.push_back(buildRun(material, 1, true));
    runs.push_back(buildRun(material, 1, false));
    runs.push_back(buildRun(material, 6, true));
    runs.push_back(buildRun(material, 6, false));
    return runs;
}

void checkCsv1(TestCounter& counter) {
    check(counter,
          runCsvHeader() ==
              "run_id,condition,layout_seed,run_seed,outcome,decision_steps,world_steps,safe_stops,justified_safe_stops,mismatches,mismatches_without_fault_cause,source_labels_correct,source_labels_scored,claims,false_alarms,wrong_accepted_corrections,evaluations,events_scheduled,events_activated",
          "CSV1: runCsvHeader() is exact");
    check(counter,
          eventCsvHeader() ==
              "event_id,condition,reasoner,layout_seed,run_seed,type,target,common_mode,cell,direction,order,action,rotation,scheduled_step,reported_as,activation_step,activation_delay,not_activated_reason,opportunity,opportunity_step,revealing_opportunity,mismatch_detected,mismatch_step,strictly_detected,strict_step,detection_delay,end_state,end_step,correction_status,verified_correction",
          "CSV1: eventCsvHeader() is exact");
}

void checkCsv2(TestCounter& counter, const std::vector<BuiltRun>& runs) {
    const std::vector<std::string> wantedRuns = {
        "500000-2,C1,500000,2,Success,12,12,0,0,1,0,1,1,1,0,0,304,3,3",
        "500000-2,C0,500000,2,Success,12,12,0,0,0,0,0,0,0,0,0,152,3,3",
        "500003-1,C1,500003,1,Success,16,16,0,0,1,0,1,1,1,0,0,525,3,3",
        "500003-1,C0,500003,1,Success,16,16,0,0,0,0,0,0,0,0,0,256,3,3"};
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const std::string row = runCsvRow(runs[i].condition,
                                          runs[i].layoutSeed,
                                          runs[i].runSeed, runs[i].result);
        if (row != wantedRuns[i]) {
            std::cout << "CSV2 run row " << i << " got \"" << row << "\"\n";
        }
        check(counter, row == wantedRuns[i],
              "CSV2: runs.csv row " + std::to_string(i) + " is exact");
    }
}

void checkCsv2Events(TestCounter& counter,
                     const std::vector<BuiltRun>& runs) {
    const std::vector<std::string> wanted = {
        "500000-2-1,C1,R,500000,2,F1,R1,0,1:6,blocked->free,,,,2,Activated at its scheduled step,2,0,,0,,0,0,,0,,,Active at end of run,,,0",
        "500000-2-2,C1,R,500000,2,F2,R2,0,,,North South East West,,,9,Activated at its scheduled step,9,0,,1,9,1,1,10,1,10,1,Corrected,10,CONSISTENT,0",
        "500000-2-3,C1,R,500000,2,F4,R2,0,7:1,,,,,10,Activated at its scheduled step,10,0,,0,,0,0,,0,,,Active at end of run,,,0",
        "500000-2-1,C0,R,500000,2,F1,R1,0,1:6,blocked->free,,,,2,Activated at its scheduled step,2,0,,0,,0,0,,0,,,Overwritten without correction,2,,0",
        "500000-2-2,C0,R,500000,2,F2,R2,0,,,North South East West,,,9,Activated at its scheduled step,9,0,,0,,0,0,,0,,,Overwritten without correction,10,,0",
        "500000-2-3,C0,R,500000,2,F4,R2,0,7:1,,,,,10,Activated at its scheduled step,10,0,,0,,0,0,,0,,,Active at end of run,,,0",
        "500003-1-1,C1,R,500003,1,F5,R1,0,,,,,,1,Activated after deferral,9,8,,1,9,1,1,10,1,10,1,Corrected,10,VERIFIED,1",
        "500003-1-2,C1,R,500003,1,F4,R1,0,1:7,,,,,2,Activated at its scheduled step,2,0,,0,,0,0,,0,,,Active at end of run,,,0",
        "500003-1-3,C1,R,500003,1,F1,R2,0,6:8,free->blocked,,,,8,Activated at its scheduled step,8,0,,0,,0,0,,0,,,Active at end of run,,,0",
        "500003-1-1,C0,R,500003,1,F5,R1,0,,,,,,1,Activated after deferral,11,10,,0,,0,0,,0,,,Overwritten without correction,11,,0",
        "500003-1-2,C0,R,500003,1,F4,R1,0,1:7,,,,,2,Activated at its scheduled step,2,0,,0,,0,0,,0,,,Active at end of run,,,0",
        "500003-1-3,C0,R,500003,1,F1,R2,0,6:8,free->blocked,,,,8,Activated at its scheduled step,8,0,,0,,0,0,,0,,,Overwritten without correction,8,,0"};
    std::size_t at = 0;
    for (const BuiltRun& run : runs) {
        for (std::size_t i = 0; i < run.events.size(); ++i) {
            const std::string row =
                eventCsvRow(run.condition, run.events[i],
                            run.result.activations[i],
                            run.result.eventScores[i]);
            if (row != wanted[at]) {
                std::cout << "CSV2 event row " << at << " got \"" << row
                          << "\"\n";
            }
            check(counter, row == wanted[at],
                  "CSV2: events.csv row " + std::to_string(at) + " is exact");
            ++at;
        }
    }
}

void checkCsv3AndObserverReset(TestCounter& counter,
                               const Material& material) {
    const WorldState& world = material.layouts[2];
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    const FaultRunResult result =
        runWithFaults(world, agent, kSettings, material.events[2], true);
    const std::string row = eventCsvRow("C1", material.events[2][2],
                                        result.activations[2],
                                        result.eventScores[2]);
    const std::string wanted =
        "500001-1-3,C1,R,500001,1,F5,R2,0,,,,,,8,Not activated: deferred, never applicable,,,the object has not moved yet,,,,,,,,,,,,";
    if (row != wanted) {
        std::cout << "CSV3 got \"" << row << "\"\n";
    }
    check(counter, row == wanted, "CSV3: not-activated event row is exact");
    bool observersReset = agent.state.deadCache.map.observer == nullptr;
    observersReset = observersReset && agent.state.deadCache.observer == nullptr;
    check(counter, observersReset,
          "CSV: after runWithFaults with observe = true, the cached map "
          "and cache observers are nullptr");
}

void checkCsv4(TestCounter& counter) {
    const std::string path = "stage4_csv_test.csv";
    const std::string text = "a,b\n1,2\n";
    check(counter, writeTextFile(path, text),
          "CSV4: writeTextFile writes a file in the current folder");
    std::ifstream in(path, std::ios::binary);
    std::ostringstream contents;
    contents << in.rdbuf();
    check(counter, in && contents.str() == text,
          "CSV4: reading the file back gives exactly the same text");
    in.close();
    const int removed = std::remove(path.c_str());
    std::ifstream gone(path, std::ios::binary);
    check(counter, removed == 0 && !gone,
          "CSV4: the test file is removed and is gone");
    check(counter, !writeTextFile("no_such_folder_stage4/x.csv", "x"),
          "CSV4: a path in a folder that does not exist returns false");
}

}  // namespace

void runCsvOutputTests(TestCounter& counter) {
    std::cout << "--- CSV output ---\n";
    checkCsv1(counter);
    const Material material = demoMaterial();
    const std::vector<BuiltRun> runs = buildDemoRuns(material);
    checkCsv2(counter, runs);
    checkCsv2Events(counter, runs);
    checkCsv3AndObserverReset(counter, material);
    checkCsv4(counter);
}

}  // namespace ai_hologram
