// Purpose: Checks the F3 and F4 parts of SH1 (plan 6.6).
// Introduced in Stage 4.
// Architecture: Split from test_shadow.cpp so each file stays under
// about 200 lines. What a shadow computation is, and why it never
// uses the reasoner's budget, is explained in shadow.h: it repeats
// one watched computation with the faulted element at its true
// value, calling the model directly. The observer can only watch
// ("the chain can be replaced by the camera", and ours cannot
// pull). Trap-status READS are observed, not whole analysis runs
// (D-040), so the F4 shadow is a map compared cell by cell.

#include "agent/heuristic.h"
#include "experiment/fault_injector.h"
#include "experiment/shadow.h"
#include "test_suites.h"
#include "world/rules.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

WorldState layoutD() {
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A.O..D#",
         "#........#", "#........#", "#...##...#", "#........#",
         "#........#", "##########"});
    return *parsed.world;
}

Prediction predictionOf(const ActionModel& model, const WorldState& state,
                        const Action action) {
    const ActionResult result = model(state, action);
    Prediction prediction;
    prediction.action = action;
    prediction.outcome = result.outcome;
    prediction.state = result.state;
    return prediction;
}

void checkF3(TestCounter& counter) {
    const WorldState belief = layoutD();
    check(counter, isValidWorld(belief), "SH1 F3: layout D is valid");
    FaultEvent event;
    event.type = FaultType::F3;
    event.action = Action::East;
    event.rotation = Rotation::Right;
    FaultedActions faults;
    faults.faulted[1] = true;
    faults.rotation[1] = Rotation::Right;
    const ActionModel faulty = makeFaultyModel(faults);
    AgentActionModel agentModel(faulty);
    std::vector<Position> notDead;
    std::vector<Position> dead;
    ScoredView view;
    view.belief = &belief;
    view.model = &agentModel;
    view.notDeadOverrides = &notDead;
    view.deadOverrides = &dead;
    WorldState state = belief;
    state.agent = {4, 3};
    check(counter, isValidWorld(state), "SH1 F3: state is valid");
    const Prediction faultyEast = predictionOf(faulty, state, Action::East);
    bool ok = evaluationEffective(event, state, Action::East, faulty,
                                  faultyEast, view, belief, belief.object);
    const ActionModel standard{&applyAction};
    const Prediction standardEast =
        predictionOf(standard, state, Action::East);
    ok = ok && !evaluationEffective(event, state, Action::East, standard,
                                    standardEast, view, belief,
                                    belief.object);
    const Prediction west = predictionOf(faulty, state, Action::West);
    ok = ok && !evaluationEffective(event, state, Action::West, faulty, west,
                                    view, belief, belief.object);
    check(counter, ok, "SH1 F3: faulty East effective, standard/West not");
}

void checkF4(TestCounter& counter) {
    const WorldState belief = layoutD();
    check(counter, isValidWorld(belief), "SH1 F4: layout D is valid");
    FaultEvent event;
    event.type = FaultType::F4;
    event.hasCell = true;
    event.cell = {1, 1};
    std::vector<Position> notDead{{1, 1}};
    std::vector<Position> dead;
    ScoredView view;
    view.belief = &belief;
    view.notDeadOverrides = &notDead;
    view.deadOverrides = &dead;
    const std::optional<DeadSquareMap> shadow =
        shadowMap(event, view, belief);
    FreeComputationCounts counts;
    const DeadSquareMap real =
        computeDeadSquares(belief, notDead, dead, counts);
    bool ok = shadow.has_value();
    if (ok) {
        ok = ok && shadow->dead[1][1];
        ok = ok && !real.dead[1][1];
        ok = ok && shadow->dead[2][2] == real.dead[2][2];
    }
    check(counter, ok, "SH1 F4: shadow (1,1) dead, real not, (2,2) equal");
    WorldState state = belief;
    state.agent = {3, 3};
    state.object = {1, 1};
    check(counter, isValidWorld(state), "SH1 F4: heuristic state valid");
    bool okHeuristic = heuristic(state, real, counts) == 12;
    if (shadow.has_value()) {
        okHeuristic = okHeuristic &&
                      heuristic(state, *shadow, counts) == kInfiniteHeuristic;
    } else {
        okHeuristic = false;
    }
    check(counter, okHeuristic,
          "SH1 F4: heuristic 12 with real map, infinite with shadow");
}

}  // namespace

void runShadowMoreTests(TestCounter& counter) {
    checkF3(counter);
    checkF4(counter);
}

}  // namespace ai_hologram
