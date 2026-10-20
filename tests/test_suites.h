// Purpose: Declares each correctness-test group.
// Introduced in Stage 1.
// Architecture: test_main.cpp calls these groups and shares their counter.

#pragma once

#include "test_support.h"

namespace ai_hologram {

void runProjectInfoTests(TestCounter& counter);
void runWorldTextTests(TestCounter& counter);
void runRulesTests(TestCounter& counter);
void runEnvironmentTests(TestCounter& counter);
void runBudgetCounterTests(TestCounter& counter);
void runWorldModelTests(TestCounter& counter);
void runCompareTests(TestCounter& counter);
void runFootprintTests(TestCounter& counter);
void runDeadSquareTests(TestCounter& counter);
void runStepPredictorTests(TestCounter& counter);
void runPlannerTests(TestCounter& counter);
void runInvestigationTests(TestCounter& counter);
void runVerificationTests(TestCounter& counter);

}  // namespace ai_hologram
