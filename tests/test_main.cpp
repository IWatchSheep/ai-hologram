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
    ai_hologram::runBudgetCounterTests(counter);
    ai_hologram::runWorldModelTests(counter);
    ai_hologram::runCompareTests(counter);
    ai_hologram::runFootprintTests(counter);
    ai_hologram::runDeadSquareTests(counter);
    ai_hologram::runStepPredictorTests(counter);
    ai_hologram::runPlannerTests(counter);
    ai_hologram::runInvestigationTests(counter);
    ai_hologram::runVerificationTests(counter);
    ai_hologram::runSingleAgentTests(counter);
    ai_hologram::runNaiveAgentTests(counter);
    ai_hologram::runAgentDemoTests(counter);
    ai_hologram::runSeededRandomTests(counter);
    ai_hologram::runLayoutGeneratorTests(counter);
    ai_hologram::runLayoutGeneratorSetTests(counter);
    ai_hologram::runFaultEventTests(counter);
    ai_hologram::runFaultEventSetTests(counter);
    ai_hologram::runFaultInjectorTests(counter);
    ai_hologram::runFaultRunTests(counter);
    ai_hologram::runScorerTests(counter);
    ai_hologram::runScorerLifeTests(counter);
    ai_hologram::runScorerDemoTests(counter);

    std::cout << counter.checks << " checks, " << counter.failures << " failures\n";
    return counter.failures == 0 ? 0 : 1;
}
