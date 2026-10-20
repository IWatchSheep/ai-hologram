// Purpose: Checks C1-6c: REPEATED safe stops while the V1 work for
// an action model is still unfinished, and how those safe stops can
// end a run when the safe-stop limit S_max is reached.
// Introduced in Stage 3.
// Architecture: Split from test_single_agent_more.cpp so that file
// stays under about 200 lines. Like the other single-agent tests,
// this file builds a SingleAgent by hand, runs it with
// runSingleAgent, and compares the whole log with the exact lines.

#include "agent/run_loop.h"
#include "agent/single_agent.h"
#include "test_suites.h"
#include "world/rules.h"
#include "world/world_text.h"

#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

WorldState parseLayout(const std::vector<std::string>& rows) {
    return parseWorld(rows).world.value_or(WorldState{});
}

WorldState demoWorld() {  // layout D, the Stage 1 demo layout
    return parseLayout({"##########", "#........#", "#........#", "#..A.O..D#",
                        "#........#", "#........#", "#...##...#", "#........#",
                        "#........#", "##########"});
}

// Compares two lists of log lines. If they differ, this prints the
// first differing line so the failure is easy to understand.
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

// Prepares the hand-made state used by both C1-6c runs below.
//
// The state is prepared by hand because, in normal runs, an action
// model V1 that cannot finish within the step budget is rare: the
// planning that leads to many recorded transitions normally costs
// more than replaying them. The hand-made history consists of real
// transitions of the standard rule, so every replay passes.
//
// Three East transitions are appended (the constructor has already
// recorded the first observation, at index 0), and unfinished V1
// work for East is set pending those three history indexes.
// Returns true when every state made here is a valid world.
bool prepareHandMadeHistory(SingleAgent& agent, const WorldState& world) {
    const WorldState a1 = applyAction(world, Action::East).state;
    const WorldState a2 = applyAction(a1, Action::East).state;
    const WorldState a3 = applyAction(a2, Action::East).state;
    agent.state.history.push_back(
        Observation{2, a1, Action::East, MoveOutcome::Moved});
    agent.state.history.push_back(
        Observation{3, a2, Action::East, MoveOutcome::Pushed});
    agent.state.history.push_back(
        Observation{4, a3, Action::East, MoveOutcome::Pushed});
    agent.state.pendingV1 = ModelV1Work{Action::East, {1, 2, 3}};
    return isValidWorld(a1) && isValidWorld(a2) && isValidWorld(a3);
}

void checkC1_6cWithLimit2(TestCounter& counter) {
    const WorldState world = demoWorld();
    const AgentSettings settings{2, 2, 100, 3};  // B = 2, S_max = 2
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, settings);
    const bool statesValid = prepareHandMadeHistory(agent, world);
    const RunResult result = runSingleAgent(world, agent, settings);
    const std::vector<std::string> expected{
        "Step 1: safe stop (investigation unfinished, no cautious action)",
        "Step 2: safe stop (investigation unfinished, no cautious action)",
        "Result: Stopped -- unresolved after 0 world steps, 2 safe stops",
        "Mismatches: 0", "Corrections: 0 (0 verified)",
        "Model evaluations used: 2"};
    const bool oneLeft = agent.state.pendingV1.has_value() &&
                         agent.state.pendingV1->pendingIndices ==
                             std::vector<int>{3};
    check(counter, isValidWorld(world) && statesValid &&
                       linesEqual(result.lines, expected, "C1-6c S_max=2") &&
                       oneLeft &&
                       !agent.state.agentModel.isRestored(Action::East),
          "C1-6c: with B = 2, unfinished V1 work gives repeated safe stops "
          "('investigation unfinished, no cautious action') that end the run "
          "Stopped -- unresolved at S_max = 2 (one replay per step, the "
          "reserve kept)");
}

void checkC1_6cWithLimit5(TestCounter& counter) {
    const WorldState world = demoWorld();
    const AgentSettings settings{2, 5, 100, 3};  // B = 2, S_max = 5
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt}, settings);
    const bool statesValid = prepareHandMadeHistory(agent, world);
    const RunResult result = runSingleAgent(world, agent, settings);
    const std::vector<std::string> expected{
        "Step 1: safe stop (investigation unfinished, no cautious action)",
        "Step 2: safe stop (investigation unfinished, no cautious action)",
        "  - correction 1: action model for East restored to the standard "
        "rule: CONSISTENT",
        "Step 3: safe stop (budget too small to plan)",
        "Step 4: safe stop (budget too small to plan)",
        "Step 5: safe stop (budget too small to plan)",
        "Result: Stopped -- unresolved after 0 world steps, 5 safe stops",
        "Mismatches: 0", "Corrections: 1 (0 verified)",
        "Model evaluations used: 3"};
    check(counter, isValidWorld(world) && statesValid &&
                       linesEqual(result.lines, expected, "C1-6c S_max=5") &&
                       !agent.state.pendingV1.has_value() &&
                       agent.state.agentModel.isRestored(Action::East),
          "C1-6c: with S_max = 5 the V1 work finishes at step 3 and creates "
          "the model correction; the reserve then rises to 2, so with B = 2 "
          "planning cannot be afforded and the run stops after 5 safe stops");
}

}  // namespace

void checkC1_6c(TestCounter& counter) {
    checkC1_6cWithLimit2(counter);
    checkC1_6cWithLimit5(counter);
}

}  // namespace ai_hologram
