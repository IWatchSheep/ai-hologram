// Purpose: Checks SC1, SC2 and SC3: the injected-F1 run is scored
// exactly, a hand-written wrong belief is a false alarm plus a
// mismatch without fault cause, and claimValueIsTrue judges each
// kind of claim (plan 6.6).
// Introduced in Stage 4.
// Architecture: These tests use runWithFaults and the scorer's own
// public function. Every hand-built world is checked valid.

#include "experiment/fault_run.h"
#include "experiment/scorer.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>

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

WorldState layoutD2() {
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A#O..D#",
         "#........#", "#........#", "#...##...#", "#........#",
         "#........#", "##########"});
    return *parsed.world;
}

FaultEvent demoEvent() {
    FaultEvent event;
    event.id = "demo-1";
    event.step = 1;
    event.type = FaultType::F1;
    event.direction = F1Direction::BlockedToFree;
    event.hasCell = true;
    event.cell = {4, 3};
    return event;
}

void checkSC1(TestCounter& counter) {
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(walled), "SC1: layout D2 is valid");
    SingleAgent agent(Observation{1, walled, std::nullopt, std::nullopt},
                      kSettings);
    const FaultRunResult result =
        runWithFaults(walled, agent, kSettings, {demoEvent()});
    bool ok = result.eventScores.size() == 1;
    if (ok) {
        const EventScore& score = result.eventScores[0];
        ok = ok && score.activated;
        ok = ok && score.activationStep == 1;
        ok = ok && score.revealing;
        ok = ok && score.opportunityStep.has_value();
        ok = ok && score.opportunityStep.value() == 1;
        ok = ok && score.mismatchStep.has_value();
        ok = ok && score.mismatchStep.value() == 2;
        ok = ok && score.strictStep.has_value();
        ok = ok && score.strictStep.value() == 2;
        ok = ok && score.endState == EndState::Corrected;
        ok = ok && score.endStep == 2;
        ok = ok && score.correctingId == 1;
        ok = ok && score.correctionStatus.has_value();
        ok = ok && score.correctionStatus.value() == CorrectionStatus::Verified;
    }
    check(counter, ok, "SC1: demo-1 event scored exactly");
    const RunScore& run = result.score;
    bool okRun = run.claims == 1;
    okRun = okRun && run.falseAlarms == 0;
    okRun = okRun && run.wrongAccepted == 0;
    okRun = okRun && run.mismatches == 1;
    okRun = okRun && run.mismatchesWithoutCause == 0;
    okRun = okRun && run.labelsCorrect == 1;
    okRun = okRun && run.labelsScored == 1;
    okRun = okRun && run.justifiedSafeStops == 0;
    check(counter, okRun, "SC1: run scores exactly");
}

void checkSC2(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(world) && isValidWorld(walled),
          "SC2: layouts D and D2 are valid");
    // The wrong belief is hand-written, not injected: the scorer
    // only knows about injected faults, so this is a false alarm
    // plus a mismatch without fault cause, reported as a possible
    // bug (plan 6.6 SC2).
    SingleAgent agent(Observation{1, walled, std::nullopt, std::nullopt},
                      kSettings, world);
    const FaultRunResult result = runWithFaults(walled, agent, kSettings, {});
    const RunScore& run = result.score;
    bool ok = run.claims == 1;
    ok = ok && run.falseAlarms == 1;
    ok = ok && run.wrongAccepted == 0;
    ok = ok && run.mismatches == 1;
    ok = ok && run.mismatchesWithoutCause == 1;
    ok = ok && run.labelsScored == 0;
    ok = ok && run.labelsCorrect == 0;
    check(counter, ok, "SC2: hand-written error scored as false alarm");
}

CorrectionRecord wallRecord(const bool blocked) {
    CorrectionRecord record;
    record.id = 1;
    record.element.kind = ElementKind::Wall;
    record.element.cell = {4, 3};
    record.newBlocked = blocked;
    return record;
}

void checkSC3(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(world) && isValidWorld(walled),
          "SC3: layouts D and D2 are valid");
    const std::vector<Observation> noHistory;
    const std::vector<MismatchRecord> noMismatches;
    bool ok = claimValueIsTrue(wallRecord(true), walled, noHistory,
                               noMismatches);
    ok = ok && !claimValueIsTrue(wallRecord(true), world, noHistory,
                                 noMismatches);
    check(counter, ok, "SC3: wall claim true for D2, false for D");
    CorrectionRecord model;
    model.element.kind = ElementKind::ActionModel;
    model.element.action = Action::East;
    check(counter,
          claimValueIsTrue(model, world, noHistory, noMismatches),
          "SC3: model claim is always true");
    CorrectionRecord trap;
    trap.element.kind = ElementKind::Trap;
    trap.element.cell = {1, 1};
    bool okTrap = claimValueIsTrue(trap, world, noHistory, noMismatches);
    trap.element.cell = {5, 3};
    okTrap = okTrap &&
             !claimValueIsTrue(trap, world, noHistory, noMismatches);
    check(counter, okTrap, "SC3: trap claim for (1,1) true, (5,3) false");
    WorldState second = world;
    second.agent = {4, 3};
    WorldState third = world;
    third.agent = {5, 3};
    third.object = {6, 3};
    check(counter, isValidWorld(second) && isValidWorld(third),
          "SC3: object history worlds are valid");
    std::vector<Observation> history;
    history.push_back(Observation{1, world, std::nullopt, std::nullopt});
    history.push_back(Observation{2, second, Action::East, MoveOutcome::Moved});
    history.push_back(Observation{3, third, Action::East, MoveOutcome::Pushed});
    MismatchRecord mismatch;
    mismatch.decisionStep = 3;
    const std::vector<MismatchRecord> mismatches{mismatch};
    CorrectionRecord object;
    object.element.kind = ElementKind::Object;
    object.newPosition = {5, 3};
    bool okObject = claimValueIsTrue(object, world, history, mismatches);
    object.newPosition = {6, 3};
    okObject = okObject &&
               !claimValueIsTrue(object, world, history, mismatches);
    okObject = okObject &&
               !claimValueIsTrue(object, world, history, noMismatches);
    check(counter, okObject, "SC3: object claim judged at its moment (D-042)");
}

}  // namespace

void runScorerTests(TestCounter& counter) {
    std::cout << "--- Scorer ---\n";
    checkSC1(counter);
    checkSC2(counter);
    checkSC3(counter);
}

}  // namespace ai_hologram
