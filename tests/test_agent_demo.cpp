// Purpose: Checks AD1: the agent demonstrations write exactly the
// 46 lines of plan 5.5 (19 and 414 evaluations included).
// Introduced in Stage 3.
// Architecture: runAgentDemos is captured in a std::ostringstream
// and compared line by line with the expected text, so the first
// differing line is printed when the output changes.

#include "demo/agent_demo.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

bool linesEqual(const std::vector<std::string>& actual,
                const std::vector<std::string>& expected,
                const std::string& label) {
    if (actual == expected) return true;
    std::cout << "First differing line in " << label << ":\n";
    const std::size_t count = std::max(actual.size(), expected.size());
    for (std::size_t i = 0; i < count; ++i) {
        const std::string a = i < actual.size() ? actual[i] : "<missing>";
        const std::string e = i < expected.size() ? expected[i] : "<missing>";
        if (a != e) {
            std::cout << "  line " << i << ": got \"" << a << "\", expected \""
                      << e << "\"\n";
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

// The 46 lines between the markers in the sub-stage 3f prompt
// (plan 5.4 and 5.5), in order; the first line is empty.
std::vector<std::string> expectedLines() {
    return {"",
            "Single-agent demonstrations use illustrative settings: budget 200 per step,",
            "safe-stop limit 5, step limit 100, active-test delay 3 (not the experiment's values).",
            "",
            "C1 demo 1: correct belief",
            "Plan: search found a plan of 4 actions",
            "Step 1: East | predicted Moved, observed Moved | match",
            "Step 2: East | predicted Pushed, observed Pushed | match",
            "Step 3: East | predicted Pushed, observed Pushed | match",
            "Step 4: East | predicted Pushed, observed Pushed | match",
            "Result: Success after 4 world steps, 0 safe stops",
            "Mismatches: 0",
            "Corrections: 0 (0 verified)",
            "Model evaluations used: 19",
            "",
            "C1 demo 2: belief wrongly has no wall at (4, 3)",
            "Plan: search found a plan of 4 actions",
            "Step 1: East | predicted Moved, observed Blocked | MISMATCH",
            "  - investigation: wall at (4, 3) believed free, recorded blocked (source S2)",
            "  - correction 1: wall at (4, 3) set to blocked: CONSISTENT",
            "Plan: fallback toward the best state found (10 actions)",
            "Step 2: North | predicted Moved, observed Moved | match",
            "Step 3: East | predicted Moved, observed Moved | match",
            "Step 4: East | predicted Moved, observed Moved | match",
            "Plan: active test of correction 1 (2 actions)",
            "Step 5: West | predicted Moved, observed Moved | match",
            "Step 6: South | predicted Blocked, observed Blocked | match",
            "  - correction 1: VERIFIED",
            "Plan: search found a plan of 12 actions",
            "Step 7: East | predicted Moved, observed Moved | match",
            "Step 8: South | predicted Pushed, observed Pushed | match",
            "Step 9: South | predicted Pushed, observed Pushed | match",
            "Step 10: West | predicted Moved, observed Moved | match",
            "Step 11: South | predicted Moved, observed Moved | match",
            "Step 12: East | predicted Pushed, observed Pushed | match",
            "Step 13: East | predicted Pushed, observed Pushed | match",
            "Step 14: East | predicted Pushed, observed Pushed | match",
            "Step 15: South | predicted Moved, observed Moved | match",
            "Step 16: East | predicted Moved, observed Moved | match",
            "Step 17: North | predicted Pushed, observed Pushed | match",
            "Step 18: North | predicted Pushed, observed Pushed | match",
            "Result: Success after 18 world steps, 0 safe stops",
            "Mismatches: 1",
            "Corrections: 1 (1 verified)",
            "Model evaluations used: 414",
            "Note: demo 2's wrong belief is hand-written; it is not fault injection."};
}

}  // namespace

void runAgentDemoTests(TestCounter& counter) {
    std::cout << "--- Agent demonstrations ---\n";
    // The layouts the demonstrations parse must be valid worlds.
    const auto demo = parseWorld({"##########", "#........#", "#........#",
                                  "#..A.O..D#", "#........#", "#........#",
                                  "#...##...#", "#........#", "#........#",
                                  "##########"});
    const auto walled = parseWorld({"##########", "#........#", "#........#",
                                    "#..A#O..D#", "#........#", "#........#",
                                    "#...##...#", "#........#", "#........#",
                                    "##########"});
    check(counter, demo.world && isValidWorld(*demo.world) && walled.world &&
                       isValidWorld(*walled.world),
          "AD1: demo layouts D and D2 are valid");
    std::ostringstream out;
    runAgentDemos(out);
    check(counter, linesEqual(splitLines(out.str()), expectedLines(), "AD1"),
          "AD1: runAgentDemos writes exactly the 46 expected lines "
          "(19 and 414 evaluations included)");
}

}  // namespace ai_hologram
