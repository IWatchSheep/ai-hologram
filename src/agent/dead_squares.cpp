// Purpose: Implements the dead-square analysis, its overrides, the local
// check, and the cache that reruns the analysis only on changed inputs.
// Introduced in Stage 3.
// Architecture: The analysis works backwards from the destination over
// object cells only. It is a free but bounded computation (protocol
// 10.7): at most 100 cells x 4 directions, no successor states, no model.
//
// A dead square is not the same as a full-state dead end (plan 3.3, 9.1).
// A dead square says the OBJECT can never reach the destination from that
// cell, even if the agent could stand anywhere ("relaxed" pushes ignore
// whether the agent can really walk to the pushing side). A full state
// (agent and object together) can still be unsolvable on a cell that is
// not dead, for example when the agent can never reach the pushing side.

#include "agent/dead_squares.h"
#include "world/action.h"

#include <cstddef>
#include <vector>

namespace ai_hologram {
namespace {

// The four directions, always in the fixed order North, East, South, West.
const std::array<Action, 4> kDirections = {Action::North, Action::East,
                                           Action::South, Action::West};

// A cell counts as free when it is inside the grid and not a believed
// wall. isWall already treats outside cells as walls.
bool isBelievedFree(const WorldState& belief, const Position cell) {
    return !isWall(belief, cell);
}

// Sets the dead flag of each listed cell, keeping the rule that walls and
// the destination are never dead. Nothing else about the cell changes:
// in particular its pushDistance is left exactly as computed.
void applyOverrides(DeadSquareMap& map, const WorldState& belief,
                    const std::vector<Position>& cells, const bool deadValue) {
    for (const Position cell : cells) {
        if (!isInsideGrid(cell) || isWall(belief, cell) ||
            cell == belief.destination) {
            continue;
        }
        map.dead[static_cast<std::size_t>(cell.y)]
                [static_cast<std::size_t>(cell.x)] = deadValue;
    }
}

// Marks every believed-free cell that got no distance as dead.
void markUnreachedCellsDead(DeadSquareMap& map, const WorldState& belief) {
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            const std::size_t row = static_cast<std::size_t>(y);
            const std::size_t column = static_cast<std::size_t>(x);
            if (!isWall(belief, {x, y}) && belief.destination != Position{x, y}) {
                map.dead[row][column] = map.pushDistance[row][column] < 0;
            }
        }
    }
}

}  // namespace

DeadSquareMap computeDeadSquares(const WorldState& belief,
                                 const std::vector<Position>& notDeadOverrides,
                                 const std::vector<Position>& deadCorrections,
                                 FreeComputationCounts& counts) {
    DeadSquareMap map;
    for (auto& row : map.pushDistance) {
        row.fill(-1);  // -1 means "not reached by relaxed pushes yet".
    }
    ++counts.analysisRuns;

    // Breadth-first search backwards from the destination. The queue is
    // a list read from the front with an index, so cells are visited in
    // the order they were discovered and distances grow step by step.
    std::vector<Position> queue;
    if (isInsideGrid(belief.destination)) {
        map.pushDistance[static_cast<std::size_t>(belief.destination.y)]
                        [static_cast<std::size_t>(belief.destination.x)] = 0;
        queue.push_back(belief.destination);
    }
    for (std::size_t front = 0; front < queue.size(); ++front) {
        const Position reached = queue[front];
        const int reachedDistance =
            map.pushDistance[static_cast<std::size_t>(reached.y)]
                            [static_cast<std::size_t>(reached.x)];
        for (const Action direction : kDirections) {
            ++counts.cellOperations;  // one (cell, direction) examined
            const Position offset = offsetFor(direction);
            // q could push in `direction` into `reached`; s is where the
            // agent would have to stand for that push.
            const Position q{reached.x - offset.x, reached.y - offset.y};
            const Position s{reached.x - 2 * offset.x, reached.y - 2 * offset.y};
            if (!isInsideGrid(q) || !isBelievedFree(belief, q) ||
                !isBelievedFree(belief, s)) {
                continue;
            }
            int& distance = map.pushDistance[static_cast<std::size_t>(q.y)]
                                            [static_cast<std::size_t>(q.x)];
            if (distance < 0) {
                distance = reachedDistance + 1;
                queue.push_back(q);
            }
        }
    }

    markUnreachedCellsDead(map, belief);
    // Overrides, in this order (plan 3.3): "not dead" first, then "dead",
    // so a "dead" correction wins when a cell is in both lists. In Stage 3
    // only tests set the "not dead" list; Stage 4 will use it for F4.
    applyOverrides(map, belief, notDeadOverrides, false);
    applyOverrides(map, belief, deadCorrections, true);
    return map;
}

bool localCheckSaysNotDead(const WorldState& belief, const DeadSquareMap& map,
                           const Position cell, FreeComputationCounts& counts) {
    ++counts.localChecks;
    if (cell == belief.destination) {
        return true;
    }
    // Re-check the analysis rule at this one cell only. The stored status
    // of `cell` itself is deliberately never read: this check is used to
    // test whether that stored status agrees with its neighbours.
    for (const Action direction : kDirections) {
        const Position offset = offsetFor(direction);
        const Position behind{cell.x - offset.x, cell.y - offset.y};
        const Position ahead{cell.x + offset.x, cell.y + offset.y};
        if (isBelievedFree(belief, behind) && isBelievedFree(belief, ahead) &&
            !map.dead[static_cast<std::size_t>(ahead.y)]
                     [static_cast<std::size_t>(ahead.x)]) {
            return true;
        }
    }
    return false;
}

const DeadSquareMap& DeadSquareCache::get(
    const WorldState& belief, const std::vector<Position>& notDeadOverrides,
    const std::vector<Position>& deadCorrections, FreeComputationCounts& counts) {
    // Only the walls, the destination and the override lists are inputs
    // of the analysis, so only they can make the cached map out of date.
    const bool inputsChanged = !hasCachedMap_ || belief.walls != cachedWalls_ ||
                               belief.destination != cachedDestination_ ||
                               notDeadOverrides != cachedNotDeadOverrides_ ||
                               deadCorrections != cachedDeadCorrections_;
    if (inputsChanged) {
        map = computeDeadSquares(belief, notDeadOverrides, deadCorrections, counts);
        cachedWalls_ = belief.walls;
        cachedDestination_ = belief.destination;
        cachedNotDeadOverrides_ = notDeadOverrides;
        cachedDeadCorrections_ = deadCorrections;
        hasCachedMap_ = true;
    }
    return map;
}

}  // namespace ai_hologram
