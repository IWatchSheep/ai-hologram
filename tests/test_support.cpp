// Purpose: Prints test results and updates their shared check counter.
// Introduced in Stage 1.
// Architecture: The test executable uses this helper across all test groups.

#include "test_support.h"

#include <iostream>

namespace ai_hologram {

void check(TestCounter& counter, const bool condition, const std::string& description) {
    ++counter.checks;

    // An ostream reference lets this function print without owning the console.
    std::ostream& output = std::cout;
    if (condition) {
        output << "PASS: " << description << '\n';
        return;
    }

    ++counter.failures;
    output << "FAIL: " << description << '\n';
}

}  // namespace ai_hologram
