// Purpose: Implements the run with faults for C1 and C0 (plan 3.6):
// the Stage 3 run loop, written again here, plus the true history,
// the connected fault injector, and the activation records.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// RunResult lines are built exactly as run_loop.cpp builds them, with
// the same agent_log.h line functions. The true world is changed
// only through Environment, exactly as in the Stage 3 loop.

#include "experiment/fault_run.h"

#include "agent/agent_log.h"
#include "agent/computation_observer.h"
#include "experiment/scorer.h"
#include "world/environment.h"

#include <functional>
#include <utility>
#include <vector>

namespace ai_hologram {
namespace {

// Builds a result from the outcome and the counted facts of a run,
// exactly as run_loop.cpp does: the log lines, then the four result
// lines of plan 5.3.
RunResult buildRunResult(const std::string& outcome, int worldSteps,
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

// Empties C1's hook first, so the agent is never left pointing at a
// destroyed injector, then finishes its run exactly as run_loop does.
FaultRunResult finishRun(SingleAgent& agent, FaultInjector& injector,
                         Scorer& scorer,
                         const std::string& outcome, int worldSteps) {
    agent.state.activationHook = std::function<void(int)>{};
    agent.state.predictor.setObserver(nullptr);
    agent.state.deadCache.observer = nullptr;
    scorer.finish(viewOf(agent.state));
    FaultRunResult result;
    result.run = buildRunResult(outcome, worldSteps, agent.safeStopCount(),
                                agent.mismatchCount(),
                                agent.correctionCount(),
                                agent.verifiedCount(),
                                agent.totalEvaluations(), agent.logLines());
    result.decisionSteps = agent.state.decisionStep;
    result.activations = injector.records();
    result.eventScores = scorer.events();
    result.score = scorer.run();
    result.observedEvaluations = scorer.observedEvaluations();
    return result;
}

// The same for C0, whose counts are 0 and which writes no log lines.
FaultRunResult finishRun(NaiveAgent& agent, FaultInjector& injector,
                         Scorer& scorer,
                         const std::string& outcome, int worldSteps) {
    agent.state.activationHook = std::function<void(int)>{};
    agent.state.predictor.setObserver(nullptr);
    agent.state.deadCache.observer = nullptr;
    scorer.finish(viewOf(agent.state));
    FaultRunResult result;
    result.run = buildRunResult(outcome, worldSteps, 0, 0, 0, 0,
                                agent.totalEvaluations(), {});
    result.decisionSteps = agent.state.decisionStep;
    result.activations = injector.records();
    result.eventScores = scorer.events();
    result.score = scorer.run();
    result.observedEvaluations = scorer.observedEvaluations();
    return result;
}

Observation nextObservation(const WorldState& state, int decisionStep) {
    return Observation{decisionStep, state, std::nullopt, std::nullopt};
}

// Attaches the watch-only observer (IE-1 H3/H4) to one agent for a
// run with observe true. The observer only calls the scorer; the
// agent reads nothing back from it, so it cannot pull (see
// computation_observer.h on the camera and the chain).
void attachObserver(Scorer& scorer, ComputationObserver& observer,
                    const ScoredView& view, StepPredictor& predictor,
                    DeadSquareCache& cache) {
    scorer.attachView(view);
    observer.onEvaluation = [&scorer](const WorldState& state,
                                      const Action action,
                                      const ActionModel* variant,
                                      const Prediction& result) {
        scorer.onEvaluation(state, action, variant, result);
    };
    observer.onDeadRead = [&scorer](const Position cell, const bool value) {
        scorer.onDeadRead(cell, value);
    };
    observer.onHeuristic = [&scorer](const WorldState& state, const int value) {
        scorer.onHeuristic(state, value);
    };
    predictor.setObserver(&observer);
    cache.observer = &observer;
}

}  // namespace

FaultRunResult runWithFaults(const WorldState& trueInitialState,
                             SingleAgent& agent,
                             const AgentSettings& settings,
                             const std::vector<FaultEvent>& events,
                             const bool observe) {
    Environment environment(trueInitialState);
    std::vector<Observation> trueHistory;
    FaultInjector injector(events, &trueHistory);
    Scorer scorer(events, trueInitialState, &trueHistory);
    ComputationObserver observer;
    if (observe) {
        attachObserver(scorer, observer, viewOf(agent.state),
                       agent.state.predictor, agent.state.deadCache);
    }
    agent.state.activationHook = [&scorer, &injector, &agent](int step) {
        scorer.endPhaseA(viewOf(agent.state), step);
        injector.atActivationPoint(partsOf(agent.state), step);
        scorer.afterActivation(viewOf(agent.state), injector.records(), step);
    };
    if (trueInitialState.object == trueInitialState.destination) {
        return finishRun(agent, injector, scorer, "Success", 0);
    }
    Observation current = nextObservation(trueInitialState, 1);
    trueHistory.push_back(current);
    int worldSteps = 0;
    while (true) {
        const Decision decision = agent.decide(current);
        scorer.afterDecide(viewOf(agent.state), decision);
        if (!decision.isAction) {
            if (agent.safeStopCount() >= settings.safeStopLimit) {
                return finishRun(agent, injector, scorer,
                                 "Stopped -- unresolved", worldSteps);
            }
            current = nextObservation(current.state,
                                      agent.state.decisionStep + 1);
            trueHistory.push_back(current);
            continue;
        }
        const ActionResult stepped = environment.step(decision.action);
        ++worldSteps;
        Observation next{agent.state.decisionStep + 1, environment.state(),
                         decision.action, stepped.outcome};
        trueHistory.push_back(next);
        if (environment.state().object == environment.state().destination) {
            agent.finalCheck(next);
            scorer.endFinalCheck(viewOf(agent.state));
            return finishRun(agent, injector, scorer, "Success", worldSteps);
        }
        if (worldSteps >= settings.maxWorldSteps) {
            agent.finalCheck(next);
            scorer.endFinalCheck(viewOf(agent.state));
            return finishRun(agent, injector, scorer, "Step limit", worldSteps);
        }
        current = next;
    }
}

FaultRunResult runWithFaults(const WorldState& trueInitialState,
                             NaiveAgent& agent,
                             const AgentSettings& settings,
                             const std::vector<FaultEvent>& events,
                             const bool observe) {
    Environment environment(trueInitialState);
    std::vector<Observation> trueHistory;
    FaultInjector injector(events, &trueHistory);
    Scorer scorer(events, trueInitialState, &trueHistory);
    ComputationObserver observer;
    if (observe) {
        attachObserver(scorer, observer, viewOf(agent.state),
                       agent.state.predictor, agent.state.deadCache);
    }
    agent.state.activationHook = [&scorer, &injector, &agent](int step) {
        scorer.endPhaseA(viewOf(agent.state), step);
        injector.atActivationPoint(partsOf(agent.state), step);
        scorer.afterActivation(viewOf(agent.state), injector.records(), step);
    };
    if (trueInitialState.object == trueInitialState.destination) {
        return finishRun(agent, injector, scorer, "Success", 0);
    }
    Observation current = nextObservation(trueInitialState, 1);
    trueHistory.push_back(current);
    int worldSteps = 0;
    while (true) {
        const Decision decision = agent.decide(current);
        scorer.afterDecide(viewOf(agent.state), decision);
        const ActionResult stepped = environment.step(decision.action);
        ++worldSteps;
        Observation next{agent.state.decisionStep + 1, environment.state(),
                         decision.action, stepped.outcome};
        trueHistory.push_back(next);
        if (environment.state().object == environment.state().destination) {
            return finishRun(agent, injector, scorer, "Success", worldSteps);
        }
        if (worldSteps >= settings.maxWorldSteps) {
            return finishRun(agent, injector, scorer, "Step limit", worldSteps);
        }
        current = next;
    }
}

}  // namespace ai_hologram
