// Purpose: Implements the scorer's watch-only observer callbacks:
// shadow evaluation of every observed evaluation, dead-status read
// and heuristic value, recording detection opportunities (plan 3.9).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/, split
// from scorer.cpp so each file stays under about 200 lines. The
// observer can only watch ("the chain can be replaced by the
// camera", and ours cannot pull): these callbacks receive only
// const data, return nothing to any agent, and only ever write the
// scorer's own opportunityStep fields. A shadow computation repeats
// the watched computation with one faulted element at its true
// value; it never uses the reasoner's budget (every counter here is
// a local FreeComputationCounts) and is never visible to an agent.
// Trap-status READS are observed, not whole analysis runs (D-040),
// so a read is compared cell by cell against the shadow map.

#include "experiment/scorer.h"

#include "agent/heuristic.h"
#include "experiment/shadow.h"

#include <cstddef>

namespace ai_hologram {
namespace {

// True when an event may still gain an opportunity step.
bool awaitingOpportunity(const EventScore& score) {
    if (!score.activated) {
        return false;
    }
    if (score.endState != EndState::None || !score.active) {
        return false;
    }
    return !score.opportunityStep.has_value();
}

}  // namespace

void Scorer::attachView(const ScoredView& view) {
    liveView_ = view;
    hasLiveView_ = true;
}

int Scorer::observedEvaluations() const {
    return observedEvaluations_;
}

void Scorer::onEvaluation(const WorldState& state, const Action action,
                          const ActionModel* variantModel,
                          const Prediction& result) {
    ++observedEvaluations_;
    if (!hasLiveView_ || liveView_.model == nullptr) {
        return;
    }
    if (trueHistory_ == nullptr || trueHistory_->empty()) {
        return;
    }
    const Position trueObject = trueHistory_->back().state.object;
    ActionModel modelUsed = liveView_.model->current();
    if (variantModel != nullptr) {
        modelUsed = *variantModel;
    }
    for (std::size_t i = 0; i < events_.size(); ++i) {
        if (!awaitingOpportunity(scores_[i])) {
            continue;
        }
        if (evaluationEffective(events_[i], state, action, modelUsed,
                                result, liveView_, trueLayout_, trueObject)) {
            scores_[i].opportunityStep = currentStep_;
        }
    }
}

void Scorer::onDeadRead(const Position cell, const bool value) {
    if (!hasLiveView_ || !isInsideGrid(cell)) {
        return;
    }
    for (std::size_t i = 0; i < events_.size(); ++i) {
        if (!awaitingOpportunity(scores_[i])) {
            continue;
        }
        const FaultEvent& event = events_[i];
        if (event.type != FaultType::F1 && event.type != FaultType::F4) {
            continue;
        }
        const std::optional<DeadSquareMap> shadow =
            shadowMap(event, liveView_, trueLayout_);
        if (!shadow.has_value()) {
            continue;
        }
        const std::size_t row = static_cast<std::size_t>(cell.y);
        const std::size_t column = static_cast<std::size_t>(cell.x);
        if (shadow->dead[row][column] != value) {
            scores_[i].opportunityStep = currentStep_;
        }
    }
}

void Scorer::onHeuristic(const WorldState& state, const int value) {
    if (!hasLiveView_ || liveView_.belief == nullptr) {
        return;
    }
    FreeComputationCounts localCounts;
    for (std::size_t i = 0; i < events_.size(); ++i) {
        if (!awaitingOpportunity(scores_[i])) {
            continue;
        }
        const FaultEvent& event = events_[i];
        if (event.type == FaultType::F1 || event.type == FaultType::F4) {
            const std::optional<DeadSquareMap> shadow =
                shadowMap(event, liveView_, trueLayout_);
            if (shadow.has_value() &&
                heuristic(state, *shadow, localCounts) != value) {
                scores_[i].opportunityStep = currentStep_;
            }
            continue;
        }
        if (event.type != FaultType::F2 && event.type != FaultType::F5) {
            continue;
        }
        if (trueHistory_ == nullptr || trueHistory_->empty()) {
            continue;
        }
        const Position trueObject = trueHistory_->back().state.object;
        const std::optional<WorldState> shadow =
            shadowInput(event, state, liveView_, trueLayout_, trueObject);
        if (!shadow.has_value()) {
            continue;
        }
        if (!isValidWorld(*shadow)) {
            scores_[i].opportunityStep = currentStep_;
            continue;
        }
        if (liveView_.notDeadOverrides == nullptr ||
            liveView_.deadOverrides == nullptr) {
            continue;
        }
        const DeadSquareMap currentMap = computeDeadSquares(
            *liveView_.belief, *liveView_.notDeadOverrides,
            *liveView_.deadOverrides, localCounts);
        if (heuristic(*shadow, currentMap, localCounts) != value) {
            scores_[i].opportunityStep = currentStep_;
        }
    }
}

}  // namespace ai_hologram
