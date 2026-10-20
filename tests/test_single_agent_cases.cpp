// Purpose: Checks C1-2 (demo 2) and VF6 (a REJECTED correction) on
// their exact expected log lines from the prompt.
// Introduced in Stage 3.
// Architecture: Split from test_single_agent.cpp so each file stays
// under about 200 lines. Every world is checked valid.

#include "agent/run_loop.h"
#include "agent/single_agent.h"
#include "test_suites.h"
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

bool linesEqual(const std::vector<std::string>& actual,
                const std::vector<std::string>& expected,
                const std::string& label) {
    if (actual == expected) return true;
    std::cout << "First differing line in " << label << ":\n";
    const std::size_t count = std::max(actual.size(), expected.size());
    for (std::size_t i = 0; i < count; ++i) {
        const std::string a = i < actual.size() ? actual[i] : "<missing>";
        const std::string e = i < expected.size() ? expected[i] : "<missing>";
        if (a != e) {
            std::cout << "  line " << i << ": got \"" << a << "\", expected \""
                      << e << "\"\n";
            break;
        }
    }
    return false;
}

}  // namespace

void checkC1_2(TestCounter& counter) {
    WorldState trueWorld = demoWorld();
    trueWorld.walls[3][4] = true;  // D2: D plus a wall at (4, 3)
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(trueWorld) && isValidWorld(belief),
          "C1-2: true world D2 and starting belief D are valid");
    const AgentSettings limits{200, 5, 100, 3};
    SingleAgent agent(Observation{1, trueWorld, std::nullopt, std::nullopt}, limits, belief);
    const RunResult result = runSingleAgent(trueWorld, agent, limits);
    const std::vector<std::string> expected{
        "Plan: search found a plan of 4 actions",
        "Step 1: East | predicted Moved, observed Blocked | MISMATCH",
        "  - investigation: wall at (4, 3) believed free, recorded blocked (source S2)",
        "  - correction 1: wall at (4, 3) set to blocked: CONSISTENT",
        "Plan: fallback toward the best state found (10 actions)",
        "Step 2: North | predicted Moved, observed Moved | match",
        "Step 3: East | predicted Moved, observed Moved | match",
        "Step 4: East | predicted Moved, observed Moved | match",
        "Plan: active test of correction 1 (2 actions)",
        "Step 5: West | predicted Moved, observed Moved | match",
        "Step 6: South | predicted Blocked, observed Blocked | match",
        "  - correction 1: VERIFIED",
        "Plan: search found a plan of 12 actions",
        "Step 7: East | predicted Moved, observed Moved | match",
        "Step 8: South | predicted Pushed, observed Pushed | match",
        "Step 9: South | predicted Pushed, observed Pushed | match",
        "Step 10: West | predicted Moved, observed Moved | match",
        "Step 11: South | predicted Moved, observed Moved | match",
        "Step 12: East | predicted Pushed, observed Pushed | match",
        "Step 13: East | predicted Pushed, observed Pushed | match",
        "Step 14: East | predicted Pushed, observed Pushed | match",
        "Step 15: South | predicted Moved, observed Moved | match",
        "Step 16: East | predicted Moved, observed Moved | match",
        "Step 17: North | predicted Pushed, observed Pushed | match",
        "Step 18: North | predicted Pushed, observed Pushed | match",
        "Result: Success after 18 world steps, 0 safe stops",
        "Mismatches: 1", "Corrections: 1 (1 verified)",
        "Model evaluations used: 414"};
    int activeAt = -1;
    for (std::size_t i = 0; i < result.lines.size(); ++i) {
        if (result.lines[i].rfind("Plan: active test", 0) == 0) {
            activeAt = static_cast<int>(i);
        }
    }
    check(counter, linesEqual(result.lines, expected, "C1-2") &&
                       result.outcome == "Success" && activeAt == 8,
          "C1-2/C1-8: demo 2 lines match exactly (414 evaluations, one "
          "VERIFIED correction; the active test starts only after 3 "
          "decision steps, at log line 8 after Steps 1-4)");
}

