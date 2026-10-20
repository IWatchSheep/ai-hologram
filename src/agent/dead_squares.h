// Purpose: Declares the dead-square (trap) analysis, its local check and
// the cache that reruns the analysis only when its inputs change.
// Introduced in Stage 3.
// Architecture: This is one of the free computations of protocol 10.7. It
// looks at object cells only, never builds agent-object states and never
// calls any model, so it is not charged to the budget counter.
// Stage 4 IE-1 H4 addition: a watch-only observer is carried by the map.

#pragma once

#include "agent/computation_observer.h"
#include "world/world_state.h"

#include <array>
#include <vector>

namespace ai_hologram {

// The result of the dead-square analysis for every cell of the grid.
// dead[y][x] says whether the object can never reach the destination from
// cell (x, y) by relaxed pushes (see dead_squares.cpp). pushDistance[y][x]
// is the number of relaxed pushes needed, or -1 when the destination
// cannot be reached that way. Walls and the destination are never dead.
struct DeadSquareMap {
    std::array<std::array<bool, gridSize>, gridSize> dead{};
    std::array<std::array<int, gridSize>, gridSize> pushDistance{};
    const ComputationObserver* observer = nullptr;
};

// Counters for the free computations (protocol 10.7, plan 3.3). These are
// counted and reported, but they are NOT model evaluations: they never
// create a successor state, so they do not use the BudgetCounter.
// - analysisRuns: how many times the dead-square analysis ran.
// - cellOperations: (cell, direction) pairs examined by the analysis.
// - localChecks: how many times the one-cell local check ran.
// - heuristicCalls: how many times the heuristic was computed.
struct FreeComputationCounts {
    int analysisRuns = 0;
    int cellOperations = 0;
    int localChecks = 0;
    int heuristicCalls = 0;
};

// Runs the backward relaxed-push search from the destination of `belief`
// over the believed walls (plan 3.3), then applies the overrides in order:
// first the "not dead" overrides, then the "dead" corrections. Overrides
// change only the dead flag of the listed cells.
DeadSquareMap computeDeadSquares(const WorldState& belief,
                                 const std::vector<Position>& notDeadOverrides,
                                 const std::vector<Position>& deadCorrections,
                                 FreeComputationCounts& counts);

// Reads one cell's stored dead status. Returns exactly the stored
// value and, when an observer is attached, reports (cell, value) to
// it first. This is a watch-only report: it changes nothing.
bool readDeadStatus(const DeadSquareMap& map, Position cell);

// The free one-cell check of protocol 11.4 B: true when `cell` is the
// destination, or some direction has both neighbours believed free and
// the far neighbour not dead. It never reads the stored status of `cell`.
bool localCheckSaysNotDead(const WorldState& belief, const DeadSquareMap& map,
                           Position cell, FreeComputationCounts& counts);

// Remembers the last map and the inputs it was computed from. A new map
// is computed only when the believed walls, the destination, or either
// override list has changed (plan 3.3, "When the analysis runs"); the
// agent's and object's positions do not affect the analysis.
struct DeadSquareCache {
    // Returns the current map, recomputing it first if the inputs changed.
    const DeadSquareMap& get(const WorldState& belief,
                             const std::vector<Position>& notDeadOverrides,
                             const std::vector<Position>& deadCorrections,
                             FreeComputationCounts& counts);

    DeadSquareMap map{};
    const ComputationObserver* observer = nullptr;

private:
    bool hasCachedMap_ = false;
    std::array<std::array<bool, gridSize>, gridSize> cachedWalls_{};
    Position cachedDestination_{0, 0};
    std::vector<Position> cachedNotDeadOverrides_;
    std::vector<Position> cachedDeadCorrections_;
};

}  // namespace ai_hologram
