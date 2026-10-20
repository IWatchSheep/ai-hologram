// Purpose: Checks the mismatch investigation of plan 3.7 (IV1-IV9,
// the deferral rule, and the records and agent model used by it).
// Introduced in Stage 3.
// Architecture: Histories are Observations built with applyAction
// on the TRUE state, as the prompt requires. Every world used is
// checked valid with check(). VF1 (time-aware object V1 through
// the investigation) lives in test_verification.cpp.

#include "agent/agent_model.h"
#include "agent/investigation.h"
#include "agent/records.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
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

Observation firstObs(const WorldState& state) {
    return Observation{1, state, std::nullopt, std::nullopt};
}

// One history entry built from the TRUE state, as the prompt requires.
Observation nextObs(int step, const WorldState& trueBefore, Action action) {
    const ActionResult result = applyAction(trueBefore, action);
    return Observation{step, result.state, action, result.outcome};
}

std::vector<Observation> twoObs(const WorldState& trueBefore, Action action) {
    return {firstObs(trueBefore), nextObs(2, trueBefore, action)};
}

bool isWallCandidate(const BeliefCandidate& c, Position cell, bool blocked) {
    return c.element.kind == ElementKind::Wall && c.element.cell == cell &&
           c.blocked == blocked;
}

bool isObjectCandidate(const BeliefCandidate& c, Position position) {
    return c.element.kind == ElementKind::Object && c.position == position;
}

// A hand-made wrong model: when East would push, the object moves
// North instead of East (to (5, 2) from (5, 3) in the IV5 setup).
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

void checkIV1(TestCounter& counter) {
    const WorldState belief = demoWorld();
    const WorldState trueBefore = withWall(demoWorld(), {4, 3});
    check(counter, isValidWorld(belief) && isValidWorld(trueBefore),
          "IV1: belief D and true D+wall(4,3) are valid worlds");
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    const InvestigationResult result = investigateMismatch(
        belief, Action::East, twoObs(trueBefore, Action::East), predictor,
        ActionModel{&applyAction}, 1);
    check(counter, result.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       result.candidates.size() == 1 &&
                       isWallCandidate(result.candidates[0], {4, 3}, true) &&
                       result.suspected == SourceLabel::S2 && !result.anomaly &&
                       predictor.usedThisStep() == 1,
          "IV1: wall missing from belief gives [wall (4,3) blocked], "
          "consistent, S2, exactly 1 evaluation");
}

void checkIV2AndIV3(TestCounter& counter) {
    WorldState trueIV2 = demoWorld(); trueIV2.agent = {4, 3};
    WorldState beliefIV2 = withWall(trueIV2, {6, 3});
    check(counter, isValidWorld(trueIV2) && isValidWorld(beliefIV2),
          "IV2: true and belief worlds are valid");
    StepPredictor p2{WorldModel{}, 200}; p2.startStep();
    const InvestigationResult r2 = investigateMismatch(
        beliefIV2, Action::East, twoObs(trueIV2, Action::East), p2,
        ActionModel{&applyAction}, 1);
    check(counter, r2.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       r2.candidates.size() == 1 &&
                       isWallCandidate(r2.candidates[0], {6, 3}, false) &&
                       r2.suspected == SourceLabel::S2,
          "IV2: wall wrongly believed at c2 gives [wall (6,3) free], S2");
    WorldState trueIV3 = demoWorld(); trueIV3.agent = {4, 3}; trueIV3.object = {5, 4};
    WorldState beliefIV3 = demoWorld(); beliefIV3.agent = {4, 3};
    check(counter, isValidWorld(trueIV3) && isValidWorld(beliefIV3),
          "IV3: true and belief worlds are valid");
    StepPredictor p3{WorldModel{}, 200}; p3.startStep();
    const InvestigationResult r3 = investigateMismatch(
        beliefIV3, Action::East, twoObs(trueIV3, Action::East), p3,
        ActionModel{&applyAction}, 1);
    check(counter, r3.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       r3.candidates.size() == 1 &&
                       isObjectCandidate(r3.candidates[0], {5, 4}) &&
                       r3.suspected == SourceLabel::S2,
          "IV3/IV8: object believed at c1 but not there gives [object "
          "(5,4)] read from o_t-1 outside the footprint, S2");
}

