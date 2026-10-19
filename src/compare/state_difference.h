// Purpose: Declares the position, wall, and prediction comparison results.
// Introduced in Stage 2.
// Architecture: Comparison results describe differences between predictions and observations.

#pragma once

#include "model/prediction.h"

#include <vector>

namespace ai_hologram {

struct PositionDifference {
    bool differs = false;
    Position predicted{};
    Position observed{};
};

struct WallDifference {
    Position cell{};
    bool predictedBlocked = false;
    bool observedBlocked = false;
};

struct StateDifference {
    PositionDifference agent;
    PositionDifference object;
    PositionDifference destination;
    std::vector<WallDifference> walls;
};

bool hasDifferences(const StateDifference& difference);
StateDifference compareStates(const WorldState& predicted, const WorldState& observed);

struct PredictionComparison {
    MoveOutcome predictedOutcome;
    MoveOutcome observedOutcome;
    StateDifference state;
};

bool outcomesDiffer(const PredictionComparison& comparison);
bool matches(const PredictionComparison& comparison);
PredictionComparison comparePrediction(const Prediction& prediction,
                                       const ActionResult& observed);

}  // namespace ai_hologram
