// Purpose: Implements the shadow computations of plan 3.9.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. A
// shadow repeats one watched computation with one faulted element
// at its true value. It calls the model function itself, so it
// never uses any reasoner's budget, and it is never visible to a
// reasoner. It can only watch, like a camera: "the chain can be
// replaced by the camera", and ours cannot pull because nothing
// here returns a value to any agent. Trap-status READS are
// observed, not whole analysis runs (D-040), so the shadow map is
// compared one read cell at a time by the scorer.

#include "experiment/shadow.h"

#include "agent/footprint.h"
#include "world/rules.h"

#include <cstddef>
#include <vector>

namespace ai_hologram {
namespace {

// True when two predictions differ (plan 3.9): the outcomes differ,
// or the footprint contents differ on the cells of the two-
// prediction footprint rule, exactly as in 4d's committedEffective.
bool predictionsDiffer(const WorldState& firstBelief, const Prediction& first,
                       const WorldState& secondBelief,
                       const ActionResult& second, const Action action) {
    if (first.outcome != second.outcome) {
        return true;
    }
    const std::vector<Position> cells = computeFootprintForTwoPredictions(
        firstBelief, first.outcome, secondBelief, second.outcome, action);
    const FootprintComparison compared = compareFootprint(
        first.state, first.outcome, second.state, second.outcome, cells);
    return compared.outcomesDiffer || !compared.cells.empty();
}

}  // namespace

std::optional<WorldState> shadowInput(const FaultEvent& event,
                                      const WorldState& state,
                                      const ScoredView& view,
                                      const WorldState& trueLayout,
                                      const Position trueObject) {
    if (event.type == FaultType::F1) {
        if (!event.hasCell) {
            return std::nullopt;
        }
        if (isWall(state, event.cell) == isWall(trueLayout, event.cell)) {
            return std::nullopt;
        }
        if (!isInsideGrid(event.cell)) {
            return std::nullopt;
        }
        WorldState shadow = state;
        const std::size_t row = static_cast<std::size_t>(event.cell.y);
        const std::size_t column = static_cast<std::size_t>(event.cell.x);
        shadow.walls[row][column] = isWall(trueLayout, event.cell);
        return shadow;
    }
    if (event.type == FaultType::F2 || event.type == FaultType::F5) {
        if (view.belief == nullptr) {
            return std::nullopt;
        }
        if (view.belief->object == trueObject) {
            return std::nullopt;
        }
        if (state.object != view.belief->object) {
            return std::nullopt;
        }
        WorldState shadow = state;
        shadow.object = trueObject;
        return shadow;
    }
    return std::nullopt;
}

bool evaluationEffective(const FaultEvent& event, const WorldState& state,
                         const Action action, const ActionModel& modelUsed,
                         const Prediction& result, const ScoredView& view,
                         const WorldState& trueLayout,
                         const Position trueObject) {
    if (event.type == FaultType::F4) {
        return false;
    }
    if (event.type == FaultType::F3) {
        if (action != event.action) {
            return false;
        }
        if (view.model == nullptr || view.model->isRestored(action)) {
            return false;
        }
        const ActionResult standard = applyAction(state, action);
        return predictionsDiffer(state, result, state, standard, action);
    }
    const std::optional<WorldState> shadow =
        shadowInput(event, state, view, trueLayout, trueObject);
    if (!shadow.has_value()) {
        return false;
    }
    if (!isValidWorld(*shadow)) {
        return true;
    }
    const ActionResult shadowResult = modelUsed(*shadow, action);
    return predictionsDiffer(state, result, *shadow, shadowResult, action);
}

std::optional<DeadSquareMap> shadowMap(const FaultEvent& event,
                                       const ScoredView& view,
                                       const WorldState& trueLayout) {
    if (view.belief == nullptr || view.notDeadOverrides == nullptr ||
        view.deadOverrides == nullptr) {
        return std::nullopt;
    }
    FreeComputationCounts localCounts;
    if (event.type == FaultType::F1) {
        if (!event.hasCell || !isInsideGrid(event.cell)) {
            return std::nullopt;
        }
        if (isWall(*view.belief, event.cell) == isWall(trueLayout, event.cell)) {
            return std::nullopt;
        }
        WorldState belief = *view.belief;
        const std::size_t row = static_cast<std::size_t>(event.cell.y);
        const std::size_t column = static_cast<std::size_t>(event.cell.x);
        belief.walls[row][column] = isWall(trueLayout, event.cell);
        return computeDeadSquares(belief, *view.notDeadOverrides,
                                  *view.deadOverrides, localCounts);
    }
    if (event.type == FaultType::F4) {
        std::vector<Position> overrides = *view.notDeadOverrides;
        for (std::size_t i = 0; i < overrides.size(); ++i) {
            if (overrides[i] == event.cell) {
                overrides.erase(overrides.begin() +
                                static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
        return computeDeadSquares(*view.belief, overrides,
                                  *view.deadOverrides, localCounts);
    }
    return std::nullopt;
}

}  // namespace ai_hologram