void checkIV4AndIV6(TestCounter& counter) {
    WorldState trueIV4 = demoWorld(); trueIV4.agent = {4, 3};
    WorldState beliefIV4 = trueIV4; beliefIV4.object = {5, 4};
    check(counter, isValidWorld(trueIV4) && isValidWorld(beliefIV4),
          "IV4: true and belief worlds are valid");
    StepPredictor p4{WorldModel{}, 200}; p4.startStep();
    const InvestigationResult r4 = investigateMismatch(
        beliefIV4, Action::East, twoObs(trueIV4, Action::East), p4,
        ActionModel{&applyAction}, 1);
    check(counter, r4.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       r4.candidates.size() == 1 &&
                       isObjectCandidate(r4.candidates[0], {5, 3}) &&
                       r4.suspected == SourceLabel::S2,
          "IV4: object at c1 but not believed there gives [object (5,3)], S2");
    WorldState beliefIV6 = withWall(beliefIV4, {6, 3});
    check(counter, isValidWorld(beliefIV6), "IV6: belief world is valid");
    StepPredictor p6{WorldModel{}, 200}; p6.startStep();
    const InvestigationResult r6 = investigateMismatch(
        beliefIV6, Action::East, twoObs(trueIV4, Action::East), p6,
        ActionModel{&applyAction}, 1);
    check(counter, r6.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       r6.candidates.size() == 2 &&
                       isWallCandidate(r6.candidates[0], {6, 3}, false) &&
                       isObjectCandidate(r6.candidates[1], {5, 3}) &&
                       r6.suspected == SourceLabel::S2 && p6.usedThisStep() == 1,
          "IV6: [wall (6,3) free, object (5,3)] verified together with "
          "one shared re-prediction, consistent, S2");
}

void checkIV5AndIV7(TestCounter& counter) {
    WorldState both = demoWorld(); both.agent = {4, 3};
    check(counter, isValidWorld(both), "IV5: belief = true world is valid");
    StepPredictor p5{WorldModel{}, 200}; p5.startStep();
    const InvestigationResult r5 = investigateMismatch(
        both, Action::East, twoObs(both, Action::East), p5,
        eastPushNorthModel(), 1);
    check(counter, r5.outcome == InvestigationOutcome::ModelSuspected &&
                       r5.candidates.empty() && r5.suspected == SourceLabel::S1 &&
                       p5.usedThisStep() == 0,
          "IV5: belief matches the record, so model(East) is suspected, "
          "S1, no evaluation used");
    const WorldState belief = demoWorld();
    const WorldState trueBefore = withWall(demoWorld(), {4, 3});
    const ActionModel alwaysMoved = [](const WorldState& state,
                                       Action action) -> ActionResult {
        if (action != Action::East) return applyAction(state, action);
        WorldState next = state;
        next.agent = moved(state.agent, Action::East);
        return {next, MoveOutcome::Moved};  // East "moves" even into a wall
    };
    StepPredictor p7{WorldModel{}, 200}; p7.startStep();
    const InvestigationResult r7 = investigateMismatch(
        belief, Action::East, twoObs(trueBefore, Action::East), p7,
        alwaysMoved, 1);
    check(counter, r7.outcome == InvestigationOutcome::NoCandidatePassed &&
                       r7.suspected == SourceLabel::S5,
          "IV7: a current model that always predicts Moved passes no "
          "candidate, so NoCandidatePassed, S5");
}

}  // namespace

void checkIV9DeferralRecords(TestCounter& counter);

void runInvestigationTests(TestCounter& counter) {
    std::cout << "--- Investigation ---\n";
    checkIV1(counter);
    checkIV2AndIV3(counter);
    checkIV4AndIV6(counter);
    checkIV5AndIV7(counter);
    checkIV9DeferralRecords(counter);
}

}  // namespace ai_hologram
