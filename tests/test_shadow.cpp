// Purpose: Checks SH1: shadowInput, evaluationEffective, shadowMap
// and heuristic give exactly the plan 6.6 values for F1, F2, F3 and
// F4, with hand-built views and activated events.
// Introduced in Stage 4.
// Architecture: These tests call the experimenter's shadow
// functions directly. A shadow computation repeats one computation
// with one faulted element at its true value; it never uses the
// reasoner's budget (the model function is called directly here)
// and is never visible to an agent. The observer that will watch
// such computations can only watch: in the owner's words "the
// chain can be replaced by the camera", and ours cannot pull
// because it receives only const data and returns void. Trap-status
// READS are observed, not whole analysis runs (D-040), so SH1
// compares the shadow map cell by cell and through the heuristic.

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

WorldState layoutD2() {
    const ParseResult parsed = parseWorld(
        {"##########", "#........#", "#........#", "#..A#O..D#",
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

FaultEvent eventOf(const FaultType type) {
    FaultEvent event;
    event.type = type;
    event.hasCell = true;
    return event;
}

void checkF1(TestCounter& counter) {
    const WorldState belief = layoutD();
    const WorldState trueLayout = layoutD2();
    check(counter, isValidWorld(belief) && isValidWorld(trueLayout),
          "SH1 F1: layouts D and D2 are valid");
    FaultEvent event = eventOf(FaultType::F1);
    event.cell = {4, 3};
    event.direction = F1Direction::BlockedToFree;
    AgentActionModel agentModel;
    std::vector<Position> notDead;
    std::vector<Position> dead;
    ScoredView view;
    view.belief = &belief;
    view.model = &agentModel;
    view.notDeadOverrides = &notDead;
    view.deadOverrides = &dead;
    const ActionModel model = agentModel.current();
    const Prediction east = predictionOf(model, belief, Action::East);
    bool ok = evaluationEffective(event, belief, Action::East, model, east,
                                  view, trueLayout, trueLayout.object);
    const Prediction north = predictionOf(model, belief, Action::North);
    ok = ok && !evaluationEffective(event, belief, Action::North, model,
                                    north, view, trueLayout,
                                    trueLayout.object);
    check(counter, ok, "SH1 F1: East from belief effective, North not");
}

void checkF2(TestCounter& counter) {
    WorldState belief = layoutD();
    belief.object = {5, 2};
    const WorldState trueLayout = layoutD();
    check(counter, isValidWorld(belief) && isValidWorld(trueLayout),
          "SH1 F2: belief and true layout are valid");
    FaultEvent event = eventOf(FaultType::F2);
    AgentActionModel agentModel;
    std::vector<Position> notDead;
    std::vector<Position> dead;
    ScoredView view;
    view.belief = &belief;
    view.model = &agentModel;
    view.notDeadOverrides = &notDead;
    view.deadOverrides = &dead;
    const ActionModel model = agentModel.current();
    const Position trueObject{5, 3};
    WorldState state = belief;
    state.agent = {4, 2};
    check(counter, isValidWorld(state), "SH1 F2: East state is valid");
    const Prediction east = predictionOf(model, state, Action::East);
    bool ok = evaluationEffective(event, state, Action::East, model, east,
                                  view, trueLayout, trueObject);
    const Prediction west = predictionOf(model, state, Action::West);
    ok = ok && !evaluationEffective(event, state, Action::West, model, west,
                                    view, trueLayout, trueObject);
    check(counter, ok, "SH1 F2: East effective, West not");
    WorldState odd = belief;
    odd.agent = {5, 3};
    check(counter, isValidWorld(odd), "SH1 F2: invalid-shadow state valid");
    const Prediction north = predictionOf(model, odd, Action::North);
    check(counter,
          evaluationEffective(event, odd, Action::North, model, north, view,
                              trueLayout, trueObject),
          "SH1 F2: invalid shadow makes North effective");
}

}  // namespace

void runShadowTests(TestCounter& counter) {
    std::cout << "--- Shadow ---\n";
    checkF1(counter);
    checkF2(counter);
    runShadowMoreTests(counter);
}

}  // namespace ai_hologram
