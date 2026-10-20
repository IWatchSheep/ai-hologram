// Purpose: Checks SP7 for C1 (plan 6.4): several pending V2
// corrections at every budget B from 1 to 11.
// Introduced in Stage 3.
// Architecture: Split from test_single_agent_more.cpp so each file
// stays under about 200 lines. The hand loop mirrors runSingleAgent
// so the check can look at usedThisStep() after every decide().

#include "agent/single_agent.h"
#include "test_suites.h"
#include "world/environment.h"
#include "world/world_text.h"

#include <string>
#include <vector>

namespace ai_hologram {
namespace {

WorldState demoWorld() {  // layout D
    return parseWorld({"##########", "#........#", "#........#", "#..A.O..D#",
                       "#........#", "#........#", "#...##...#", "#........#",
                       "#........#", "##########"})
        .world.value_or(WorldState{});
}

struct ManualRun {
    std::string outcome;
    int worldSteps = 0;
    bool allCommitted = true;
    bool withinBudget = true;
    std::vector<std::string> stopReasons;
};

ManualRun runManual(const WorldState& world, SingleAgent& agent,
                    const AgentSettings& limits) {
    ManualRun run;
    Environment environment(world);
    Observation current{1, world, std::nullopt, std::nullopt};
    while (true) {
        const Decision decision = agent.decide(current);
        if (agent.state.predictor.usedThisStep() > limits.budgetPerStep) {
            run.withinBudget = false;
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

}  // namespace

void checkSP7(TestCounter& counter) {
    const WorldState world = demoWorld();
    check(counter, isValidWorld(world), "SP7: layout D is valid");
    for (int budget = 1; budget <= 11; ++budget) {
        const AgentSettings limits{budget, 3, 30, 3};
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          limits);
        const Position cells[3] = {{4, 3}, {4, 2}, {3, 4}};
        for (int i = 0; i < 3; ++i) {
            CorrectionRecord record;
            record.id = i + 1;
            record.element.kind = ElementKind::Wall;
            record.element.cell = cells[i];
            record.newBlocked = false;
            record.oldBlocked = true;
            record.status = CorrectionStatus::Consistent;
            record.stepConsistent = 0;
            agent.state.corrections.push_back(record);
        }
        agent.state.nextCorrectionId = 4;
        const ManualRun run = runManual(world, agent, limits);
        std::string outcome = "Step limit";
        int steps = 30;
        int stops = 0;
        if (budget <= 4) {
            outcome = "Stopped -- unresolved"; steps = 0; stops = 3;
        }
        if (budget == 5) {
            outcome = "Stopped -- unresolved"; steps = 2; stops = 3;
        }
        if (budget == 6 || budget == 7) { outcome = "Success"; steps = 4; }
        if (budget == 11) { outcome = "Success"; steps = 12; }
        bool smallReasons = true;
        if (budget <= 4) {
            for (const std::string& reason : run.stopReasons) {
                smallReasons &= reason == "budget too small to plan";
            }
            smallReasons &= run.worldSteps == 0 && run.stopReasons.size() == 3;
        }
        check(counter, run.withinBudget && run.allCommitted && smallReasons &&
                           run.outcome == outcome && run.worldSteps == steps &&
                           agent.safeStopCount() == stops,
              "SP7/C1-7: B=" + std::to_string(budget) + " gives " + outcome +
                  " / " + std::to_string(steps) + " world steps, no step "
                  "over B and every action committed");
    }
}

}  // namespace ai_hologram
