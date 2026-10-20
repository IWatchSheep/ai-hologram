// Purpose: Implements the footprint rules for one-step disagreement checks.
// Introduced in Stage 3.
// Architecture: The footprint is computed from the pre-action belief, the action, and the observed result.

#include "agent/footprint.h"

namespace ai_hologram {
namespace {

bool isCellDifferent(const bool left, const bool right) {
    return left != right;
}

}  // namespace

std::vector<Position> computeFootprint(const WorldState& beliefBefore,
                                      const Action action,
                                      const WorldState& observedAfter,
                                      const MoveOutcome observedOutcome) {
    const Position c0 = beliefBefore.agent;
    const Position c1 = moved(c0, action);
    const Position c2 = moved(c1, action);
    std::vector<Position> cells;
    cells.push_back(c0);
    cells.push_back(c1);

    const bool includeC2 = beliefBefore.object == c1 ||
                           observedOutcome == MoveOutcome::Pushed ||
                               observedAfter.object == c1;

    if (includeC2) {
        cells.push_back(c2);
    }

    return cells;
}

FootprintComparison compareFootprint(const WorldState& predictedState,
                                    const MoveOutcome predictedOutcome,
                                    const WorldState& observedState,
                                    const MoveOutcome observedOutcome,
                                    const std::vector<Position>& cells) {
    FootprintComparison comparison;
    comparison.outcomesDiffer = predictedOutcome != observedOutcome;

    for (const Position cell : cells) {
        FootprintCellDifference difference;
        difference.cell = cell;
        difference.predictedWall = isWall(predictedState, cell);
        difference.observedWall = isWall(observedState, cell);
        difference.wallDiffers = isCellDifferent(difference.predictedWall,
                                                difference.observedWall);

        difference.predictedAgentHere = predictedState.agent == cell;
        difference.observedAgentHere = observedState.agent == cell;
        difference.agentDiffers = isCellDifferent(difference.predictedAgentHere,
                                                 difference.observedAgentHere);

        difference.predictedObjectHere = predictedState.object == cell;
        difference.observedObjectHere = observedState.object == cell;
        difference.objectDiffers = isCellDifferent(difference.predictedObjectHere,
                                                  difference.observedObjectHere);

        if (difference.wallDiffers || difference.agentDiffers ||
            difference.objectDiffers) {
            comparison.cells.push_back(difference);
        }
    }

    return comparison;
}

std::vector<Position> computeFootprintForTwoPredictions(
    const WorldState& firstBelief,
    const MoveOutcome firstOutcome,
    const WorldState& secondBelief,
    const MoveOutcome secondOutcome,
    const Action action) {
    const Position c0 = firstBelief.agent;
    const Position c1 = moved(c0, action);
    const Position c2 = moved(c1, action);
    std::vector<Position> cells;
    cells.push_back(c0);
    cells.push_back(c1);

    const bool includeC2 = firstBelief.object == c1 ||
                           secondBelief.object == c1 ||
                           firstOutcome == MoveOutcome::Pushed ||
                           secondOutcome == MoveOutcome::Pushed;
    if (includeC2) {
        cells.push_back(c2);
    }

    return cells;
}

void applyFootprintUpdate(WorldState& belief,
                         const std::vector<Position>& cells,
                         const WorldState& observedState) {
    for (const Position cell : cells) {
        if (!isInsideGrid(cell)) {
            continue;
        }

        const std::size_t x = static_cast<std::size_t>(cell.x);
        const std::size_t y = static_cast<std::size_t>(cell.y);
        belief.walls[y][x] = observedState.walls[y][x];

        if (observedState.agent == cell) {
            belief.agent = observedState.agent;
        }

        if (observedState.object == cell || belief.object == cell) {
            belief.object = observedState.object;
        }
    }
}

}  // namespace ai_hologram
