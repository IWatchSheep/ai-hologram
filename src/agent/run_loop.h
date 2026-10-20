// Purpose: Declares the run loop that runs one agent in the true
// world and reports the run's outcome (plan 3.10).
// Introduced in Stage 3.
// Architecture: This is the ONLY code that touches Environment for an
// agent run. It builds every Observation from Environment and hands it
// to the agent; the agent itself never reads Environment.

#pragma once

#include "agent/single_agent.h"
#include "world/world_state.h"

#include <string>
#include <vector>

namespace ai_hologram {

struct RunResult {
    std::string outcome;  // "Success", "Step limit" or "Stopped -- unresolved"
    int worldSteps = 0;
    int safeStops = 0;
    int mismatches = 0;
    int corrections = 0;
    int verified = 0;
    int evaluations = 0;
    std::vector<std::string> lines;
};

RunResult runSingleAgent(const WorldState& trueInitialState,
                         SingleAgent& agent, const AgentSettings& settings);

}  // namespace ai_hologram
