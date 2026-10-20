// Purpose: Declares the run with faults for C1 and C0: the Stage 3
// run loop plus fault injection at the activation point (plan 3.6).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. Like
// run_loop.cpp, it is the only code that touches Environment for its
// runs, and it builds every Observation itself. run_loop.* is not
// changed: the loop is written again here, so a run with an empty
// event list gives exactly the Stage 3 result (IE-T2).

#pragma once

#include "agent/naive_agent.h"
#include "agent/run_loop.h"
#include "agent/single_agent.h"
#include "experiment/fault_event.h"
#include "experiment/fault_injector.h"
#include "experiment/scorer.h"
#include "world/world_state.h"

#include <vector>

namespace ai_hologram {

// One run with faults: the Stage 3 RunResult, how many decision
// steps the agent took, and one activation record per event.
struct FaultRunResult {
    RunResult run;
    int decisionSteps = 0;
    std::vector<ActivationRecord> activations;
    std::vector<EventScore> eventScores;
    RunScore score;
    int observedEvaluations = 0;
};

// Exactly the Stage 3 runSingleAgent loop for C1, plus three things:
// the run's true observation history is kept for F5, a FaultInjector
// is connected to the agent's activation hook before the first
// decision, and the hook is emptied again before returning, on every
// return path, so the agent never points at a destroyed injector.
FaultRunResult runWithFaults(const WorldState& trueInitialState,
                             SingleAgent& agent,
                             const AgentSettings& settings,
                             const std::vector<FaultEvent>& events,
                             bool observe = true);

// The same for C0, exactly its Stage 3 loop (no final check).
FaultRunResult runWithFaults(const WorldState& trueInitialState,
                             NaiveAgent& agent,
                             const AgentSettings& settings,
                             const std::vector<FaultEvent>& events,
                             bool observe = true);

}  // namespace ai_hologram
