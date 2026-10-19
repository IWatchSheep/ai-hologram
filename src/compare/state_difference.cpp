// Purpose: Compares predicted and observed positions, walls, and outcomes.
// Introduced in Stage 2.
// Architecture: Pure comparison functions provide complete results for later reporting.

#include "compare/state_difference.h"

#include <cstddef>

namespace ai_hologram {
namespace {

PositionDifference comparePosition(const Position predicted,
                                   const Position observed) {
    return {predicted != observed, predicted, observed};
}

}  // namespace

bool hasDifferences(const StateDifference& difference) {
    return difference.agent.differs || difference.object.differs ||
           difference.destination.differs || !difference.walls.empty();
}

StateDifference compareStates(const WorldState& predicted,
                              const WorldState& observed) {
    StateDifference difference;
    difference.agent = comparePosition(predicted.agent, observed.agent);
    difference.object = comparePosition(predicted.object, observed.object);
    difference.destination =
        comparePosition(predicted.destination, observed.destination);

    // Use only our own grid coordinates here; stored positions are data, not
    // safe wall-array indexes, even if a caller supplies an invalid world.
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            const std::size_t row = static_cast<std::size_t>(y);
            const std::size_t column = static_cast<std::size_t>(x);
            const bool predictedBlocked = predicted.walls[row][column];
            const bool observedBlocked = observed.walls[row][column];
            if (predictedBlocked != observedBlocked) {
                difference.walls.push_back(
                    {{x, y}, predictedBlocked, observedBlocked});
            }
        }
    }
    return difference;
}

bool outcomesDiffer(const PredictionComparison& comparison) {
    return comparison.predictedOutcome != comparison.observedOutcome;
}

bool matches(const PredictionComparison& comparison) {
    return !outcomesDiffer(comparison) && !hasDifferences(comparison.state);
}

PredictionComparison comparePrediction(const Prediction& prediction,
                                       const ActionResult& observed) {
    return {prediction.outcome, observed.outcome,
            compareStates(prediction.state, observed.state)};
}

}  // namespace ai_hologram
