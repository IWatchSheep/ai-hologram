// Purpose: Checks FE1, FE5 and FE6: the demonstration fault-event
// list gives exactly the reference lines, generation repeats, a
// different master seed changes the list, and the two small helpers
// give their exact values (plan 3.3 and 6.3).
// Introduced in Stage 4.
// Architecture: These checks verify experimenter code only. No agent
// (C0 or C1) is run in this file. FE2, FE3 and FE4 are in
// test_fault_events_sets.cpp, split off so this file stays under
// about 200 lines with one statement per line.

#include "experiment/fault_events.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

// The demonstration experiment: layouts 500000..500003, run seeds 1
// and 2, in the order layout ascending then run seed ascending.
std::vector<std::vector<FaultEvent>> demoEvents(std::uint64_t masterSeed) {
    std::vector<RunKey> runs;
    std::vector<WorldState> layouts;
    for (std::uint64_t seed = 500000; seed <= 500003; ++seed) {
        const WorldState world = generateLayout(seed).world;
        for (int runSeed = 1; runSeed <= 2; ++runSeed) {
            runs.push_back({seed, runSeed});
            layouts.push_back(world);
        }
    }
    return generateFaultEvents(runs, layouts, masterSeed);
}

std::vector<std::string> allTexts(
    const std::vector<std::vector<FaultEvent>>& events) {
    std::vector<std::string> texts;
    for (const std::vector<FaultEvent>& run : events) {
        for (const FaultEvent& event : run) {
            texts.push_back(faultEventText(event));
        }
    }
    return texts;
}

std::vector<std::string> expectedDemoTexts() {
    return {"500000-1-1 step 4 F1 target both | (6, 8) free->blocked",
            "500000-1-2 step 6 F3 target R1 | East right",
            "500000-1-3 step 10 F4 target R2 | (8, 2)",
            "500000-2-1 step 2 F1 target R1 | (1, 6) blocked->free",
            "500000-2-2 step 9 F2 target R2 | North South East West",
            "500000-2-3 step 10 F4 target R2 | (7, 1)",
            "500001-1-1 step 4 F2 target R1 | West North East South",
            "500001-1-2 step 6 F2 target R2 | West East South North",
            "500001-1-3 step 8 F5 target R2 | -",
            "500001-2-1 step 2 F1 target R1 | (1, 3) free->blocked",
            "500001-2-2 step 4 F2 target R1 | West East South North",
            "500001-2-3 step 10 F4 target both | (7, 1)",
            "500002-1-1 step 3 F3 target R1 | East right",
            "500002-1-2 step 6 F4 target R1 | (4, 1)",
            "500002-1-3 step 7 F1 target R2 | (2, 8) blocked->free",
            "500002-2-1 step 5 F3 target both | South right",
            "500002-2-2 step 8 F3 target R2 | West left",
            "500002-2-3 step 9 F5 target both | -",
            "500003-1-1 step 1 F5 target R1 | -",
            "500003-1-2 step 2 F4 target R1 | (1, 7)",
            "500003-1-3 step 8 F1 target R2 | (6, 8) free->blocked",
            "500003-2-1 step 1 F2 target both | East North South West",
            "500003-2-2 step 9 F3 target R2 | North right",
            "500003-2-3 step 10 F5 target R1 | -"};
}

void checkDemoList(TestCounter& counter) {
    const std::vector<std::vector<FaultEvent>> events = demoEvents(4242);
    const std::vector<std::string> texts = allTexts(events);
    const std::vector<std::string> expected = expectedDemoTexts();
    bool matches = texts.size() == expected.size();
    if (matches) {
        for (std::size_t i = 0; i < texts.size(); ++i) {
            if (texts[i] != expected[i]) {
                std::cout << "FE1 first differing line " << i << ": got \""
                          << texts[i] << "\", expected \"" << expected[i]
                          << "\"\n";
                matches = false;
                break;
            }
        }
    }
    check(counter, matches,
          "FE1: demonstration list gives exactly the 24 reference lines");
    bool commonOk = true;
    bool idOk = true;
    int bothCount = 0;
    for (const std::vector<FaultEvent>& run : events) {
        for (const FaultEvent& event : run) {
            const bool isBoth = event.target == FaultTarget::Both;
            commonOk = commonOk && (event.commonMode == isBoth);
            if (isBoth) {
                ++bothCount;
            }
            const std::string id = std::to_string(event.layoutSeed) + "-" +
                                   std::to_string(event.runSeed) + "-" +
                                   std::to_string(event.index);
            idOk = idOk && (event.id == id);
            idOk = idOk && (event.index >= 1 && event.index <= 3);
        }
    }
    check(counter, commonOk && bothCount == 5,
          "FE1: commonMode is true exactly for the 5 target-both events");
    check(counter, idOk,
          "FE1: every event layoutSeed, runSeed and index match its id");
}

void checkRepeatAndSeed(TestCounter& counter) {
    const std::vector<std::string> first = allTexts(demoEvents(4242));
    const std::vector<std::string> second = allTexts(demoEvents(4242));
    check(counter, first == second && first.size() == 24,
          "FE5: generating the demonstration list twice is identical");
    const std::vector<std::vector<FaultEvent>> other = demoEvents(4243);
    bool firstOk = !other.empty() && !other[0].empty();
    if (firstOk) {
        firstOk = faultEventText(other[0][0]) ==
                  "500000-1-1 step 1 F3 target R1 | East left";
    }
    check(counter, firstOk,
          "FE5: master seed 4243 first event is the reference line");
    const std::vector<RunKey> runs{{500000, 1}};
    const std::vector<WorldState> noLayouts;
    check(counter, generateFaultEvents(runs, noLayouts, 4242).empty(),
          "FE5: different run and layout lengths give an empty list");
}

void checkHelpers(TestCounter& counter) {
    const int counts[] = {2, 3, 5, 8, 48, 1200};
    const int expected[] = {0, 1, 1, 2, 10, 240};
    bool matches = true;
    for (int i = 0; i < 6; ++i) {
        matches = matches && commonModeCount(counts[i]) == expected[i];
    }
    check(counter, matches,
          "FE6: commonModeCount gives 0, 1, 1, 2, 10, 240");
    const std::uint64_t seed = runStreamSeed(4242, 500000, 1);
    const std::uint64_t wanted = (4242ULL << 40) + (500000ULL << 8) + 1;
    check(counter, seed == wanted,
          "FE6: runStreamSeed(4242, 500000, 1) matches the formula");
}

}  // namespace

void runFaultEventTests(TestCounter& counter) {
    std::cout << "--- Fault events ---\n";
    checkDemoList(counter);
    checkRepeatAndSeed(counter);
    checkHelpers(counter);
}

}  // namespace ai_hologram
