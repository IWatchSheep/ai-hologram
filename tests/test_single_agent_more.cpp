// Purpose: Checks C1-3, C1-4, C1-6, C1-7 and the rest of VF5
// for C1 (plan 6.4 and 6.8), using a hand-written run loop so the
// checks can look at C1 after every single decide() call.
// Introduced in Stage 3.
// Architecture: Split from test_single_agent.cpp so each file stays
// under about 200 lines. The hand loop mirrors runSingleAgent; the
// agent itself still never touches Environment (C1-4, by design:
// SingleAgent's interface takes only Observations).

#include "agent/run_loop.h"
#include "agent/single_agent.h"
#include "test_suites.h"
#include "world/environment.h"
#include "world/world_text.h"

#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

WorldState parseLayout(const std::vector<std::string>& rows) {
    return parseWorld(rows).world.value_or(WorldState{});
}

WorldState demoWorld() {  // layout D
    return parseLayout({"##########", "#........#", "#........#", "#..A.O..D#",
                        "#........#", "#........#", "#...##...#", "#........#",
                        "#........#", "##########"});
}

struct ManualRun {
    std::string outcome;
    int worldSteps = 0;
    bool allCommitted = true;
    bool withinBudget = true;
    bool beliefTrue = true;
    std::vector<std::string> stopReasons;
    std::vector<Observation> delivered;
};

ManualRun runManual(const WorldState& world, SingleAgent& agent,
                    const AgentSettings& limits) {
    ManualRun run;
    Environment environment(world);
    if (world.object == world.destination) {
        run.outcome = "Success";
        return run;
    }
    Observation current{1, world, std::nullopt, std::nullopt};
    while (true) {
        const bool willCheck = agent.state.committed.has;
        run.delivered.push_back(current);
        const Decision decision = agent.decide(current);
        if (agent.state.predictor.usedThisStep() > limits.budgetPerStep) {
            run.withinBudget = false;
        }
        if (willCheck && agent.state.belief != current.state) {
            run.beliefTrue = false;  // belief after the check (C1-3)
        }
        if (!decision.isAction) {
            run.stopReasons.push_back(decision.stopReason);
            if (agent.safeStopCount() >= limits.safeStopLimit) {
                run.outcome = "Stopped -- unresolved";
                return run;
            }
            current = Observation{agent.state.decisionStep + 1, current.state,
                                  std::nullopt, std::nullopt};
            continue;
        }
        if (!agent.state.committed.has) run.allCommitted = false;
        const ActionResult stepped = environment.step(decision.action);
        ++run.worldSteps;
        Observation next{agent.state.decisionStep + 1, environment.state(),
                         decision.action, stepped.outcome};
        if (environment.state().object == environment.state().destination) {
            agent.finalCheck(next);
            if (agent.state.belief != environment.state()) run.beliefTrue = false;
            run.outcome = "Success";
            return run;
        }
        if (run.worldSteps >= limits.maxWorldSteps) {
            agent.finalCheck(next);
            run.outcome = "Step limit";
            return run;
        }
        current = next;
    }
}

bool sameObservation(const Observation& left, const Observation& right) {
    return left.decisionStep == right.decisionStep && left.state == right.state &&
           left.lastAction == right.lastAction &&
           left.lastOutcome == right.lastOutcome;
}

