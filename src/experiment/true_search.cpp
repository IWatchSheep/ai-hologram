// Purpose: Implements the experimenter's breadth-first search over true
// states (plan 3.2).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent file
// may include its header, and no agent ever calls this function. It uses
// the same Stage 1 rules function the Environment uses to apply an
// action, so the measured length is a length in real world steps.

#include "experiment/true_search.h"

#include "world/action.h"
#include "world/rules.h"

#include <array>
#include <cstddef>
#include <queue>

namespace ai_hologram {
namespace {

// visited[agentY][agentX][objectY][objectX]: a state is identified by
// (agent position, object position), because the walls and destination
// never change during the search. Remembering seen states means each of
// the at most 100 x 100 states is expanded at most once.
using Visited =
    std::array<std::array<std::array<std::array<bool, gridSize>, gridSize>,
                          gridSize>,
               gridSize>;

// One state waiting to be expanded, with the steps taken to reach it.
struct SearchNode {
    WorldState state;
    int distance;
};

bool isSeen(const Visited& visited, const WorldState& state) {
    return visited[static_cast<std::size_t>(state.agent.y)]
                  [static_cast<std::size_t>(state.agent.x)]
                  [static_cast<std::size_t>(state.object.y)]
                  [static_cast<std::size_t>(state.object.x)];
}

void markSeen(Visited& visited, const WorldState& state) {
    visited[static_cast<std::size_t>(state.agent.y)]
           [static_cast<std::size_t>(state.agent.x)]
           [static_cast<std::size_t>(state.object.y)]
           [static_cast<std::size_t>(state.object.x)] = true;
}

}  // namespace

std::optional<int> shortestSolutionLength(const WorldState& world) {
    // An invalid world has no solution. This guard must come first: the
    // visited table below is indexed by agent and object positions, so
    // a position outside the grid would index outside the table.
    if (!isValidWorld(world)) {
        return std::nullopt;
    }
    if (world.object == world.destination) {
        return 0;
    }
    // Breadth-first search: states are expanded in order of increasing
    // distance, so the first goal state reached is a shortest solution.
    // Actions are tried in the order North, East, South, West (plan 3.2).
    Visited visited{};
    std::queue<SearchNode> queue;
    queue.push({world, 0});
    markSeen(visited, world);
    const std::array<Action, 4> order = {Action::North, Action::East,
                                         Action::South, Action::West};
    while (!queue.empty()) {
        const SearchNode node = queue.front();
        queue.pop();
        for (const Action action : order) {
            const ActionResult result = applyAction(node.state, action);
            if (result.outcome == MoveOutcome::Blocked ||
                result.outcome == MoveOutcome::InvalidState) {
                continue;
            }
            if (isSeen(visited, result.state)) {
                continue;
            }
            if (result.state.object == result.state.destination) {
                return node.distance + 1;
            }
            markSeen(visited, result.state);
            queue.push({result.state, node.distance + 1});
        }
    }
    return std::nullopt;
}

}  // namespace ai_hologram
