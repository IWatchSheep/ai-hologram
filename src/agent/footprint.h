// Purpose: Computes the disagreement footprint and applies the matching update.
// Introduced in Stage 3.
// Architecture: These helpers compare the predicted step to the observed step without rechecking the whole belief.

#pragma once

#include "world/action.h"
#include "world/rules.h"
#include "world/world_state.h"

#include <vector>

namespace ai_hologram {

struct FootprintCellDifference {
    Position cell{};
    bool wallDiffers = false;
    bool agentDiffers = false;
    bool objectDiffers = false;
    bool predictedWall = false;
    bool observedWall = false;
    bool predictedAgentHere = false;
    bool observedAgentHere = false;
    bool predictedObjectHere = false;
    bool observedObjectHere = false;
};

struct FootprintComparison {
    bool outcomesDiffer = false;
    std::vector<FootprintCellDifference> cells;
};

std::vector<Position> computeFootprint(const WorldState& beliefBefore,
                                      Action action,
                                      const WorldState& observedAfter,
                                      MoveOutcome observedOutcome);
FootprintComparison compareFootprint(const WorldState& predictedState,
                                    MoveOutcome predictedOutcome,
                                    const WorldState& observedState,
                                    MoveOutcome observedOutcome,
                                    const std::vector<Position>& cells);
std::vector<Position> computeFootprintForTwoPredictions(
    const WorldState& firstBelief,
    MoveOutcome firstOutcome,
    const WorldState& secondBelief,
    MoveOutcome secondOutcome,
    Action action);
void applyFootprintUpdate(WorldState& belief,
                         const std::vector<Position>& cells,
                         const WorldState& observedState);

}  // namespace ai_hologram
