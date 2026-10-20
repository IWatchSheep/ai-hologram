// Purpose: Checks IV9 (anomaly), the investigation deferral rule,
// the plain records and the agent's action model (plan 3.7, 3.8).
// Introduced in Stage 3.
// Architecture: Split from test_investigation.cpp so each file
// stays under about 200 lines. Every world is checked valid.

#include "agent/agent_model.h"
#include "agent/investigation.h"
#include "agent/records.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <vector>

namespace ai_hologram {
namespace {

WorldState demoWorld() {  // layout D
    return parseWorld({"##########", "#........#", "#........#", "#..A.O..D#",
                       "#........#", "#........#", "#...##...#", "#........#",
                       "#........#", "##########"})
        .world.value_or(WorldState{});
}

WorldState withWall(WorldState state, Position cell) {
    state.walls[static_cast<std::size_t>(cell.y)]
               [static_cast<std::size_t>(cell.x)] = true;
    return state;
}

std::vector<Observation> twoObs(const WorldState& trueBefore, Action action) {
    const ActionResult result = applyAction(trueBefore, action);
    return {Observation{1, trueBefore, std::nullopt, std::nullopt},
            Observation{2, result.state, action, result.outcome}};
}

// A hand-made wrong model: when East would push, the object moves
// North instead of East (to (5, 2) from (5, 3) in this setup).
ActionModel eastPushNorthModel() {
    return [](const WorldState& state, Action action) -> ActionResult {
        const ActionResult standard = applyAction(state, action);
        if (action != Action::East || standard.outcome != MoveOutcome::Pushed) {
            return standard;
        }
        WorldState wrong = standard.state;
        wrong.object = moved(state.object, Action::North);
        return {wrong, MoveOutcome::Pushed};
    };
}

}  // namespace

void checkIV9DeferralRecords(TestCounter& counter) {
    WorldState trueIV9 = demoWorld(); trueIV9.agent = {3, 4};
    check(counter, isValidWorld(trueIV9), "IV9: true world is valid");
    StepPredictor p9{WorldModel{}, 200}; p9.startStep();
    const InvestigationResult r9 = investigateMismatch(
        demoWorld(), Action::North, twoObs(trueIV9, Action::North), p9,
        ActionModel{&applyAction}, 1);
    check(counter, r9.anomaly, "IV9: believed agent differs from o_t-1, "
                                "so the anomaly flag is set");
    const WorldState trueBefore = withWall(demoWorld(), {4, 3});
    StepPredictor tight{WorldModel{}, 1}; tight.startStep();
    const InvestigationResult deferred = investigateMismatch(
        demoWorld(), Action::East, twoObs(trueBefore, Action::East), tight,
        ActionModel{&applyAction}, 1);
    check(counter, deferred.outcome == InvestigationOutcome::Deferred &&
                       tight.usedThisStep() == 0,
          "Deferral: remaining() == reserve gives Deferred, 0 evaluations");
    CorrectionRecord record;
    record.id = 1; record.element.kind = ElementKind::Wall;
    record.element.cell = {4, 3}; record.newBlocked = true;
    record.status = CorrectionStatus::Consistent; record.stepConsistent = 2;
    MismatchRecord mismatch;
    mismatch.decisionStep = 2; mismatch.action = Action::East;
    mismatch.suspected = SourceLabel::S2;
    check(counter, record.id == 1 && record.element.cell == Position{4, 3} &&
                       !record.tested && mismatch.suspected == SourceLabel::S2 &&
                       SourceLabel::S3 != SourceLabel::S4,
          "Records: correction and mismatch records hold their fields "
          "(S3/S4 exist but C1 never uses them)");
    AgentActionModel agentModel(eastPushNorthModel());
    WorldState state = demoWorld(); state.agent = {4, 3};
    check(counter, !agentModel.isRestored(Action::East) &&
                       agentModel.current()(state, Action::East).state.object ==
                           Position{5, 2},
          "Agent model: an unrestored action uses the (wrong) base model");
    agentModel.restore(Action::East);
    check(counter, agentModel.isRestored(Action::East) &&
                       agentModel.current()(state, Action::East).state.object ==
                           Position{6, 3} &&
                       agentModel.base()(state, Action::East).state.object ==
                           Position{5, 2},
          "Agent model: restore(East) uses the standard rule; base() "
          "still returns the old model");
    agentModel.unrestore(Action::East);
    check(counter, !agentModel.isRestored(Action::East) &&
                       AgentActionModel{}.current()(state, Action::East)
                               .state.object == Position{6, 3},
          "Agent model: unrestore removes the mark; the default base "
          "model is the standard rule");
}

}  // namespace ai_hologram
