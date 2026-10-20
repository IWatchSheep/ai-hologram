// Purpose: Checks LG5 and LG6: whole pilot and confirmatory layout
// sets (material only) and repeat generation of one layout.
// Introduced in Stage 4.
// Architecture: These checks verify experimenter code only. No agent
// (C0 or C1) is run in this file; the pilot and confirmatory seed sets
// in LG5 are generated as material only and nothing is run on them.
// This file was split off from test_layout_generator.cpp so that file
// stays under about 200 lines with one statement per line. It prints
// no new section heading; LG5 and LG6 keep their original order.

#include "experiment/layout_generator.h"
#include "test_suites.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace ai_hologram {
namespace {

// One check per whole set (material only; no agent is run).
void checkWholeSet(TestCounter& counter, std::uint64_t first, int count,
                   int maxAttemptsExpected, int maxShortestExpected,
                   const std::string& label) {
    bool allFound = true;
    int maxAttempts = 0;
    int minShortest = 100;
    int maxShortest = 0;
    for (int i = 0; i < count; ++i) {
        const GeneratedLayout layout =
            generateLayout(first + static_cast<std::uint64_t>(i));
        allFound = allFound && layout.found;
        maxAttempts = std::max(maxAttempts, layout.attempts);
        minShortest = std::min(minShortest, layout.shortest);
        maxShortest = std::max(maxShortest, layout.shortest);
    }
    check(counter, allFound && maxAttempts == maxAttemptsExpected &&
              minShortest == 12 && maxShortest == maxShortestExpected,
          label);
}

}  // namespace

void runLayoutGeneratorSetTests(TestCounter& counter) {
    checkWholeSet(counter, 900000, 40, 10, 35,
                  "LG5: pilot 900000-900039 (material only; no agent run): "
                  "all found, largest attempts 10, shortest 12..35");
    checkWholeSet(counter, 100000, 400, 20, 38,
                  "LG5: confirmatory 100000-100399 (material only; no agent "
                  "run): all found, largest attempts 20, shortest 12..38");
    const GeneratedLayout first = generateLayout(500002);
    const GeneratedLayout second = generateLayout(500002);
    check(counter, first.found == second.found && first.world == second.world &&
              first.attempts == second.attempts &&
              first.shortest == second.shortest,
          "LG6: generateLayout(500002) twice gives identical results");
}

}  // namespace ai_hologram
