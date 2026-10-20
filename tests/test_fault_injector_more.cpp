// Purpose: Checks FI5 and FI6: F5 with A3's checks on a hand-made
// true history, and the prediction cache emptied by an applied fault.
// Introduced in Stage 4.
// Architecture: Split off from test_fault_injector.cpp so each test
// file stays under about 200 lines. FI5 reads the object's previous
// square from the TRUE history, but A3 uses the reasoner's own belief.

#include "experiment/fault_injector.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <vector>

namespace ai_hologram {
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

Observation observationOf(const WorldState& state, int step) {
    return Observation{step, state, std::nullopt, std::nullopt};
}

void checkFI5(TestCounter& counter) {
    const WorldState world = layoutD();
    check(counter, isValidWorld(world), "FI5: layout D is valid");
    WorldState o2state = world;
    o2state.agent = {4, 3};
    WorldState o3state = o2state;
    o3state.agent = {5, 3};
    o3state.object = {6, 3};
    WorldState o4state = o3state;
    o4state.agent = {5, 2};
    check(counter, isValidWorld(o2state) && isValidWorld(o3state) &&
                       isValidWorld(o4state),
          "FI5: hand-built history states are valid");
    const Observation o1 = observationOf(world, 1);
    const Observation o2 = observationOf(o2state, 2);
    const Observation o3 = observationOf(o3state, 3);
    const Observation o4 = observationOf(o4state, 4);
    const FaultEvent event = handEvent("t", 1, FaultType::F5);
    {
        SingleAgent agent(o1, kSettings);
        agent.state.belief = o3state;
        FaultedActions faults;
        const ApplyOutcome outcome = tryApplyFault(
            event, partsOf(agent.state), faults, {o1, o2, o3});
        check(counter, !outcome.applied &&
                           outcome.reason ==
                               "the previous square is the believed agent square",
              "FI5: F5 blocked by the believed agent square (A3)");
    }
    {
        SingleAgent agent(o1, kSettings);
        agent.state.belief = o4state;
        FaultedActions faults;
        const ApplyOutcome outcome = tryApplyFault(
            event, partsOf(agent.state), faults, {o1, o2, o3, o4});
        check(counter, outcome.applied &&
                           agent.state.belief.object == Position{5, 3},
              "FI5: F5 applies after the agent moved north, object (5, 3)");
    }
    {
        SingleAgent agent(o1, kSettings);
        FaultedActions faults;
        const ApplyOutcome outcome =
            tryApplyFault(event, partsOf(agent.state), faults, {o1});
        check(counter, !outcome.applied &&
                           outcome.reason == "the object has not moved yet",
              "FI5: F5 with no object move is not applied");
    }
    {
        SingleAgent agent(o1, kSettings);
        WorldState belief = o4state;
        belief.walls[3][5] = true;
        agent.state.belief = belief;
        FaultedActions faults;
        const ApplyOutcome outcome = tryApplyFault(
            event, partsOf(agent.state), faults, {o1, o2, o3, o4});
        check(counter, !outcome.applied &&
                           outcome.reason ==
                               "the previous square is believed to be a wall",
              "FI5: F5 blocked by a believed wall at the previous square");
    }
}

void checkFI6(TestCounter& counter) {
    const WorldState world = layoutD();
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    ReasonerParts parts = partsOf(agent.state);
    parts.predictor->predict(agent.state.belief, Action::East);
    const bool cachedBefore = parts.predictor->isCached(
        agent.state.belief, Action::East);
    FaultedActions faults;
    FaultEvent appliedEvent = handEvent("t", 1, FaultType::F1);
    appliedEvent.hasCell = true;
    appliedEvent.cell = {6, 5};
    const ApplyOutcome applied =
        tryApplyFault(appliedEvent, parts, faults, {});
    const bool cachedAfterApplied = parts.predictor->isCached(
        agent.state.belief, Action::East);
    check(counter, cachedBefore && applied.applied && !cachedAfterApplied,
          "FI6: an applied fault empties the prediction cache (D-036)");
    parts.predictor->predict(agent.state.belief, Action::East);
    const bool cachedAgain = parts.predictor->isCached(
        agent.state.belief, Action::East);
    FaultEvent refusedEvent = handEvent("t", 1, FaultType::F1);
    refusedEvent.hasCell = true;
    refusedEvent.cell = {3, 3};
    const ApplyOutcome refused =
        tryApplyFault(refusedEvent, parts, faults, {});
    const bool cachedAfterRefused = parts.predictor->isCached(
        agent.state.belief, Action::East);
    check(counter, cachedAgain && !refused.applied && cachedAfterRefused,
          "FI6: a fault that does not apply leaves the cache as it was");
}

}  // namespace

void runFaultInjectorMoreTests(TestCounter& counter) {
    checkFI5(counter);
    checkFI6(counter);
}

}  // namespace ai_hologram
