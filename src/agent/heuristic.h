// Purpose: Declares the planner's heuristic estimate (plan 3.3).
// Introduced in Stage 3.
// Architecture: The heuristic is a free computation (protocol 10.7): a
// fixed formula of table lookups in the DeadSquareMap plus Manhattan
// distances. It does no search and never calls any model.

#pragma once

#include "agent/dead_squares.h"
#include "world/world_state.h"

namespace ai_hologram {

// A value far larger than any real plan length on a 10x10 grid. It means
// "unreachable": the object is on a believed dead square, from which it
// can never reach the destination (see dead_squares.cpp on how a dead
// square differs from a full-state dead end).
constexpr int kInfiniteHeuristic = 1000000000;

// Estimates the actions still needed from `state`: the relaxed push
// distance of the object's cell, plus the steps the agent needs to walk
// to the cell next to the object. Infinite on a dead square.
int heuristic(const WorldState& state, const DeadSquareMap& map,
              FreeComputationCounts& counts);

}  // namespace ai_hologram
