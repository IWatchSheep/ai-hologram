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
void runSingleAgentTests(TestCounter& counter);
void runNaiveAgentTests(TestCounter& counter);
void runAgentDemoTests(TestCounter& counter);
void runSeededRandomTests(TestCounter& counter);
void runLayoutGeneratorTests(TestCounter& counter);
void runLayoutGeneratorSetTests(TestCounter& counter);
void runFaultEventTests(TestCounter& counter);
void runFaultEventSetTests(TestCounter& counter);
void runFaultInjectorTests(TestCounter& counter);
void runFaultRunTests(TestCounter& counter);
void runScorerTests(TestCounter& counter);
void runScorerLifeTests(TestCounter& counter);
void runScorerDemoTests(TestCounter& counter);
void runShadowTests(TestCounter& counter);
void runShadowMoreTests(TestCounter& counter);
void runShadowSiteTests(TestCounter& counter);
void runShadowSiteMoreTests(TestCounter& counter);
void runShadowRunTests(TestCounter& counter);
void runShadowDemoTests(TestCounter& counter);

}  // namespace ai_hologram
