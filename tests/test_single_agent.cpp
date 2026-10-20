// Purpose: Checks C1 as a whole (plan 6.8) on the exact expected log
// lines from the prompt: C1-1, C1-5, C1-9 and C1-12.
// Introduced in Stage 3.
// Architecture: C1-2 and VF6 live in test_single_agent_cases.cpp and
// the remaining checks in test_single_agent_more.cpp, so each file
// stays under about 200 lines. Every world is checked valid.

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

AgentSettings settings(int budget = 200, int safeStops = 5, int steps = 100) {
    return AgentSettings{budget, safeStops, steps, 3};
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

void checkC1_1(TestCounter& counter) {
    const WorldState world = demoWorld();
    check(counter, isValidWorld(world), "C1-1: layout D is valid");
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, settings());
    const RunResult result = runSingleAgent(world, agent, settings());
    const std::vector<std::string> expected{
        "Plan: search found a plan of 4 actions",
        "Step 1: East | predicted Moved, observed Moved | match",
        "Step 2: East | predicted Pushed, observed Pushed | match",
        "Step 3: East | predicted Pushed, observed Pushed | match",
        "Step 4: East | predicted Pushed, observed Pushed | match",
        "Result: Success after 4 world steps, 0 safe stops",
        "Mismatches: 0", "Corrections: 0 (0 verified)",
        "Model evaluations used: 19"};
    check(counter, linesEqual(result.lines, expected, "C1-1") &&
                       result.outcome == "Success" && result.worldSteps == 4,
          "C1-1: demo 1 lines match exactly (19 evaluations)");
    check(counter, result.lines.size() == expected.size() &&
                       result.lines[4].rfind("Step 4:", 0) == 0,
          "C1-10: C1-1's last step line is present only because the "
          "final check logged it after the last world action");
}

void checkC1_5(TestCounter& counter) {
    const WorldState world = parseLayout(
        {"##########", "#O.......#", "#........#", "#........#", "#....A...#",
         "#........#", "#........#", "#.......D#", "#........#", "##########"});
    check(counter, isValidWorld(world) && world.object == Position{1, 1},
          "C1-5: layout LT is valid, object on the dead corner (1, 1)");
    const AgentSettings limits = settings(200, 2);
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, limits);
    const RunResult result = runSingleAgent(world, agent, limits);
    const std::vector<std::string> expected{
        "Step 1: safe stop (no safe action)",
        "Step 2: safe stop (no safe action)",
        "Result: Stopped -- unresolved after 0 world steps, 2 safe stops",
        "Mismatches: 0", "Corrections: 0 (0 verified)",
        "Model evaluations used: 8"};
    check(counter, linesEqual(result.lines, expected, "C1-5") &&
                       agent.state.decisionStep == 2 &&
                       agent.state.history.size() == 2 &&
                       !agent.state.history[1].lastAction.has_value() &&
                       !agent.state.history[1].lastOutcome.has_value(),
          "C1-5: two safe stops, each a decision step (decisionStep == "
          "2), and the observation after a safe stop has no last action");
}

void checkC1_12(TestCounter& counter) {
    const WorldState goalWorld = parseLayout(
        {"##########", "#........#", "#........#", "#..A....*#", "#........#",
         "#........#", "#...##...#", "#........#", "#........#", "##########"});
    check(counter, isValidWorld(goalWorld), "C1-12: layout LG is valid");
    SingleAgent goalAgent(Observation{1, goalWorld, std::nullopt, std::nullopt}, settings());
    const RunResult goal = runSingleAgent(goalWorld, goalAgent, settings());
    check(counter, goal.outcome == "Success" && goal.worldSteps == 0 &&
                       goal.evaluations == 0 && goalAgent.state.decisionStep == 0,
          "C1-12a: object already on the destination: Success, 0 world "
          "steps, 0 evaluations, decide() never called");
    WorldState belief = demoWorld();
    belief.object = {8, 3};  // hand-written wrong belief: object on D
    check(counter, isValidWorld(belief), "C1-12: wrong belief is valid");
    const AgentSettings limits = settings(200, 2);
    const WorldState world = demoWorld();
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, limits, belief);
    const RunResult result = runSingleAgent(world, agent, limits);
    const std::vector<std::string> expected{
        "Step 1: safe stop (belief shows goal reached)",
        "Step 2: safe stop (belief shows goal reached)",
        "Result: Stopped -- unresolved after 0 world steps, 2 safe stops",
        "Mismatches: 0", "Corrections: 0 (0 verified)",
        "Model evaluations used: 0"};
    check(counter, linesEqual(result.lines, expected, "C1-12b"),
          "C1-12b: belief shows goal reached, twice, with 0 evaluations");
}

// A hand-made model: when East would push, the object moves North.
ActionModel eastPushNorthModel() {
    return [](const WorldState& state, Action action) -> ActionResult {
        const ActionResult standard = applyAction(state, action);
        if (action != Action::East || standard.outcome != MoveOutcome::Pushed) {
            return standard;
        }
        const Position north = moved(state.object, Action::North);
        if (isWall(state, north)) return {state, MoveOutcome::Blocked};
        WorldState wrong = state;
        wrong.agent = state.object;
        wrong.object = north;
        return {wrong, MoveOutcome::Pushed};
    };
}

void checkC1_9(TestCounter& counter) {
    const WorldState world = demoWorld();
    const AgentSettings limits = settings(200, 5, 6);
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, limits, std::nullopt,
                      eastPushNorthModel());
    const RunResult result = runSingleAgent(world, agent, limits);
    const std::vector<std::string> expected{
        "Plan: fallback toward the best state found (1 actions)",
        "Step 1: East | predicted Moved, observed Moved | match",
        "Plan: fallback toward the best state found (2 actions)",
        "Step 2: North | predicted Moved, observed Moved | match",
        "Step 3: East | predicted Moved, observed Moved | match",
        "Plan: fallback toward the best state found (2 actions)",
        "Step 4: East | predicted Moved, observed Moved | match",
        "Step 5: South | predicted Moved, observed Moved | match",
        "Plan: fallback toward the best state found (2 actions)",
        "Step 6: North | predicted Moved, observed Moved | match",
        "Result: Step limit after 6 world steps, 0 safe stops",
        "Mismatches: 0", "Corrections: 0 (0 verified)",
        "Model evaluations used: 798"};
    check(counter, linesEqual(result.lines, expected, "C1-9") &&
                       result.outcome == "Step limit",
          "C1-9: step-limit lines match exactly (798 evaluations); a "
          "faulty model never used in a checked prediction is never "
          "noticed (plan 5.6)");
}

}  // namespace

void checkC1_2(TestCounter& counter);
void checkVF6(TestCounter& counter);
void checkDeferred(TestCounter& counter);
void checkRemainingSingleAgent(TestCounter& counter);
void checkC1_6c(TestCounter& counter);

void runSingleAgentTests(TestCounter& counter) {
    std::cout << "--- Single agent (C1) ---\n";
    checkC1_1(counter);
    checkC1_2(counter);
    checkC1_5(counter);
    checkC1_12(counter);
    checkC1_9(counter);
    checkVF6(counter);
    checkDeferred(counter);
    checkRemainingSingleAgent(counter);
    checkC1_6c(counter);
}

}  // namespace ai_hologram
