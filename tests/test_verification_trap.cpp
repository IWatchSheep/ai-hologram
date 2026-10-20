// Purpose: Checks the trap claims of plan 3.7 B and 3.8 (VF9-VF12)
// and the trap part of VF8: a valid dead-square claim, an invalid
// one, the valid hypothetical states, and a full-state dead end.
// Introduced in Stage 3.
// Architecture: Split from test_verification.cpp so each file stays
// under about 200 lines. Every world used is checked valid.

#include "agent/heuristic.h"
#include "agent/investigation.h"
#include "agent/planner.h"
#include "agent/verification.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
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

WorldState trapWorld() {  // layout LT from the prompt
    return parseLayout({"##########", "#O.......#", "#........#", "#........#",
                        "#....A...#", "#........#", "#........#", "#.......D#",
                        "#........#", "##########"});
}

bool goalReached(const WorldState& state) {
    return state.object == state.destination;
}

SearchResult runSearch(const WorldState& root, const DeadSquareMap& map,
                       FreeComputationCounts& counts, StepPredictor& predictor) {
    predictor.startStep();
    const std::function<int(const WorldState&)> estimate =
        [&map, &counts](const WorldState& state) {
            return heuristic(state, map, counts);
        };
    return search(root, map, &goalReached, estimate, predictor, 1);
}

void checkVF9Valid(TestCounter& counter) {
    const WorldState belief = trapWorld();  // object already on (1, 1)
    check(counter, isValidWorld(belief) && belief.object == Position{1, 1},
          "VF9: layout LT is valid, with the object on (1, 1)");
    FreeComputationCounts counts;
    // The hand-set "not dead" override on the true dead square (1,1):
    // this is the hand-written wrong status a trap claim corrects.
    const DeadSquareMap map = computeDeadSquares(belief, {{1, 1}}, {}, counts);
    StepPredictor searchPredictor{WorldModel{}, 1000000};
    const SearchResult result = runSearch(belief, map, counts, searchPredictor);
    check(counter, result.kind == SearchKind::Exhausted,
          "VF9: a large-budget main search on LT returns Exhausted");
    const std::vector<Observation> history{
        Observation{1, belief, std::nullopt, std::nullopt}};
    // VF12: this Mismatch is the plan 3.7 B.3 consequence mismatch,
    // whose suspected source is S1 by the plan's rule.
    check(counter, checkConsequence(belief, map, history, counts) ==
                           ConsequenceOutcome::Mismatch,
          "VF9/VF12: checkConsequence returns Mismatch (the plan "
          "labels this consequence mismatch source S1)");
    check(counter, trapV1Check(belief, map, {1, 1}, history, counts),
          "VF9: trapV1Check((1,1)) is true (local check fails and "
          "the history check passes)");
    int currentCalls = 0;
    const ActionModel countingCurrent = [&currentCalls](
        const WorldState& state, Action action) {
        ++currentCalls;
        return applyAction(state, action);
    };
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    int anomalies = 0;
    const TrapV2Result trapResult = trapV2Test(
        belief, map, {1, 1}, predictor, countingCurrent, 1, anomalies);
    check(counter, trapResult == TrapV2Result::Verified &&
                       predictor.usedThisStep() == 2 && currentCalls == 2,
          "VF9/VF8: trapV2Test returns Verified using exactly 2 "
          "evaluations through the CURRENT model (pushes North and "
          "West; East and South are skipped, their standing cells "
          "(0,1) and (1,0) being walls)");
    WorldState northPush = belief;  // agent south of p, pushing North
    northPush.agent = {1, 2};
    WorldState westPush = belief;  // agent east of p, pushing West
    westPush.agent = {2, 1};
    check(counter, isValidWorld(northPush) && isValidWorld(westPush) &&
                       anomalies == 0,
          "VF10: both hypothetical states are valid worlds and "
          "anomalies stays 0 (no invalid hypothetical was reached)");
    StepPredictor tight{WorldModel{}, 2};
    tight.startStep();
    int tightAnomalies = 0;
    check(counter, trapV2Test(belief, map, {1, 1}, tight,
                               ActionModel{&applyAction}, 1, tightAnomalies) ==
                           TrapV2Result::Waiting &&
                       tight.usedThisStep() == 0,
          "VF9: remaining() - k < reserve makes trapV2Test wait, "
          "predicting nothing");
}

void checkVF9InvalidAndVF11(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "VF9: layout D is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(belief, {}, {}, counts);
    const std::vector<Observation> history{
        Observation{1, belief, std::nullopt, std::nullopt}};
    check(counter, !trapV1Check(belief, map, {5, 3}, history, counts),
          "VF9: trapV1Check((5,3)) in D is false (the local check "
          "holds there, so (5,3) cannot be claimed dead)");
    StepPredictor predictor{WorldModel{}, 200};
    predictor.startStep();
    int anomalies = 0;
    check(counter, trapV2Test(belief, map, {5, 3}, predictor,
                               ActionModel{&applyAction}, 1, anomalies) ==
                           TrapV2Result::Rejected,
          "VF9: trapV2Test at (5,3) in D is Rejected (a push moves "
          "the object onto a cell that is not dead, e.g. East onto "
          "(6,3))");
    const WorldState l2 = parseLayout(
        {"##########", "#....#...#", "#.O..#.A.#", "#....#...#", "#D...#...#",
         "######...#", "#........#", "#........#", "#........#", "##########"});
    check(counter, isValidWorld(l2), "VF11: layout L2 is a valid world");
    FreeComputationCounts l2Counts;
    const DeadSquareMap l2Map = computeDeadSquares(l2, {}, {}, l2Counts);
    StepPredictor l2Predictor{WorldModel{}, 1000000};
    const SearchResult l2Result = runSearch(l2, l2Map, l2Counts, l2Predictor);
    const std::vector<Observation> l2History{
        Observation{1, l2, std::nullopt, std::nullopt}};
    check(counter, l2Result.kind == SearchKind::Exhausted &&
                       checkConsequence(l2, l2Map, l2History, l2Counts) ==
                           ConsequenceOutcome::FullStateDeadEnd,
          "VF11: after L2's Exhausted search, checkConsequence returns "
          "FullStateDeadEnd (local check holds, so no claim is made)");
}

}  // namespace

void checkTrapTests(TestCounter& counter) {
    checkVF9Valid(counter);
    checkVF9InvalidAndVF11(counter);
}

}  // namespace ai_hologram