void checkVF6(TestCounter& counter) {
    WorldState belief = demoWorld();
    belief.walls[3][4] = true;  // the WRONG wall at (4, 3)
    const WorldState world = demoWorld();
    check(counter, isValidWorld(belief) && isValidWorld(world),
          "VF6: true world D and wrong belief D+wall(4,3) are valid");
    const AgentSettings limits{200, 5, 100, 3};
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, limits, belief);
    CorrectionRecord hand;
    hand.id = 1;
    hand.element.kind = ElementKind::Wall;
    hand.element.cell = {4, 3};
    hand.newBlocked = true;
    hand.oldBlocked = false;
    hand.status = CorrectionStatus::Consistent;
    hand.stepConsistent = 0;
    hand.tested = false;
    agent.state.corrections.push_back(hand);
    agent.state.nextCorrectionId = 2;
    const RunResult result = runSingleAgent(world, agent, limits);
    const std::vector<std::string> expected{
        "Plan: fallback toward the best state found (10 actions)",
        "Step 1: North | predicted Moved, observed Moved | match",
        "Step 2: East | predicted Moved, observed Moved | match",
        "Plan: active test of correction 1 (1 actions)",
        "Step 3: South | predicted Blocked, observed Moved | MISMATCH",
        "  - correction 1: REJECTED",
        "  - investigation: wall at (4, 3) believed blocked, recorded free (source S2)",
        "  - correction 2: wall at (4, 3) set to free: CONSISTENT",
        "Plan: search found a plan of 3 actions",
        "Step 4: East | predicted Pushed, observed Pushed | match",
        "Step 5: East | predicted Pushed, observed Pushed | match",
        "Step 6: East | predicted Pushed, observed Pushed | match",
        "Result: Success after 6 world steps, 0 safe stops",
        "Mismatches: 1", "Corrections: 2 (0 verified)",
        "Model evaluations used: 216"};
    check(counter, linesEqual(result.lines, expected, "VF6") &&
                       result.outcome == "Success",
          "VF6/C1-8: a REJECTED wall correction is reverted and the new "
          "correction created; lines match exactly (216 evaluations)");
}

void checkDeferred(TestCounter& counter) {
    WorldState trueWorld = demoWorld();
    trueWorld.walls[3][4] = true;  // D2
    const AgentSettings limits{1, 5, 100, 3};  // B = 1, reserve = 1
    SingleAgent agent(Observation{1, trueWorld, std::nullopt, std::nullopt},
                      limits, demoWorld());
    agent.state.decisionStep = 1;
    agent.state.committed.has = true;
    agent.state.committed.prediction =
        Prediction{Action::East, MoveOutcome::Moved,
                   applyAction(demoWorld(), Action::East).state};
    agent.state.committed.beliefBefore = demoWorld();
    agent.state.committed.decisionStep = 1;
    const ActionResult blocked = applyAction(trueWorld, Action::East);
    const Decision first = agent.decide(
        Observation{2, blocked.state, Action::East, blocked.outcome});
    bool deferredLogged = false;
    for (const std::string& line : agent.logLines()) {
        deferredLogged |= line == "  - investigation: budget too small "
                                  "for verification; retried next step";
    }
    const Decision second = agent.decide(
        Observation{3, blocked.state, std::nullopt, std::nullopt});
    check(counter, isValidWorld(trueWorld) && !first.isAction &&
                       first.stopReason == "budget too small for verification" &&
                       !second.isAction &&
                       second.stopReason == "budget too small for verification" &&
                       deferredLogged && agent.mismatchCount() == 1 &&
                       agent.correctionCount() == 0,
          "C1-11: an investigation whose re-prediction cannot be "
          "afforded is deferred, logged, and retried next step, "
          "safe-stopping 'budget too small for verification'");
    SingleAgent noBudget(Observation{1, demoWorld(), std::nullopt,
                                     std::nullopt},
                         AgentSettings{0, 5, 100, 3});
    noBudget.state.plan = {PlanStep{Action::East, demoWorld()}};
    noBudget.state.planKind = PlanKind::Goal;
    const Decision stopped = noBudget.decide(
        Observation{1, demoWorld(), std::nullopt, std::nullopt});
    check(counter, !stopped.isAction &&
                       stopped.stopReason == "no budget for a committed prediction",
          "C1-11: with no budget for a committed prediction C1 takes "
          "no world action and safe-stops with that reason");
}

}  // namespace ai_hologram
