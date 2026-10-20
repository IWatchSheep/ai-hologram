// Purpose: Implements the verification checks of plan 3.8: V1
// history checks and model replays, the trap V1 check and V2 test,
// V2 alternative predictions and active-test targets.
// Introduced in Stage 3.
// Architecture: Every prediction here is a variant prediction made
// through the StepPredictor (see verification.h for which model
// each one uses), so every successor state is paid for from the
// step budget. These functions never change a belief themselves.

#include "agent/verification.h"
#include "world/rules.h"

#include <array>
#include <cstddef>

namespace ai_hologram {
namespace {

const std::array<Action, 4> kDirections = {Action::North, Action::East,
                                           Action::South, Action::West};

// The history check shared by plan 3.7 B.3 and trap V1: no recorded
// observation shows the object at p with a LATER recorded
// observation showing it on the destination. If that happened, the
// object demonstrably escaped p, so p cannot be claimed dead.
bool trapHistoryPasses(Position p, Position destination,
                       const std::vector<Observation>& history) {
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history[i].state.object != p) continue;
        for (std::size_t j = i + 1; j < history.size(); ++j) {
            if (history[j].state.object == destination) return false;
        }
    }
    return true;
}

bool isDeadAt(const DeadSquareMap& map, Position cell) {
    return map.dead[static_cast<std::size_t>(cell.y)]
                   [static_cast<std::size_t>(cell.x)];
}

}  // namespace

bool wallHistoryCheck(const Position cell, const bool blocked,
                      const std::vector<Observation>& history) {
    for (const Observation& entry : history) {
        if (isWall(entry.state, cell) != blocked) return false;
    }
    return true;
}

bool objectHistoryCheck(const Position value, const Observation& before) {
    // Only o_t-1 is compared. The object moves, so its position in
    // any earlier observation is old news, never a contradiction.
    return before.state.object == value;
}

ModelV1Work startModelV1(const Action action,
                         const std::vector<Observation>& history) {
    ModelV1Work work;
    work.action = action;
    for (std::size_t k = 0; k < history.size(); ++k) {
        if (history[k].lastAction.has_value() &&
            *history[k].lastAction == action) {
            work.pendingIndices.push_back(static_cast<int>(k));
        }
    }
    return work;
}

V1Progress continueModelV1(ModelV1Work& work,
                           const std::vector<Observation>& history,
                           StepPredictor& predictor, const int reserve) {
    while (!work.pendingIndices.empty()) {
        // Replays are optional evaluations: never break the reserve.
        if (predictor.remaining() <= reserve) return V1Progress::Pending;
        const int k = work.pendingIndices.front();
        if (k < 1 || static_cast<std::size_t>(k) >= history.size()) {
            return V1Progress::Failed;
        }
        const Observation& recorded = history[static_cast<std::size_t>(k)];
        // The STANDARD rule, passed as the variant model. The agent's
        // current model is deliberately never used for these replays.
        const auto replay = predictor.predictVariant(
            history[static_cast<std::size_t>(k - 1)].state, work.action,
            ActionModel{&applyAction});
        if (!replay.has_value()) return V1Progress::Pending;
        if (replay->state != recorded.state ||
            !recorded.lastOutcome.has_value() ||
            replay->outcome != *recorded.lastOutcome) {
            return V1Progress::Failed;
        }
        work.pendingIndices.erase(work.pendingIndices.begin());
    }
    return V1Progress::Done;
}

bool trapV1Check(const WorldState& belief, const DeadSquareMap& map,
                 const Position p, const std::vector<Observation>& history,
                 FreeComputationCounts& counts) {
    if (localCheckSaysNotDead(belief, map, p, counts)) return false;
    return trapHistoryPasses(p, belief.destination, history);
}

TrapV2Result trapV2Test(const WorldState& belief, const DeadSquareMap& map,
                        const Position p, StepPredictor& predictor,
                        const ActionModel& currentModel, const int reserve,
                        int& anomalies) {
    // Build every hypothetical state first, so the whole test can
    // be refused (Waiting) before any evaluation is spent.
    std::vector<std::pair<WorldState, Action>> pushes;
    for (const Action direction : kDirections) {
        const Position offset = offsetFor(direction);
        const Position stand{p.x - offset.x, p.y - offset.y};
        if (isWall(belief, stand)) continue;  // agent cannot stand there
        WorldState hypothetical = belief;
        hypothetical.agent = stand;
        hypothetical.object = p;
        if (!isValidWorld(hypothetical)) {
            ++anomalies;  // skipped and logged as an anomaly by plan 3.8
            continue;
        }
        pushes.push_back({hypothetical, direction});
    }
    const int k = static_cast<int>(pushes.size());
    if (predictor.remaining() - k < reserve) return TrapV2Result::Waiting;
    bool rejected = false;
    for (const auto& push : pushes) {
        const auto predicted = predictor.predictVariant(
            push.first, push.second, currentModel);  // the CURRENT model
        if (!predicted.has_value()) return TrapV2Result::Waiting;
        const Position movedTo = predicted->state.object;
        if (movedTo != p && isInsideGrid(movedTo) && !isDeadAt(map, movedTo)) {
            rejected = true;  // a push escapes to a cell that is not dead
        }
    }
    return rejected ? TrapV2Result::Rejected : TrapV2Result::Verified;
}

std::optional<Prediction> alternativePrediction(
    const CorrectionRecord& correction, const WorldState& belief,
    const Action action, StepPredictor& predictor,
    const ActionModel& currentModel, const ActionModel& oldModel) {
    const Position c1 = moved(belief.agent, action);
    const Position c2 = moved(c1, action);
    const Element& element = correction.element;
    if (element.kind == ElementKind::ActionModel) {
        if (element.action != action) return std::nullopt;
        if (!isValidWorld(belief)) return std::nullopt;
        // A model alternative predicts the same belief with the OLD model.
        return predictor.predictVariant(belief, action, oldModel);
    }
    if (element.kind == ElementKind::Trap) return std::nullopt;
    WorldState alternative = belief;
    if (element.kind == ElementKind::Wall) {
        if (element.cell != c1 && element.cell != c2) return std::nullopt;
        if (!isInsideGrid(element.cell)) return std::nullopt;
        alternative.walls[static_cast<std::size_t>(element.cell.y)]
                         [static_cast<std::size_t>(element.cell.x)] =
                             correction.oldBlocked;
    } else {  // Object: relevant if its old or believed position is c1/c2.
        if (correction.oldPosition != c1 && correction.oldPosition != c2 &&
            belief.object != c1 && belief.object != c2) {
            return std::nullopt;
        }
        alternative.object = correction.oldPosition;
    }
    // An alternative belief that is not a valid world is never predicted.
    if (!isValidWorld(alternative)) return std::nullopt;
    // Wall and object alternatives use the CURRENT model (plan 3.8).
    return predictor.predictVariant(alternative, action, currentModel);
}

bool predictionsDiffer(const Prediction& first, const Prediction& second) {
    return first.outcome != second.outcome ||
           first.state.agent != second.state.agent ||
           first.state.object != second.state.object;
}

std::vector<std::pair<Position, Action>> activeTestTargets(
    const WorldState& belief, const Position cell) {
    std::vector<std::pair<Position, Action>> targets;
    for (const Action direction : kDirections) {
        const Position offset = offsetFor(direction);
        const Position stand{cell.x - offset.x, cell.y - offset.y};
        if (!isWall(belief, stand) && stand != belief.object) {
            targets.push_back({stand, direction});
        }
    }
    return targets;
}

}  // namespace ai_hologram
