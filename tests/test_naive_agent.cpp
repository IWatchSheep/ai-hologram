// Purpose: Checks C0, the naive agent (plan 6.9): C0-1 to C0-5.
// Introduced in Stage 3.
// Architecture: C0 is run both through runSingleAgent and in hand-run
// loops (decide, Environment::step, next observation), so the tests can
// look at usedThisStep(), searchCount() and the exact actions after
// every decision. Every world is checked valid with check().

#include "agent/naive_agent.h"
#include "agent/run_loop.h"
#include "test_suites.h"
#include "world/environment.h"
#include "world/world_text.h"

#include <iostream>
#include <optional>
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

WorldState demoWorldWalled() {  // layout D2: D plus a wall at (4, 3)
    return parseLayout({"##########", "#........#", "#........#", "#..A#O..D#",
                        "#........#", "#........#", "#...##...#", "#........#",
                        "#........#", "##########"});
}

AgentSettings settings() { return AgentSettings{200, 5, 100, 3}; }

Observation firstOf(const WorldState& world) {
    return Observation{1, world, std::nullopt, std::nullopt};
}

bool linesEqual(const std::vector<std::string>& actual,
                const std::vector<std::string>& expected, const std::string& label) {
    if (actual == expected) return true;
    std::cout << "First differing line in " << label << ":\n";
    const std::size_t count = std::max(actual.size(), expected.size());
    for (std::size_t i = 0; i < count; ++i) {
        const std::string a = i < actual.size() ? actual[i] : "<missing>";
        const std::string e = i < expected.size() ? expected[i] : "<missing>";
        if (a != e) {
            std::cout << "  line " << i << ": got \"" << a << "\", expected \"" << e << "\"\n";
            break;
        }
    }
    return false;
}

// One hand-run decision: ask the agent, step the true world, and build
// the next observation from the result (as the run loop does).
Observation handStep(NaiveAgent& agent, Environment& environment,
                     const Observation& current, Action& taken) {
    const Decision decision = agent.decide(current);
    taken = decision.action;
    const ActionResult stepped = environment.step(decision.action);
    return Observation{agent.state.decisionStep + 1, environment.state(),
                       decision.action, stepped.outcome};
}

void checkC0_4AndC0_2(TestCounter& counter) {
    const WorldState world = demoWorld();
    check(counter, isValidWorld(world), "C0-4: layout D is valid");
    NaiveAgent runAgent(firstOf(world), settings());
    const RunResult result = runSingleAgent(world, runAgent, settings());
    const std::vector<std::string> expected{
        "Result: Success after 4 world steps, 0 safe stops", "Mismatches: 0",
        "Corrections: 0 (0 verified)", "Model evaluations used: 16"};
    check(counter, linesEqual(result.lines, expected, "C0-4") &&
                       result.outcome == "Success" && result.worldSteps == 4,
          "C0-4: C0 reaches the goal in 4 world steps; lines match exactly (16 evaluations)");

    // The same case in a hand-run loop, to watch each decision.
    NaiveAgent agent(firstOf(world), settings());
    Environment environment(world);
    Observation current = firstOf(world);
    std::vector<Action> actions;
    std::vector<int> usedPerStep;
    for (int step = 0; step < 4; ++step) {
        Action taken = Action::North;
        current = handStep(agent, environment, current, taken);
        actions.push_back(taken);
        usedPerStep.push_back(agent.state.predictor.usedThisStep());
    }
    const std::vector<Action> eastFour{Action::East, Action::East, Action::East, Action::East};
    check(counter, actions == eastFour && usedPerStep == std::vector<int>{16, 0, 0, 0} &&
                       agent.searchCount() == 1,
          "C0-2/C0-4: actions are East x4; usedThisStep() after each decide is 16, 0, 0, 0 and searchCount() is 1");
}

