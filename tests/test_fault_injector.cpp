// Purpose: Checks FI1 and FI2: applying F1 and F2 faults to a
// reasoner by hand, with the exact reasons of plan 3.4.
// Introduced in Stage 4.
// Architecture: These checks use tryApplyFault on partsOf(agent.state)
// of a SingleAgent built on layout D. The true world is never touched:
// only the agent's belief is changed. FI3-FI4 are in
// test_fault_injector_f3.cpp, FI5-FI6 in test_fault_injector_more.cpp
// and FI7-FI8 in test_fault_injector_runs.cpp, split for file size.

#include "experiment/fault_injector.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {

void runFaultInjectorF3Tests(TestCounter& counter);
void runFaultInjectorMoreTests(TestCounter& counter);
void runFaultInjectorRunTests(TestCounter& counter);

namespace {

const AgentSettings kSettings{200, 5, 100, 3};

WorldState layoutD() {
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A.O..D#",
         "#........#", "#........#", "#...##...#", "#........#",
         "#........#", "##########"});
    return *parsed.world;
}

FaultEvent handEvent(const std::string& id, int step, FaultType type) {
    FaultEvent event;
    event.id = id;
    event.step = step;
    event.type = type;
    return event;
}

bool wallAt(const WorldState& state, Position cell) {
    return state.walls[static_cast<std::size_t>(cell.y)]
                      [static_cast<std::size_t>(cell.x)];
}

void checkFI1(TestCounter& counter) {
    const WorldState world = layoutD();
    check(counter, isValidWorld(world), "FI1: layout D is valid");
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F1);
        event.hasCell = true;
        event.cell = {3, 3};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, !outcome.applied &&
                           outcome.reason ==
                               "the wall would be under the believed agent",
              "FI1: F1 free->blocked at (3, 3) is not applied, agent reason");
    }
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F1);
        event.hasCell = true;
        event.cell = {5, 3};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, !outcome.applied &&
                           outcome.reason ==
                               "the wall would be under the believed object",
              "FI1: F1 free->blocked at (5, 3) is not applied, object reason");
    }
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F1);
        event.hasCell = true;
        event.cell = {6, 5};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, outcome.applied && wallAt(agent.state.belief, {6, 5}),
              "FI1: F1 free->blocked at (6, 5) applies, belief has the wall");
    }
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F1);
        event.direction = F1Direction::BlockedToFree;
        event.hasCell = true;
        event.cell = {4, 6};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, outcome.applied && !wallAt(agent.state.belief, {4, 6}),
              "FI1: F1 blocked->free at (4, 6) applies, cell now free");
    }
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F1);
        event.hasCell = false;
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, !outcome.applied &&
                           outcome.reason == "no cell of the required kind",
              "FI1: F1 with no cell is not applied, no-cell reason");
    }
}

void checkFI2(TestCounter& counter) {
    const WorldState world = layoutD();
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F2);
        event.order = {Action::North, Action::East, Action::South,
                       Action::West};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, outcome.applied &&
                           agent.state.belief.object == Position{5, 2},
              "FI2: F2 order N,E,S,W moves believed object to (5, 2)");
    }
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        agent.state.belief.object = {1, 1};
        agent.state.belief.walls[1][2] = true;
        agent.state.belief.walls[2][1] = true;
        check(counter, isValidWorld(agent.state.belief),
              "FI2: hand-built belief with object (1, 1) is valid");
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F2);
        event.order = {Action::North, Action::East, Action::South,
                       Action::West};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, !outcome.applied &&
                           outcome.reason == "no direction passes the test",
              "FI2: F2 with every direction blocked is not applied");
    }
    {
        SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                          kSettings);
        agent.state.belief.agent = {5, 2};
        FaultedActions faults;
        FaultEvent event = handEvent("t", 1, FaultType::F2);
        event.order = {Action::North, Action::East, Action::South,
                       Action::West};
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {});
        check(counter, outcome.applied &&
                           agent.state.belief.object == Position{6, 3},
              "FI2: F2 skips the believed agent cell, object to (6, 3)");
    }
}

}  // namespace

void runFaultInjectorTests(TestCounter& counter) {
    std::cout << "--- Fault injector ---\n";
    checkFI1(counter);
    checkFI2(counter);
    runFaultInjectorF3Tests(counter);
    runFaultInjectorMoreTests(counter);
    runFaultInjectorRunTests(counter);
}

}  // namespace ai_hologram
