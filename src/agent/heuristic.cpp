// Purpose: Implements the heuristic formula of plan 3.3.
// Introduced in Stage 3.
// Architecture: One table lookup in the DeadSquareMap plus two Manhattan
// distances. This is free but bounded (protocol 10.7): it is called only
// on states already produced by a budgeted evaluation, it does no search
// and it loops over no actions, so it can never create a successor state.
// Stage 4 IE-1 H4 addition: the object-cell read and the value are reported to a watch-only observer.

#include "agent/heuristic.h"

#include <algorithm>
#include <cstdlib>

namespace ai_hologram {
namespace {

// Manhattan distance: the steps between two cells when only the four
// directions are allowed and walls are ignored.
int manhattan(const Position first, const Position second) {
    return std::abs(first.x - second.x) + std::abs(first.y - second.y);
}

}  // namespace

int heuristic(const WorldState& state, const DeadSquareMap& map,
              FreeComputationCounts& counts) {
    ++counts.heuristicCalls;
    const std::size_t objectRow = static_cast<std::size_t>(state.object.y);
    const std::size_t objectColumn = static_cast<std::size_t>(state.object.x);
    int value = kInfiniteHeuristic;
    if (!readDeadStatus(map, state.object)) {
        // P is the relaxed push count when the analysis reached this cell.
        // An overridden "not dead" cell has no distance (-1), so fall back to
        // the straight-line distance from the object to the destination.
        const int pushDistance = map.pushDistance[objectRow][objectColumn];
        const int pushes = pushDistance >= 0
                               ? pushDistance
                               : manhattan(state.object, state.destination);
        // The agent must first walk next to the object; standing next to it
        // (distance 1) or on it (impossible in a valid world) costs 0 extra.
        const int walking = std::max(0, manhattan(state.agent, state.object) - 1);
        value = pushes + walking;
    }
    if (map.observer && map.observer->onHeuristic) {
        map.observer->onHeuristic(state, value);
    }
    return value;
}

}  // namespace ai_hologram
