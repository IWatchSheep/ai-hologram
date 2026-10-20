// Purpose: Checks FI3 and FI4: the F3 faulty model (faults combine,
// restoring one action, setBase keeps restored marks) and the F4
// "not dead" override on a true dead square (plan 3.4).
// Introduced in Stage 4.
// Architecture: Split off from test_fault_injector.cpp so each test
// file stays under about 200 lines. Uses tryApplyFault on a
// SingleAgent built on layout D; the true world is never touched.

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

int deadCount(const DeadSquareMap& map) {
    int count = 0;
    for (const auto& row : map.dead) {
        for (const bool dead : row) {
            if (dead) {
                ++count;
            }
        }
    }
    return count;
}

void checkFI3(TestCounter& counter) {
    const WorldState world = layoutD();
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    ReasonerParts parts = partsOf(agent.state);
    FaultedActions faults;
    FaultEvent east = handEvent("t", 1, FaultType::F3);
    east.action = Action::East;
    east.rotation = Rotation::Right;
    const ApplyOutcome first = tryApplyFault(east, parts, faults, {});
    WorldState input = world;
    input.agent = {4, 3};
    check(counter, isValidWorld(input), "FI3: hand-built input is valid");
    ActionModel direct = makeFaultyModel(faults);
    const ActionResult directResult = direct(input, Action::East);
    const ActionResult result = parts.model->current()(input, Action::East);
    bool eastOk = first.applied;
    eastOk = eastOk && result.outcome == MoveOutcome::Pushed;
    eastOk = eastOk && result.state.agent == Position{5, 3};
    eastOk = eastOk && result.state.object == Position{5, 4};
    eastOk = eastOk && directResult.state.object == Position{5, 4};
    eastOk = eastOk && applyAction(input, Action::East).state.object ==
                           Position{6, 3};
    check(counter, eastOk,
          "FI3: East faulted right predicts object (5, 4), standard (6, 3)");
    WorldState walled = input;
    walled.walls[4][5] = true;
    const ActionResult blocked = parts.model->current()(walled, Action::East);
    check(counter, blocked.outcome == MoveOutcome::Blocked &&
                       blocked.state == walled,
          "FI3: East faulted right with wall at (5, 4) is Blocked, unchanged");
    const ActionResult west = parts.model->current()(input, Action::West);
    check(counter, west.outcome == MoveOutcome::Moved &&
                       west.state.agent == Position{3, 3},
          "FI3: West from the same state gives the standard result");
    FaultEvent north = handEvent("t", 1, FaultType::F3);
    north.action = Action::North;
    north.rotation = Rotation::Left;
    const ApplyOutcome second = tryApplyFault(north, parts, faults, {});
    WorldState northInput = world;
    northInput.agent = {5, 4};
    const ActionResult northResult =
        parts.model->current()(northInput, Action::North);
    const ActionResult eastAgain = parts.model->current()(input, Action::East);
    bool bothOk = second.applied;
    bothOk = bothOk && eastAgain.state.object == Position{5, 4};
    bothOk = bothOk && northResult.outcome == MoveOutcome::Pushed;
    bothOk = bothOk && northResult.state.agent == Position{5, 3};
    bothOk = bothOk && northResult.state.object == Position{4, 3};
    check(counter, bothOk,
          "FI3: second F3 North left keeps East faulted, North object (4, 3)");
    parts.model->restore(Action::East);
    const ActionResult restored = parts.model->current()(input, Action::East);
    const ActionResult northStill =
        parts.model->current()(northInput, Action::North);
    bool restoreOk = restored.state.object == Position{6, 3};
    restoreOk = restoreOk && northStill.state.object == Position{4, 3};
    check(counter, restoreOk,
          "FI3: restoring East gives the standard rule, North still faulted");
    parts.model->setBase(makeFaultyModel(faults));
    const ActionResult afterSetBase =
        parts.model->current()(input, Action::East);
    bool setBaseOk = parts.model->isRestored(Action::East);
    setBaseOk = setBaseOk && afterSetBase.state.object == Position{6, 3};
    setBaseOk = setBaseOk &&
                parts.model->current()(northInput, Action::North)
                        .state.object == Position{4, 3};
    check(counter, setBaseOk,
          "FI3: setBase keeps East restored (H2), North still faulted");
}

void checkFI4(TestCounter& counter) {
    const WorldState world = layoutD();
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    ReasonerParts parts = partsOf(agent.state);
    const DeadSquareMap& before = agent.state.deadCache.get(
        agent.state.belief, agent.state.notDeadOverrides,
        agent.state.deadOverrides, agent.state.freeCounts);
    const int beforeCount = deadCount(before);
    FaultedActions faults;
    FaultEvent event = handEvent("t", 1, FaultType::F4);
    event.hasCell = true;
    event.cell = {1, 1};
    const ApplyOutcome outcome = tryApplyFault(event, parts, faults, {});
    const DeadSquareMap& after = agent.state.deadCache.get(
        agent.state.belief, agent.state.notDeadOverrides,
        agent.state.deadOverrides, agent.state.freeCounts);
    bool appliedOk = outcome.applied;
    appliedOk = appliedOk && !after.dead[1][1];
    appliedOk = appliedOk && beforeCount == 22;
    appliedOk = appliedOk && deadCount(after) == 21;
    check(counter, appliedOk,
          "FI4: F4 at (1, 1) applies, not dead, 21 dead cells (22 before)");
    agent.state.deadOverrides.push_back({1, 1});
    const DeadSquareMap& corrected = agent.state.deadCache.get(
        agent.state.belief, agent.state.notDeadOverrides,
        agent.state.deadOverrides, agent.state.freeCounts);
    check(counter, corrected.dead[1][1],
          "FI4: a later dead correction of (1, 1) makes it dead again");
    FaultEvent noCell = handEvent("t", 1, FaultType::F4);
    noCell.hasCell = false;
    const ApplyOutcome missing = tryApplyFault(noCell, parts, faults, {});
    check(counter, !missing.applied &&
                       missing.reason == "the layout has no true dead square",
          "FI4: F4 with no cell is not applied, no-dead-square reason");
}

}  // namespace

void runFaultInjectorF3Tests(TestCounter& counter) {
    checkFI3(counter);
    checkFI4(counter);
}

}  // namespace ai_hologram
