// Purpose: Checks FD1: the Stage 4 demonstration writes exactly
// the 68 lines of plan Section 5.
// Introduced in Stage 4.
// Architecture: runFaultDemo is captured in a std::ostringstream
// and compared line by line with the expected text, so the first
// differing line is printed when the output changes.

#include "demo/fault_demo.h"
#include "test_suites.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

bool linesEqual(const std::vector<std::string>& actual,
                const std::vector<std::string>& expected) {
    if (actual == expected) {
        return true;
    }
    std::cout << "First differing line in FD1:\n";
    const std::size_t count = std::max(actual.size(), expected.size());
    for (std::size_t i = 0; i < count; ++i) {
        const std::string a = i < actual.size() ? actual[i] : "<missing>";
        const std::string e = i < expected.size() ? expected[i] : "<missing>";
        if (a != e) {
            std::cout << "  line " << i + 1 << ": got \"" << a
                      << "\", expected \"" << e << "\"\n";
            break;
        }
    }
    return false;
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

std::vector<std::string> expectedLines() {
    return {"",
            "Stage 4: the measuring instrument, on demonstration material (not an experiment)",
            "Faults are injected into a reasoner's beliefs or reasoning, never into the world.",
            "Demonstration seeds 500000-500003 only; no pilot or confirmatory layout is used.",
            "",
            "Layout 500000 (found at attempt 3, shortest solution 12 world steps):",
            "##########",
            "#...#....#",
            "#.....#A.#",
            "##.......#",
            "#.#......#",
            "#.....O..#",
            "##D...#..#",
            "#........#",
            "#......#.#",
            "##########",
            "",
            "Fault events of runs 500000-1 and 500003-1 (fault master seed 4242):",
            "500000-1-1 step 4 F1 target both | (6, 8) free->blocked",
            "500000-1-2 step 6 F3 target R1 | East right",
            "500000-1-3 step 10 F4 target R2 | (8, 2)",
            "500003-1-1 step 1 F5 target R1 | -",
            "500003-1-2 step 2 F4 target R1 | (1, 7)",
            "500003-1-3 step 8 F1 target R2 | (6, 8) free->blocked",
            "",
            "Run 500000-2, C1: Success after 12 world steps, 12 decision steps, 0 safe stops, 304 model evaluations",
            "  500000-2-1 F1: Activated at its scheduled step (step 2)",
            "  500000-2-2 F2: Activated at its scheduled step (step 9)",
            "  500000-2-3 F4: Activated at its scheduled step (step 10)",
            "",
            "Run 500000-2, C0: Success after 12 world steps, 12 decision steps, 0 safe stops, 152 model evaluations",
            "  500000-2-1 F1: Activated at its scheduled step (step 2)",
            "  500000-2-2 F2: Activated at its scheduled step (step 9)",
            "  500000-2-3 F4: Activated at its scheduled step (step 10)",
            "",
            "Run 500003-1, C1: Success after 16 world steps, 16 decision steps, 0 safe stops, 525 model evaluations",
            "  500003-1-1 F5: Activated after deferral (step 9, delay 8)",
            "  500003-1-2 F4: Activated at its scheduled step (step 2)",
            "  500003-1-3 F1: Activated at its scheduled step (step 8)",
            "",
            "Run 500003-1, C0: Success after 16 world steps, 16 decision steps, 0 safe stops, 256 model evaluations",
            "  500003-1-1 F5: Activated after deferral (step 11, delay 10)",
            "  500003-1-2 F4: Activated at its scheduled step (step 2)",
            "  500003-1-3 F1: Activated at its scheduled step (step 8)",
            "",
            "runs.csv:",
            "run_id,condition,layout_seed,run_seed,outcome,decision_steps,world_steps,safe_stops,justified_safe_stops,mismatches,mismatches_without_fault_cause,source_labels_correct,source_labels_scored,claims,false_alarms,wrong_accepted_corrections,evaluations,events_scheduled,events_activated",
            "500000-2,C1,500000,2,Success,12,12,0,0,1,0,1,1,1,0,0,304,3,3",
            "500000-2,C0,500000,2,Success,12,12,0,0,0,0,0,0,0,0,0,152,3,3",
            "500003-1,C1,500003,1,Success,16,16,0,0,1,0,1,1,1,0,0,525,3,3",
            "500003-1,C0,500003,1,Success,16,16,0,0,0,0,0,0,0,0,0,256,3,3",
            "",
            "events.csv:",
            "event_id,condition,reasoner,layout_seed,run_seed,type,target,common_mode,cell,direction,order,action,rotation,scheduled_step,reported_as,activation_step,activation_delay,not_activated_reason,opportunity,opportunity_step,revealing_opportunity,mismatch_detected,mismatch_step,strictly_detected,strict_step,detection_delay,end_state,end_step,correction_status,verified_correction",
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
            "500003-1-3,C0,R,500003,1,F1,R2,0,6:8,free->blocked,,,,8,Activated at its scheduled step,8,0,,0,,0,0,,0,,,Overwritten without correction,8,,0",
            "",
            "These lines show the instrument working; they are not results and are never compared."};
}

}  // namespace

void runFaultDemoTests(TestCounter& counter) {
    std::cout << "--- Fault demonstration ---\n";
    std::ostringstream out;
    runFaultDemo(out);
    check(counter, linesEqual(splitLines(out.str()), expectedLines()),
          "FD1: runFaultDemo writes exactly the 68 expected lines");
}

}  // namespace ai_hologram
