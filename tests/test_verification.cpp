// Purpose: Checks the V1 verification rules of plan 3.8: the
// time-aware object check (VF1, VF2), the wall check over every
// observation (VF3) and the model(a) replays (VF4).
// Introduced in Stage 3.
// Architecture: VF5, VF7, VF8 and the active-test targets live in
// test_verification_cases.cpp, and the trap tests VF9-VF12 in
// test_verification_trap.cpp, so each file stays under 200 lines.
// Every world used is checked valid with check().

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

// The VF1 history: D, then East, East, East on the true state.
std::vector<Observation> eastHistory() {
    std::vector<Observation> history;
    history.push_back(Observation{1, demoWorld(), std::nullopt, std::nullopt});
    for (int step = 2; step <= 4; ++step) {
        history.push_back(nextObs(step, history.back().state, Action::East));
    }
    return history;
}

void checkVF1AndVF2(TestCounter& counter) {
    const std::vector<Observation> history = eastHistory();
    const Observation& o2 = history[2];  // agent (5,3), object (6,3)
    check(counter, isValidWorld(o2.state) &&
                       objectHistoryCheck({6, 3}, o2) &&
                       !objectHistoryCheck({5, 3}, o2),
          "VF1/VF2: objectHistoryCheck compares only with o_t-1, so "
          "(6,3) passes and (5,3) fails against o2");
    WorldState beliefBefore = o2.state;
    beliefBefore.object = {6, 4};  // the hand-written wrong belief
    check(counter, isValidWorld(beliefBefore),
          "VF1: beliefBefore (object wrongly at (6,4)) is valid");
    // Full history is passed on purpose, so o0 and o1 (object at (5, 3)) are present.
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    const InvestigationResult result = investigateMismatch(
        beliefBefore, Action::East, history, predictor,
        ActionModel{&applyAction}, 1);
    check(counter, result.outcome == InvestigationOutcome::BeliefCandidatesConsistent &&
                       result.candidates.size() == 1 &&
                       result.candidates[0].element.kind == ElementKind::Object &&
                       result.candidates[0].position == Position{6, 3},
          "VF1: candidate [object (6,3)] is consistent even though o0 "
          "and o1 show the object at (5,3) (time-aware V1)");
}

void checkVF3(TestCounter& counter) {
    WorldState trueBefore = demoWorld();
    trueBefore.walls[3][4] = true;  // a wall at (4, 3) in every entry
    check(counter, isValidWorld(trueBefore), "VF3: true world is valid");
    const std::vector<Observation> history{
        Observation{1, trueBefore, std::nullopt, std::nullopt},
        nextObs(2, trueBefore, Action::East)};
    check(counter, wallHistoryCheck({4, 3}, true, history),
          "VF3: wallHistoryCheck((4,3), blocked) is true when every "
          "recorded observation has the wall");
    std::vector<Observation> lacking = history;
    lacking[0].state.walls[3][4] = false;  // even the FIRST entry counts
    check(counter, isValidWorld(lacking[0].state) &&
                       !wallHistoryCheck({4, 3}, true, lacking),
          "VF3: it is false if one recorded observation (even the "
          "first) lacks the wall");
}

void checkVF4(TestCounter& counter) {
    const std::vector<Observation> history = eastHistory();
    ModelV1Work work = startModelV1(Action::East, history);
    check(counter, work.pendingIndices == std::vector<int>({1, 2, 3}),
          "VF4: startModelV1(East) gives indices 1, 2, 3 in history order");
    int currentCalls = 0;
    const ActionModel counting = [&currentCalls](const WorldState& state,
                                                 Action action) {
        ++currentCalls;
        return applyAction(state, action);
    };
    StepPredictor predictor{WorldModel{counting}, 200};
    predictor.startStep();
    const V1Progress progress = continueModelV1(work, history, predictor, 1);
    check(counter, progress == V1Progress::Done && predictor.usedThisStep() == 3 &&
                       currentCalls == 0,
          "VF4/VF8: replays are Done with exactly 3 variant evaluations; "
          "the predictor's current model is never called (standard rule)");
    std::vector<Observation> altered = history;
    altered[2].state.object = {6, 4};  // hand-altered, still valid
    check(counter, isValidWorld(altered[2].state),
          "VF4: the hand-altered recorded state is still valid");
    ModelV1Work failing = startModelV1(Action::East, altered);
    StepPredictor second{WorldModel{}, 200};
    second.startStep();
    check(counter, continueModelV1(failing, altered, second, 1) ==
                       V1Progress::Failed,
          "VF4: a replay that differs from the recorded state is Failed");
}

}  // namespace

void checkVF5AndVF7(TestCounter& counter);
void checkVF8AndActive(TestCounter& counter);
void checkTrapTests(TestCounter& counter);

void runVerificationTests(TestCounter& counter) {
    std::cout << "--- Verification ---\n";
    checkVF1AndVF2(counter);
    checkVF3(counter);
    checkVF4(counter);
    checkVF5AndVF7(counter);
    checkVF8AndActive(counter);
    checkTrapTests(counter);
}

}  // namespace ai_hologram
