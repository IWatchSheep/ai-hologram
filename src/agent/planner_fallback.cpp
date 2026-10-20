// Purpose: Implements the planner's fallback choice (plan 3.5).
// Introduced in Stage 3.
// Architecture: Split from planner.cpp so each file stays under about
// 200 lines. When a search cannot reach the goal (OutOfBudget or
// Exhausted), the caller uses this best path toward the most promising
// state the search did manage to discover. No state is predicted here:
// the fallback only reads the states the budgeted search produced.

#include "agent/heuristic.h"
#include "agent/planner.h"

#include <cstddef>
#include <vector>

namespace ai_hologram {
namespace {

// Is `candidate` a better fallback target than `current`? The order is
// (h, bestG, discoveryNumber), smallest first: prefer the state that
// looks closest to the goal, then the one reached by the fewest
// actions, then the one discovered earliest (plan 3.5).
bool isBetterFallback(const DiscoveredState& candidate,
                      const DiscoveredState& current) {
    if (candidate.h != current.h) {
        return candidate.h < current.h;
    }
    if (candidate.bestG != current.bestG) {
        return candidate.bestG < current.bestG;
    }
    return candidate.discoveryNumber < current.discoveryNumber;
}

// Builds the plan from the root to discovered[stateIndex] by following
// parent links backwards and then reversing them into order.
std::vector<PlanStep> buildPlanTo(const SearchResult& result,
                                  const int stateIndex) {
    std::vector<PlanStep> reversed;
    for (int at = stateIndex;
         result.discovered[static_cast<std::size_t>(at)].parentIndex != -1;) {
        const DiscoveredState& step =
            result.discovered[static_cast<std::size_t>(at)];
        reversed.push_back(PlanStep{step.action, step.state});
        at = step.parentIndex;
    }
    return {reversed.rbegin(), reversed.rend()};
}

}  // namespace

std::vector<PlanStep> fallback(const SearchResult& result) {
    // The root is never a fallback target: moving toward it would mean
    // taking no action at all. States with an infinite heuristic have
    // their object on a believed dead square and cannot lead to the
    // goal, so they are not targets either.
    int bestIndex = -1;
    for (std::size_t i = 0; i < result.discovered.size(); ++i) {
        const DiscoveredState& candidate = result.discovered[i];
        if (candidate.parentIndex == -1 || candidate.h >= kInfiniteHeuristic) {
            continue;
        }
        if (bestIndex < 0 ||
            isBetterFallback(candidate,
                             result.discovered[static_cast<std::size_t>(bestIndex)])) {
            bestIndex = static_cast<int>(i);
        }
    }
    if (bestIndex < 0) {
        return {};  // no action: the caller must take a safe stop
    }
    return buildPlanTo(result, bestIndex);
}

}  // namespace ai_hologram
