// Purpose: Runs each correctness-test group and prints the shared summary.
// Introduced in Stage 1.
// Architecture: The test executable links test groups to the shared core library.

#include "test_suites.h"

#include <iostream>

int main() {
    ai_hologram::TestCounter counter;
    ai_hologram::runProjectInfoTests(counter);
    ai_hologram::runWorldTextTests(counter);
    ai_hologram::runRulesTests(counter);
    ai_hologram::runEnvironmentTests(counter);

    std::cout << counter.checks << " checks, " << counter.failures << " failures\n";
    return counter.failures == 0 ? 0 : 1;
}
