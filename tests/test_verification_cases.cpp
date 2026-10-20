// Purpose: Checks model V1 across steps (VF5 part), when V2
// alternatives are computed (VF7), which model each verification
// prediction uses (VF8, except the trap part), predictionsDiffer
// and the active-test targets (plan 3.8).
// Introduced in Stage 3.
// Architecture: Split from test_verification.cpp so each file stays
// under about 200 lines. The trap part of VF8 is in
// test_verification_trap.cpp. Every world is checked valid.

#include "agent/investigation.h"
#include "agent/verification.h"
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

Observation nextObs(int step, const WorldState& trueBefore, Action action) {
    const ActionResult result = applyAction(trueBefore, action);
    return Observation{step, result.state, action, result.outcome};
}

std::vector<Observation> eastHistory() {
    std::vector<Observation> history;
    history.push_back(Observation{1, demoWorld(), std::nullopt, std::nullopt});
    for (int step = 2; step <= 4; ++step) {
        history.push_back(nextObs(step, history.back().state, Action::East));
    }
    return history;
}

CorrectionRecord wallRecord() {
    CorrectionRecord record;
    record.id = 1;
    record.element.kind = ElementKind::Wall;
    record.element.cell = {4, 3};
    record.newBlocked = true;   // corrected to blocked...
    record.oldBlocked = false;  // ...from believed free
    record.status = CorrectionStatus::Consistent;
    record.stepConsistent = 1;
    return record;
}

CorrectionRecord modelRecord() {
    CorrectionRecord record;
    record.id = 2;
    record.element.kind = ElementKind::ActionModel;
    record.element.action = Action::East;
    record.status = CorrectionStatus::Consistent;
    record.stepConsistent = 1;
    return record;
}

}  // namespace

void checkVF5AndVF7(TestCounter& counter) {
    const std::vector<Observation> history = eastHistory();
    ModelV1Work work = startModelV1(Action::East, history);
    StepPredictor predictor{WorldModel{}, 3};  // B = 3, reserve 1
    predictor.startStep();
    const V1Progress first = continueModelV1(work, history, predictor, 1);
    check(counter, first == V1Progress::Pending && predictor.usedThisStep() == 2 &&
                       work.pendingIndices == std::vector<int>({3}),
          "VF5 (part): B=3, reserve 1 makes 2 replays, then Pending "
          "with index 3 left (the decision-step part belongs to 3e)");
    predictor.startStep();  // a fresh step budget, as plan 3.8 describes
    check(counter, continueModelV1(work, history, predictor, 1) ==
                           V1Progress::Done &&
                       predictor.usedThisStep() == 1,
          "VF5 (part): after startStep() the last replay runs and the "
          "work is Done, using 1 more evaluation");
    const WorldState belief = demoWorld();  // agent (3, 3)
    check(counter, isValidWorld(belief), "VF7: belief D is a valid world");
    StepPredictor east{WorldModel{}, 200}; east.startStep();
    const auto wallEast = alternativePrediction(wallRecord(), belief,
        Action::East, east, ActionModel{&applyAction}, ActionModel{&applyAction});
    StepPredictor north{WorldModel{}, 200}; north.startStep();
    const auto wallNorth = alternativePrediction(wallRecord(), belief,
        Action::North, north, ActionModel{&applyAction}, ActionModel{&applyAction});
    check(counter, wallEast.has_value() && east.usedThisStep() == 1 &&
                       !wallNorth.has_value() && north.usedThisStep() == 0,
          "VF7: wall correction at (4,3) computes an alternative for "
          "East (1 evaluation) but not for North (0 evaluations)");
    StepPredictor modelEast{WorldModel{}, 200}; modelEast.startStep();
    const auto modelAlt = alternativePrediction(modelRecord(), belief,
        Action::East, modelEast, ActionModel{&applyAction}, ActionModel{&applyAction});
    StepPredictor modelWest{WorldModel{}, 200}; modelWest.startStep();
    const auto modelWestAlt = alternativePrediction(modelRecord(), belief,
        Action::West, modelWest, ActionModel{&applyAction}, ActionModel{&applyAction});
    check(counter, modelAlt.has_value() && modelEast.usedThisStep() == 1 &&
                       !modelWestAlt.has_value() && modelWest.usedThisStep() == 0,
          "VF7: a model(East) correction computes an alternative for "
          "East but not for West");
}

void checkVF8AndActive(TestCounter& counter) {
    WorldState trueBefore = demoWorld();
    trueBefore.walls[3][4] = true;
    const std::vector<Observation> history{
        Observation{1, trueBefore, std::nullopt, std::nullopt},
        nextObs(2, trueBefore, Action::East)};
    int currentCalls = 0;
    const ActionModel countingCurrent = [&currentCalls](
        const WorldState& state, Action action) {
        ++currentCalls;
        return applyAction(state, action);
    };
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    const InvestigationResult result = investigateMismatch(
        demoWorld(), Action::East, history, predictor, countingCurrent, 1);
    check(counter, result.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       currentCalls == 1 && predictor.usedThisStep() == 1,
          "VF8: investigateMismatch's re-prediction calls the CURRENT "
          "model exactly once");
    int oldCalls = 0;
    int otherCurrentCalls = 0;
    const ActionModel countingOld = [&oldCalls](const WorldState& state,
                                                Action action) {
        ++oldCalls;
        return applyAction(state, action);
    };
    const ActionModel countingOther = [&otherCurrentCalls](
        const WorldState& state, Action action) {
        ++otherCurrentCalls;
        return applyAction(state, action);
    };
    StepPredictor altPredictor{WorldModel{}, 200};
    altPredictor.startStep();
    const auto alternative = alternativePrediction(
        modelRecord(), demoWorld(), Action::East, altPredictor,
        countingOther, countingOld);
    check(counter, alternative.has_value() && oldCalls == 1 &&
                       otherCurrentCalls == 0 && altPredictor.usedThisStep() == 1,
          "VF8: a model correction's alternative calls the OLD model "
          "once and never the current one");
    const Prediction base{Action::East, MoveOutcome::Moved, demoWorld()};
    Prediction changed = base;
    changed.outcome = MoveOutcome::Blocked;
    Prediction movedAgent = base;
    movedAgent.state.agent = {4, 3};
    check(counter, !predictionsDiffer(base, base) &&
                       predictionsDiffer(base, changed) &&
                       predictionsDiffer(base, movedAgent),
          "VF8/predictionsDiffer: outcome or agent/object differences "
          "count, identical predictions do not");
    WorldState belief = demoWorld();
    belief.walls[3][4] = true;  // D plus a wall at (4, 3)
    check(counter, isValidWorld(belief), "Active targets: belief is valid");
    const std::vector<std::pair<Position, Action>> expected{
        {{4, 4}, Action::North}, {{3, 3}, Action::East}, {{4, 2}, Action::South}};
    check(counter, activeTestTargets(belief, {4, 3}) == expected,
          "Active targets: (4,4)/North, (3,3)/East, (4,2)/South for "
          "cell (4,3); (5,3), the object cell, is excluded");
}

}  // namespace ai_hologram
