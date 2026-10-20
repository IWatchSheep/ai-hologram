// Purpose: Runs one agent in the true world (plan 3.10) and builds
// the run result: the agent's log lines followed by the four result
// lines of plan 5.3.
// Introduced in Stage 3.
// Architecture: Only this file touches Environment for an agent run.
// The agent sees the world only through the Observations built here,
// each one delivered unchanged at its decision step.

#include "agent/run_loop.h"
#include "agent/agent_log.h"
#include "world/environment.h"

namespace ai_hologram {
namespace {

RunResult finish(SingleAgent& agent, const std::string& outcome, int worldSteps) {
    RunResult result;
    result.outcome = outcome;
    result.worldSteps = worldSteps;
    result.safeStops = agent.safeStopCount();
    result.mismatches = agent.mismatchCount();
    result.corrections = agent.correctionCount();
    result.verified = agent.verifiedCount();
    result.evaluations = agent.totalEvaluations();
    result.lines = agent.logLines();
    result.lines.push_back(resultLine(outcome, worldSteps, result.safeStops));
    result.lines.push_back(mismatchesLine(result.mismatches));
    result.lines.push_back(correctionsLine(result.corrections, result.verified));
    result.lines.push_back(evaluationsLine(result.evaluations));
    return result;
}

Observation nextObservation(const WorldState& state, int decisionStep) {
    return Observation{decisionStep, state, std::nullopt, std::nullopt};
}

}  // namespace

RunResult runSingleAgent(const WorldState& trueInitialState,
                         SingleAgent& agent, const AgentSettings& settings) {
    Environment environment(trueInitialState);
    if (trueInitialState.object == trueInitialState.destination) {
        return finish(agent, "Success", 0);  // no decision is made (3.10)
    }
    Observation current =
        nextObservation(trueInitialState, 1);  // delivered at step 1
    int worldSteps = 0;
    while (true) {
        const Decision decision = agent.decide(current);
        if (!decision.isAction) {
            if (agent.safeStopCount() >= settings.safeStopLimit) {
                return finish(agent, "Stopped -- unresolved", worldSteps);
            }
            // A safe stop repeats the state, with no action or outcome.
            current = nextObservation(current.state,
                                      agent.state.decisionStep + 1);
            continue;
        }
        const ActionResult stepped = environment.step(decision.action);
        ++worldSteps;
        Observation next{agent.state.decisionStep + 1, environment.state(),
                         decision.action, stepped.outcome};
        if (environment.state().object ==
            environment.state().destination) {
            agent.finalCheck(next);  // the last step is still checked
            return finish(agent, "Success", worldSteps);
        }
        if (worldSteps >= settings.maxWorldSteps) {
            agent.finalCheck(next);
            return finish(agent, "Step limit", worldSteps);
        }
        current = next;
    }
}

}  // namespace ai_hologram
