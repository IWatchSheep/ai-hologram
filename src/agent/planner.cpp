// Purpose: Implements the A* search of plan 3.5.
// Introduced in Stage 3.
// Architecture: The search works only over the agent's own belief. It
// obtains every successor state from the StepPredictor, never from the
// rules directly, so every state it examines (apart from the root) has
// been paid for with exactly one budgeted model evaluation.

#include "agent/planner.h"
#include "agent/heuristic.h"

#include <array>
#include <cstddef>
#include <optional>
#include <queue>
#include <vector>

namespace ai_hologram {
namespace {

// One entry of the open list. g is the path length the entry was added
// with, so an older entry can be recognised as outdated after a shorter
// path to its state is found later.
struct QueueEntry {
    int f;           // f = g + h: estimated total plan length
    int h;           // heuristic of the state
    int entryNumber; // increases by one for every entry ever added
    int stateIndex;  // index into SearchResult::discovered
    int g;           // bestG at the moment this entry was added
};

// std::priority_queue is a standard container that always gives back
// the "largest" entry (according to this comparator) first. We want
// the SMALLEST (f, h, entry number) first, so the comparator answers
// "does `left` belong BEHIND `right`?": yes when left's f is larger, or
// f ties and left's h is larger, or both tie and left was added later.
// This is the documented tie-breaking order of plan 3.5; the entry
// number makes the order fully deterministic.
struct QueueEntryBehind {
    bool operator()(const QueueEntry& left, const QueueEntry& right) const {
        if (left.f != right.f) return left.f > right.f;
        if (left.h != right.h) return left.h > right.h;
        return left.entryNumber > right.entryNumber;
    }
};

using OpenList =
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, QueueEntryBehind>;
// The mutable bookkeeping of one running search, grouped so the helper
// functions below each stay short and take few parameters.
struct SearchState {
    SearchResult result;
    OpenList queue;
    std::vector<bool> closed;
    int nextEntryNumber = 0;
};

// Expansion always tries the actions in this fixed order (plan 3.5),
// which together with the queue order makes the search deterministic.
const std::array<Action, 4> kExpansionOrder = {
    Action::North, Action::East, Action::South, Action::West};

// Finds the already-discovered state with these positions, or -1. A
// state is identified by (agent position, object position) only.
int findDiscovered(const SearchResult& result, const WorldState& state) {
    for (std::size_t i = 0; i < result.discovered.size(); ++i) {
        const WorldState& known = result.discovered[i].state;
        if (known.agent == state.agent && known.object == state.object) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
bool objectOnDeadSquare(const WorldState& state, const DeadSquareMap& map) {
    return map.dead[static_cast<std::size_t>(state.object.y)]
                   [static_cast<std::size_t>(state.object.x)];
}

// Builds the plan from the root to discovered[stateIndex] by following
// parent links backwards and then reversing them into order.
std::vector<PlanStep> buildPlan(const SearchResult& result, int stateIndex) {
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
void addEntry(SearchState& search, const DiscoveredState& state, int index) {
    search.queue.push(QueueEntry{state.bestG + state.h, state.h,
                                 search.nextEntryNumber, index, state.bestG});
    ++search.nextEntryNumber;
}
// Records a newly discovered successor and queues it. h is computed
// once, here at discovery time (plan 3.5).
void discover(SearchState& search, const Prediction& prediction, int newG,
              int parentIndex, Action action,
              const std::function<int(const WorldState&)>& heuristic) {
    DiscoveredState fresh;
    fresh.state = prediction.state;
    fresh.bestG = newG;
    fresh.h = heuristic(prediction.state);
    fresh.parentIndex = parentIndex;
    fresh.action = action;
    fresh.discoveryNumber = static_cast<int>(search.result.discovered.size());
    search.result.discovered.push_back(fresh);
    search.closed.push_back(false);
    addEntry(search, fresh, static_cast<int>(search.result.discovered.size()) - 1);
}
// Improves an already-discovered successor reached by a shorter path:
// bestG, parent and action are replaced, a closed state is reopened,
// and a fresh queue entry is added (the older entries become outdated
// and are skipped when taken).
void improve(SearchState& search, int knownIndex, int newG, int parentIndex,
             Action action) {
    DiscoveredState& known =
        search.result.discovered[static_cast<std::size_t>(knownIndex)];
    ++search.result.improvedPaths;
    if (search.closed[static_cast<std::size_t>(knownIndex)]) {
        search.closed[static_cast<std::size_t>(knownIndex)] = false;
        ++search.result.reopenedStates;
    }
    known.bestG = newG;
    known.parentIndex = parentIndex;
    known.action = action;
    addEntry(search, known, knownIndex);
}
// Tries one action from the state being expanded. Returns false only
// when the search must stop with OutOfBudget.
bool trySuccessor(SearchState& search, const DeadSquareMap& map,
                  const std::function<int(const WorldState&)>& heuristic,
                  StepPredictor& predictor, int reserve, int currentIndex,
                  Action action) {
    const std::size_t current = static_cast<std::size_t>(currentIndex);
    const WorldState state = search.result.discovered[current].state;
    const int newG = search.result.discovered[current].bestG + 1;
    // The reserve protects the step's mandatory predictions (plan 3.4).
    // A cached prediction costs nothing, so only an uncached one must
    // prove it would still leave at least the reserve behind.
    if (!predictor.isCached(state, action) && predictor.remaining() <= reserve) {
        search.result.kind = SearchKind::OutOfBudget;
        return false;
    }
    // Every successor state comes from the predictor. Making one any
    // other way would be an uncharged evaluation, which protocol 10.7
    // forbids; this is why the planner never calls the rules itself.
    const std::optional<Prediction> prediction = predictor.predict(state, action);
    if (!prediction.has_value()) {
        search.result.kind = SearchKind::OutOfBudget;
        return false;
    }
    if (prediction->outcome == MoveOutcome::Blocked ||
        prediction->outcome == MoveOutcome::InvalidState ||
        objectOnDeadSquare(prediction->state, map)) {
        return true;  // skipped, but the search continues
    }
    const int knownIndex = findDiscovered(search.result, prediction->state);
    if (knownIndex >= 0 &&
        newG >= search.result.discovered[static_cast<std::size_t>(knownIndex)]
                    .bestG) {
        return true;  // no better than the path already known
    }
    if (knownIndex >= 0) {
        improve(search, knownIndex, newG, currentIndex, action);
    } else {
        discover(search, *prediction, newG, currentIndex, action, heuristic);
    }
    return true;
}
// Closes and expands one taken state, N, E, S, W. Returns false only
// when the search must stop with OutOfBudget.
bool expand(SearchState& search, const DeadSquareMap& map,
            const std::function<int(const WorldState&)>& heuristic,
            StepPredictor& predictor, int reserve, int currentIndex) {
    search.closed[static_cast<std::size_t>(currentIndex)] = true;
    for (const Action action : kExpansionOrder) {
        if (!trySuccessor(search, map, heuristic, predictor, reserve,
                          currentIndex, action)) {
            return false;
        }
    }
    return true;
}
}  // namespace

SearchResult search(const WorldState& root, const DeadSquareMap& map,
                    const std::function<bool(const WorldState&)>& goalTest,
                    const std::function<int(const WorldState&)>& heuristic,
                    StepPredictor& predictor, const int reserve) {
    SearchState search;
    // Step 1: discover the root (g = 0, discovery number 0) and queue it.
    search.result.discovered.push_back(
        DiscoveredState{root, 0, heuristic(root), -1, Action::North, 0});
    search.closed.push_back(false);
    addEntry(search, search.result.discovered[0], 0);

    while (!search.queue.empty()) {
        const QueueEntry entry = search.queue.top();
        search.queue.pop();
        const std::size_t index = static_cast<std::size_t>(entry.stateIndex);
        // Step 2: skip outdated entries (a shorter path was found after
        // this entry was added, or the state was already expanded).
        if (entry.g != search.result.discovered[index].bestG ||
            search.closed[index]) {
            continue;
        }
        // Step 3: the goal is tested when a state is TAKEN from the
        // queue, not when it is generated. The root is taken first, so
        // an already-solved root is Found with an empty plan and no
        // evaluations at all.
        if (goalTest(search.result.discovered[index].state)) {
            search.result.kind = SearchKind::Found;
            search.result.plan = buildPlan(search.result, entry.stateIndex);
            return search.result;
        }
        // Step 4: close the state and expand it.
        if (!expand(search, map, heuristic, predictor, reserve,
                    entry.stateIndex)) {
            return search.result;  // OutOfBudget, set by trySuccessor
        }
    }

    // Step 5: nothing left to take from the queue.
    search.result.kind = SearchKind::Exhausted;
    return search.result;
}

}  // namespace ai_hologram
