// Purpose: Declares the A* planner over the agent's own belief and its
// fallback choice (plan 3.5).
// Introduced in Stage 3.
// Architecture: The planner never creates a successor state itself.
// Every successor comes from StepPredictor::predict, which charges the
// step budget (protocol 10.7); the root is the only state examined that
// did not come from a budgeted evaluation, because the caller gives it.

#pragma once

#include "agent/dead_squares.h"
#include "agent/step_predictor.h"
#include "world/action.h"
#include "world/world_state.h"

#include <functional>
#include <vector>

namespace ai_hologram {

// One step of a plan: the action to take, and the state the planner
// expects afterwards (the state the predictor produced for it).
struct PlanStep {
    Action action = Action::North;
    WorldState expected{};
};

// How a search ended. Found: a goal state was taken from the queue.
// OutOfBudget: the reserve rule stopped the search. Exhausted: the
// queue emptied without reaching the goal.
enum class SearchKind { Found, OutOfBudget, Exhausted };

// Bookkeeping kept for every state the search discovers (plan 3.5).
// A state is identified by (agent position, object position); the walls
// and destination are the belief's, shared by all discovered states.
struct DiscoveredState {
    WorldState state{};
    int bestG = 0;        // fewest actions found so far from the root
    int h = 0;            // heuristic, computed once at discovery
    int parentIndex = -1; // index into `discovered`; -1 for the root
    Action action = Action::North; // action from the parent to this state
    int discoveryNumber = 0;       // 0 for the root, then 1, 2, 3, ...
};

struct SearchResult {
    SearchKind kind = SearchKind::Exhausted;
    std::vector<PlanStep> plan;
    std::vector<DiscoveredState> discovered;
    int improvedPaths = 0;  // bestG replaced by a shorter path
    int reopenedStates = 0; // of those, how many reopened a closed state
};

// The A* search of plan 3.5. The open list is a std::priority_queue of
// entries ordered by (f = g + h, h, entry number), smallest first; see
// planner.cpp for the beginner explanation of that container.
//
// `reserve` is the number of evaluations that must always be kept back
// for the mandatory predictions of the step (the committed prediction,
// plus one V2 alternative per pending correction, plan 3.4). This
// search is optional work, so before every prediction that is not
// already cached it checks that spending one evaluation would still
// leave the reserve; if not, it returns OutOfBudget.
SearchResult search(const WorldState& root, const DeadSquareMap& map,
                    const std::function<bool(const WorldState&)>& goalTest,
                    const std::function<int(const WorldState&)>& heuristic,
                    StepPredictor& predictor, int reserve);

// After OutOfBudget or Exhausted: the best path to the discovered
// non-root state with the smallest (h, bestG, discoveryNumber) whose h
// is finite. An empty result means there is no action to take.
std::vector<PlanStep> fallback(const SearchResult& result);

}  // namespace ai_hologram