void checkC1_3And4(TestCounter& counter) {
    const WorldState lp = parseLayout(
        {"##########", "#....D##.#", "#.O......#", "#....A#..#", "#.#......#",
         "#.....##.#", "#........#", "#........#", "#........#", "##########"});
    WorldState d2 = demoWorld();
    d2.walls[3][4] = true;
    const std::vector<WorldState> worlds{demoWorld(), lp, d2};
    bool allTrue = true;
    bool allValid = true;
    for (const WorldState& world : worlds) {
        allValid &= isValidWorld(world);
        const AgentSettings limits{200, 5, 100, 3};
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, limits);
        const ManualRun run = runManual(world, agent, limits);
        allTrue &= run.beliefTrue && run.outcome == "Success" &&
                   agent.mismatchCount() == 0;
        if (&world == &worlds[0]) {
            bool historySame = agent.state.history.size() >= run.delivered.size();
            for (std::size_t i = 0; i < run.delivered.size() && historySame; ++i) {
                historySame &= sameObservation(agent.state.history[i],
                                               run.delivered[i]);
            }
            check(counter, historySame,
                  "C1-4: every Observation equals the environment state "
                  "at that point and the history holds them unchanged, "
                  "in order (C1 reads no Environment, by design)");
        }
    }
    check(counter, allValid && allTrue,
          "C1-3: fault-free runs of D, LP and D2 (correct belief) keep "
          "the belief equal to the true state after every check, with "
          "no mismatches");
}

void checkC1_6AndVF5(TestCounter& counter) {
    const WorldState world = demoWorld();
    check(counter, isValidWorld(world), "C1-6: layout D is valid");
    SingleAgent cautious(Observation{1, world, std::nullopt, std::nullopt}, AgentSettings{200, 5, 100, 3});
    cautious.state.predictor.startStep();  // a fresh budget
    const auto pick = cautious.chooseCautiousAction();
    check(counter, pick.has_value() && pick->action == Action::North,
          "C1-6a: the cautious chooser alone picks the first action "
          "whose two predictions agree (North)");
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, AgentSettings{2, 5, 100, 3});
    WorldState afterOne = applyAction(world, Action::East).state;
    WorldState afterTwo = applyAction(afterOne, Action::East).state;
    agent.state.history.push_back(
        Observation{2, afterOne, Action::East, MoveOutcome::Moved});
    agent.state.history.push_back(
        Observation{3, afterTwo, Action::East, MoveOutcome::Pushed});
    agent.state.pendingV1 = ModelV1Work{Action::East, {1, 2}};
    const Decision stopped = agent.decide(Observation{1, world, std::nullopt, std::nullopt});
    check(counter, !stopped.isAction &&
                       stopped.stopReason ==
                           "investigation unfinished, no cautious action" &&
                       agent.state.pendingV1.has_value(),
          "C1-6b/VF5: V1 work that cannot finish within B leaves C1 "
          "safe-stopping 'investigation unfinished, no cautious action'");
    agent.decide(Observation{4, afterTwo, std::nullopt, std::nullopt});
    bool logged = false;
    for (const std::string& line : agent.logLines()) {
        logged |= line == "  - correction 1: action model for East "
                          "restored to the standard rule: CONSISTENT";
    }
    check(counter, logged && !agent.state.pendingV1.has_value() &&
                       agent.correctionCount() == 1 &&
                       agent.state.agentModel.isRestored(Action::East),
          "VF5 (rest): a later decision step continues the pending V1 "
          "work and creates the model correction when it is Done");
}

void checkC1_7(TestCounter& counter) {
    WorldState d2 = demoWorld();
    d2.walls[3][4] = true;
    const AgentSettings limits{200, 5, 100, 3};
    SingleAgent first(Observation{1, demoWorld(), std::nullopt, std::nullopt}, limits);
    const ManualRun runD = runManual(demoWorld(), first, limits);
    SingleAgent second(Observation{1, d2, std::nullopt, std::nullopt}, limits, demoWorld());
    const ManualRun runD2 = runManual(d2, second, limits);
    check(counter, runD.allCommitted && runD2.allCommitted &&
                       runD.withinBudget && runD2.withinBudget,
          "C1-7: in C1-1, C1-2 (and SP7, above) every world action has "
          "a committed prediction and stays within B");
}

}  // namespace

void checkSP7(TestCounter& counter);

void checkRemainingSingleAgent(TestCounter& counter) {
    checkSP7(counter);
    checkC1_3And4(counter);
    checkC1_6AndVF5(counter);
    checkC1_7(counter);
}

}  // namespace ai_hologram
