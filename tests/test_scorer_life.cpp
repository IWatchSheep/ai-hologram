// Purpose: Checks SC4 and SC6: the life of one fault event driven
// by hand through the scorer's phases, and that scoring never
// changes a run (plan 6.6).
// Introduced in Stage 4.
// Architecture: SC4 calls the scorer itself and changes the agent's
// state by hand between calls as test set-up only; the scorer
// itself never writes to the agent. Every world is checked valid.

#include "agent/run_loop.h"
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

CorrectionRecord wallRecord(const int id, const bool newBlocked,
                            const bool oldBlocked, const int step) {
    CorrectionRecord record;
    record.id = id;
    record.element.kind = ElementKind::Wall;
    record.element.cell = {4, 3};
    record.newBlocked = newBlocked;
    record.oldBlocked = oldBlocked;
    record.status = CorrectionStatus::Consistent;
    record.stepConsistent = step;
    return record;
}

FaultEvent eventE() {
    FaultEvent event;
    event.id = "E";
    event.step = 2;
    event.type = FaultType::F1;
    event.direction = F1Direction::BlockedToFree;
    event.hasCell = true;
    event.cell = {4, 3};
    return event;
}

Decision actionDecision() {
    Decision decision;
    decision.isAction = true;
    decision.action = Action::North;
    return decision;
}

void checkSC4(TestCounter& counter) {
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(walled), "SC4: layout D2 is valid");
    SingleAgent agent(Observation{1, walled, std::nullopt, std::nullopt},
                      kSettings);
    std::vector<Observation> trueHistory;
    trueHistory.push_back(Observation{1, walled, std::nullopt, std::nullopt});
    Scorer scorer({eventE()}, walled, &trueHistory);
    std::vector<ActivationRecord> records;
    ActivationRecord activation;
    activation.eventId = "E";
    activation.type = FaultType::F1;
    activation.scheduledStep = 2;
    activation.status = ActivationStatus::Scheduled;
    records.push_back(activation);
    // Step 1: nothing happens yet.
    scorer.endPhaseA(viewOf(agent.state), 1);
    scorer.afterActivation(viewOf(agent.state), records, 1);
    scorer.afterDecide(viewOf(agent.state), actionDecision());
    // Step 2, phase A: a claim before activation is a false alarm.
    agent.state.corrections.push_back(wallRecord(1, true, true, 2));
    scorer.endPhaseA(viewOf(agent.state), 2);
    bool ok2 = scorer.run().falseAlarms == 1;
    ok2 = ok2 && scorer.run().claims == 1;
    ok2 = ok2 && !scorer.events()[0].activated;
    check(counter, ok2, "SC4: claim before activation is a false alarm");
    // Step 2: the fault is applied by hand (belief wall removed).
    agent.state.belief.walls[3][4] = false;
    records[0].status = ActivationStatus::Applied;
    records[0].applicationStep = 2;
    scorer.afterActivation(viewOf(agent.state), records, 2);
    bool ok3 = scorer.events()[0].activated;
    ok3 = ok3 && scorer.events()[0].activationStep == 2;
    ok3 = ok3 && scorer.events()[0].active;
    check(counter, ok3, "SC4: event activated at step 2 and active");
    scorer.afterDecide(viewOf(agent.state), actionDecision());
    // Step 3: a true correction ends the event.
    agent.state.corrections.push_back(wallRecord(2, true, false, 3));
    agent.state.belief.walls[3][4] = true;
    scorer.endPhaseA(viewOf(agent.state), 3);
    bool ok4 = scorer.events()[0].strictStep.has_value();
    ok4 = ok4 && scorer.events()[0].strictStep.value() == 3;
    ok4 = ok4 && scorer.events()[0].endState == EndState::Corrected;
    ok4 = ok4 && scorer.events()[0].endStep == 3;
    ok4 = ok4 && scorer.events()[0].correctingId == 2;
    ok4 = ok4 && scorer.run().falseAlarms == 1;
    check(counter, ok4, "SC4: event corrected at step 3 by record 2");
    scorer.afterActivation(viewOf(agent.state), records, 3);
    scorer.afterDecide(viewOf(agent.state), actionDecision());
    // Step 4: the correction is rejected and the fault returns.
    agent.state.corrections[1].status = CorrectionStatus::Rejected;
    agent.state.belief.walls[3][4] = false;
    scorer.endPhaseA(viewOf(agent.state), 4);
    bool ok5 = scorer.events()[0].endState == EndState::None;
    ok5 = ok5 && scorer.events()[0].active;
    ok5 = ok5 && scorer.events()[0].correctingId == 0;
    check(counter, ok5, "SC4: rejected correction revives the event");
    scorer.afterActivation(viewOf(agent.state), records, 4);
    Decision stop;
    stop.isAction = false;
    stop.action = Action::North;
    stop.stopReason = "test";
    scorer.afterDecide(viewOf(agent.state), stop);
    check(counter, scorer.run().justifiedSafeStops == 1,
          "SC4: safe stop while active is justified");
    // The final check and finish: still active at end of run.
    scorer.endFinalCheck(viewOf(agent.state));
    scorer.finish(viewOf(agent.state));
    bool ok6 = scorer.events()[0].endState == EndState::None;
    ok6 = ok6 && scorer.events()[0].strictStep.has_value();
    ok6 = ok6 && scorer.events()[0].strictStep.value() == 3;
    ok6 = ok6 && scorer.run().claims == 2;
    ok6 = ok6 && scorer.run().falseAlarms == 1;
    ok6 = ok6 && scorer.run().wrongAccepted == 0;
    ok6 = ok6 && !scorer.events()[0].correctionStatus.has_value();
    check(counter, ok6, "SC4: event active at end, totals exact");
}

void checkSC6(TestCounter& counter) {
    const WorldState world = layoutD();
    const WorldState walled = layoutD2();
    check(counter, isValidWorld(world) && isValidWorld(walled),
          "SC6: layouts D and D2 are valid");
    FaultEvent event;
    event.id = "demo-1";
    event.step = 1;
    event.type = FaultType::F1;
    event.direction = F1Direction::BlockedToFree;
    event.hasCell = true;
    event.cell = {4, 3};
    SingleAgent faulty(Observation{1, walled, std::nullopt, std::nullopt},
                       kSettings);
    const FaultRunResult injected =
        runWithFaults(walled, faulty, kSettings, {event});
    SingleAgent reference(Observation{1, walled, std::nullopt, std::nullopt},
                          kSettings, world);
    const RunResult expected = runSingleAgent(walled, reference, kSettings);
    bool ok = injected.run.lines == expected.lines;
    ok = ok && injected.run.outcome == expected.outcome;
    ok = ok && injected.run.evaluations == expected.evaluations;
    ok = ok && !faulty.state.activationHook;
    check(counter, ok,
          "SC6: FR1 lines unchanged with scorer, hook empty after run");
}

}  // namespace

void runScorerLifeTests(TestCounter& counter) {
    std::cout << "--- Scorer life ---\n";
    checkSC4(counter);
    checkSC6(counter);
}

}  // namespace ai_hologram