void checkC0_5(TestCounter& counter) {
    const WorldState world = demoWorldWalled();  // D2
    const WorldState belief = demoWorld();       // D, no wall at (4, 3)
    check(counter, isValidWorld(world) && isValidWorld(belief),
          "C0-5: true world D2 and hand-written belief D are valid");
    NaiveAgent runAgent(firstOf(world), settings(), belief);
    const RunResult result = runSingleAgent(world, runAgent, settings());
    const std::vector<std::string> expected{
        "Result: Step limit after 100 world steps, 0 safe stops", "Mismatches: 0",
        "Corrections: 0 (0 verified)", "Model evaluations used: 207"};
    check(counter, linesEqual(result.lines, expected, "C0-5") &&
                       result.outcome == "Step limit" && runAgent.searchCount() == 97,
          "C0-5: C0 makes no checks, records or safe stops, so a wrong belief is never noticed (Phase B 11.8); lines match exactly (207 evaluations) and searchCount() is 97");

    // The first ten actions, from a hand-run loop with a second identical agent.
    NaiveAgent agent(firstOf(world), settings(), belief);
    Environment environment(world);
    Observation current = firstOf(world);
    std::vector<Action> actions;
    for (int step = 0; step < 10; ++step) {
        Action taken = Action::North;
        current = handStep(agent, environment, current, taken);
        actions.push_back(taken);
    }
    const std::vector<Action> firstTen{Action::East, Action::East, Action::East, Action::East,
                                       Action::North, Action::North, Action::East,
                                       Action::South, Action::North, Action::South};
    check(counter, actions == firstTen,
          "C0-5: the first ten actions are East, East, East, East, North, North, East, South, North, South");
}

void checkC0_1(TestCounter& counter) {
    const WorldState world = demoWorldWalled();  // D2
    const WorldState belief = demoWorld();       // D
    check(counter, isValidWorld(world) && isValidWorld(belief),
          "C0-1: true world D2 and hand-written belief D are valid");
    const Observation first = firstOf(world);
    NaiveAgent agentA(first, settings(), belief);
    NaiveAgent agentB(first, settings(), belief);
    Environment environment(world);
    Observation current = first;
    std::vector<Action> actionsA;
    std::vector<Action> actionsB;
    for (int step = 0; step < 20; ++step) {
        // Agent A gets the true observations; B gets the FIRST one again.
        Action takenA = Action::North;
        current = handStep(agentA, environment, current, takenA);
        actionsA.push_back(takenA);
        actionsB.push_back(agentB.decide(first).action);
    }
    check(counter, actionsA == actionsB &&
                       agentA.totalEvaluations() == agentB.totalEvaluations(),
          "C0-1: over 20 decisions, both agents choose identical actions and have identical totalEvaluations(); different later observations change nothing");
}

void checkC0_3(TestCounter& counter) {
    const WorldState corner = parseLayout(  // layout LTa
        {"##########", "#O...A...#", "#........#", "#........#", "#........#",
         "#........#", "#........#", "#.......D#", "#........#", "##########"});
    check(counter, isValidWorld(corner) && corner.object == Position{1, 1} &&
                       corner.agent == Position{5, 1},
          "C0-3a: layout LTa is valid, object on the dead corner (1, 1), agent at (5, 1), so North is Blocked");
    NaiveAgent agentA(firstOf(corner), settings());
    const Decision firstA = agentA.decide(firstOf(corner));
    check(counter, firstA.isAction && firstA.action == Action::East &&
                       agentA.state.predictor.usedThisStep() == 4 &&
                       agentA.searchCount() == 1 && agentA.state.belief.agent == Position{6, 1},
          "C0-3a: the first decide gives East, with usedThisStep() == 4 and searchCount() == 1; C0's belief then has its agent at (6, 1)");

    const WorldState boxed = parseLayout(  // layout BOXED
        {"##########", "#.#......#", "##A#.....#", "#.#..O..D#", "#........#",
         "#........#", "#...##...#", "#........#", "#........#", "##########"});
    check(counter, isValidWorld(boxed) && boxed.agent == Position{2, 2},
          "C0-3b: layout BOXED is valid, with walls on all four sides of the agent at (2, 2)");
    NaiveAgent agentB(firstOf(boxed), settings());
    const Decision firstB = agentB.decide(firstOf(boxed));
    check(counter, firstB.isAction && firstB.action == Action::North &&
                       agentB.state.predictor.usedThisStep() == 4 && agentB.state.belief == boxed,
          "C0-3b: the first decide gives North, with usedThisStep() == 4, and C0's belief is unchanged");
}

}  // namespace

void runNaiveAgentTests(TestCounter& counter) {
    std::cout << "--- Naive agent (C0) ---\n";
    checkC0_4AndC0_2(counter);
    checkC0_5(counter);
    checkC0_1(counter);
    checkC0_3(counter);
}

}  // namespace ai_hologram
