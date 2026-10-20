// Purpose: Implements tryApplyFault: how each of the five fault
// types is applied to one reasoner, with A3's checks (plan 3.4).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/, split
// off from fault_injector.cpp so each file stays under about 200
// lines. It changes only the reasoner parts it is handed: never the
// true world, and never any budget (applying a fault is free).

#include "experiment/fault_injector.h"
#include "world/rules.h"

#include <cstddef>
#include <optional>

namespace ai_hologram {
namespace {

// Turns an action into an index 0..3 (North, East, South, West).
int applyActionIndex(const Action action) {
    switch (action) {
        case Action::North: return 0;
        case Action::East: return 1;
        case Action::South: return 2;
        case Action::West: return 3;
    }
    return 0;
}

ApplyOutcome notApplied(const std::string& reason) {
    return ApplyOutcome{false, reason};
}

ApplyOutcome appliedFault(ReasonerParts parts) {
    // D-036: the belief or the model has changed, so cached
    // predictions may be wrong; the cache is emptied. The dead-square
    // cache needs no call: it recomputes by itself when the walls or
    // the overrides change.
    parts.predictor->clearCache();
    return ApplyOutcome{true, ""};
}

ApplyOutcome applyF1(const FaultEvent& event, ReasonerParts parts) {
    if (!event.hasCell) {
        return notApplied("no cell of the required kind");
    }
    if (event.direction == F1Direction::FreeToBlocked) {
        // A3 in plain words: a wall can never be put under the agent
        // or the object the reasoner believes are there, because such
        // a belief would be impossible.
        if (event.cell == parts.belief->agent) {
            return notApplied("the wall would be under the believed agent");
        }
        if (event.cell == parts.belief->object) {
            return notApplied("the wall would be under the believed object");
        }
    }
    if (isInsideGrid(event.cell)) {
        bool& wall = parts.belief->walls[static_cast<std::size_t>(event.cell.y)]
                                        [static_cast<std::size_t>(event.cell.x)];
        wall = !wall;
    }
    return appliedFault(parts);
}

ApplyOutcome applyF2(const FaultEvent& event, ReasonerParts parts) {
    for (const Action direction : event.order) {
        const Position cell = moved(parts.belief->object, direction);
        if (isWall(*parts.belief, cell)) {
            continue;
        }
        if (cell == parts.belief->agent) {
            continue;
        }
        parts.belief->object = cell;
        return appliedFault(parts);
    }
    return notApplied("no direction passes the test");
}

ApplyOutcome applyF3(const FaultEvent& event, ReasonerParts parts,
                     FaultedActions& faults) {
    // F3 is always applicable. Faults combine (D-035): this action's
    // entry is set in the table, the base model is rebuilt from the
    // whole table, and any earlier "restored" mark on this action is
    // removed, so a correction of another action keeps this fault.
    const std::size_t index =
        static_cast<std::size_t>(applyActionIndex(event.action));
    faults.faulted[index] = true;
    faults.rotation[index] = event.rotation;
    parts.model->setBase(makeFaultyModel(faults));
    parts.model->unrestore(event.action);
    return appliedFault(parts);
}

ApplyOutcome applyF4(const FaultEvent& event, ReasonerParts parts) {
    if (!event.hasCell) {
        return notApplied("the layout has no true dead square");
    }
    parts.notDeadOverrides->push_back(event.cell);
    return appliedFault(parts);
}

// The object's position just before its most recent move, read from
// the TRUE history: the earlier object of the last pair of consecutive
// observations whose object positions differ (D-034).
std::optional<Position> previousObject(
    const std::vector<Observation>& trueHistory) {
    if (trueHistory.size() < 2) {
        return std::nullopt;
    }
    for (std::size_t i = trueHistory.size() - 1; i >= 1; --i) {
        if (trueHistory[i].state.object != trueHistory[i - 1].state.object) {
            return trueHistory[i - 1].state.object;
        }
    }
    return std::nullopt;
}

ApplyOutcome applyF5(ReasonerParts parts,
                     const std::vector<Observation>& trueHistory) {
    const std::optional<Position> previous = previousObject(trueHistory);
    if (!previous.has_value()) {
        return notApplied("the object has not moved yet");
    }
    // A3: both checks use this reasoner's own belief.
    if (*previous == parts.belief->agent) {
        return notApplied("the previous square is the believed agent square");
    }
    if (isWall(*parts.belief, *previous)) {
        return notApplied("the previous square is believed to be a wall");
    }
    parts.belief->object = *previous;
    return appliedFault(parts);
}

}  // namespace

ApplyOutcome tryApplyFault(const FaultEvent& event, ReasonerParts parts,
                           FaultedActions& faults,
                           const std::vector<Observation>& trueHistory) {
    if (event.type == FaultType::F1) {
        return applyF1(event, parts);
    }
    if (event.type == FaultType::F2) {
        return applyF2(event, parts);
    }
    if (event.type == FaultType::F3) {
        return applyF3(event, parts, faults);
    }
    if (event.type == FaultType::F4) {
        return applyF4(event, parts);
    }
    return applyF5(parts, trueHistory);
}

}  // namespace ai_hologram
