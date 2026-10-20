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

#include <utility>

namespace ai_hologram {
namespace {

// Builds a result from the outcome and the counted facts of a run.
// `lines` holds any log lines written during the run (none for C0);
// the four result lines of plan 5.3 are appended here for both agents.
RunResult buildResult(const std::string& outcome, int worldSteps,
                      int safeStops, int mismatches, int corrections,
                      int verified, int evaluations,
                      std::vector<std::string> lines) {
    RunResult result;
    result.outcome = outcome;
    result.worldSteps = worldSteps;
    result.safeStops = safeStops;
    result.mismatches = mismatches;
    result.corrections = corrections;
    result.verified = verified;
    result.evaluations = evaluations;
    result.lines = std::move(lines);
    result.lines.push_back(resultLine(outcome, worldSteps, safeStops));
    result.lines.push_back(mismatchesLine(mismatches));
    result.lines.push_back(correctionsLine(corrections, verified));
    result.lines.push_back(evaluationsLine(evaluations));
    return result;
}

RunResult finish(SingleAgent& agent, const std::string& outcome, int worldSteps) {
    return buildResult(outcome, worldSteps, agent.safeStopCount(),
                       agent.mismatchCount(), agent.correctionCount(),
                       agent.verifiedCount(), agent.totalEvaluations(),
                       agent.logLines());
}

// C0 makes no checks, records or safe stops (plan 3.11), so its
// counts are 0 by definition and it writes no log lines.
RunResult finish(NaiveAgent& agent, const std::string& outcome, int worldSteps) {
    return buildResult(outcome, worldSteps, 0, 0, 0, 0,
                       agent.totalEvaluations(), {});
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

RunResult runSingleAgent(const WorldState& trueInitialState,
                         NaiveAgent& agent, const AgentSettings& settings) {
    Environment environment(trueInitialState);
    if (trueInitialState.object == trueInitialState.destination) {
        return finish(agent, "Success", 0);  // no decision is made (3.10)
    }
    Observation current =
        nextObservation(trueInitialState, 1);  // delivered at step 1
    int worldSteps = 0;
    while (true) {
        // C0 always returns an action: it never safe-stops, so this
        // loop can end only with Success or Step limit (plan 3.11),
        // and there is no final check of a last prediction.
        const Decision decision = agent.decide(current);
        const ActionResult stepped = environment.step(decision.action);
        ++worldSteps;
        Observation next{agent.state.decisionStep + 1, environment.state(),
                         decision.action, stepped.outcome};
        if (environment.state().object ==
            environment.state().destination) {
            return finish(agent, "Success", worldSteps);
        }
        if (worldSteps >= settings.maxWorldSteps) {
            return finish(agent, "Step limit", worldSteps);
        }
        current = next;
    }
}

}  // namespace ai_hologram
