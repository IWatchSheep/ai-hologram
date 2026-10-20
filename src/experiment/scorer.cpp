// Purpose: Implements the scorer's read-only views, the true-value
// test for claims, "active now" and the phase entry points
// (plan 3.7 and 3.8). The event life is in scorer_life.cpp and the
// committed-prediction shadow is in scorer_commit.cpp.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// scorer only READS an agent's public state; it never writes to it,
// and no agent file includes this header.

#include "experiment/scorer.h"

#include "agent/dead_squares.h"

#include <utility>

namespace ai_hologram {

ScoredView viewOf(const SingleAgentState& state) {
    ScoredView view;
    view.belief = &state.belief;
    view.model = &state.agentModel;
    view.notDeadOverrides = &state.notDeadOverrides;
    view.deadOverrides = &state.deadOverrides;
    view.corrections = &state.corrections;
    view.mismatches = &state.mismatches;
    view.committed = &state.committed;
    return view;
}

ScoredView viewOf(const NaiveAgentState& state) {
    ScoredView view;
    view.belief = &state.belief;
    view.model = &state.agentModel;
    view.notDeadOverrides = &state.notDeadOverrides;
    view.deadOverrides = &state.deadOverrides;
    return view;
}

bool claimValueIsTrue(const CorrectionRecord& record,
                      const WorldState& trueLayout,
                      const std::vector<Observation>& trueHistory,
                      const std::vector<MismatchRecord>& mismatches) {
    if (record.element.kind == ElementKind::Wall) {
        return record.newBlocked == isWall(trueLayout, record.element.cell);
    }
    if (record.element.kind == ElementKind::ActionModel) {
        // The only model correction is "the standard rule", which
        // is the true behaviour, so it is always true.
        return true;
    }
    if (record.element.kind == ElementKind::Trap) {
        FreeComputationCounts counts;
        const DeadSquareMap map =
            computeDeadSquares(trueLayout, {}, {}, counts);
        const Position cell = record.element.cell;
        if (!isInsideGrid(cell)) {
            return false;
        }
        return map.dead[static_cast<std::size_t>(cell.y)]
                        [static_cast<std::size_t>(cell.x)];
    }
    // Object (D-042): judged against the true object position just
    // before the investigated action. A mismatch recorded at step
    // s checks observation o_s, so the claim is about o_(s-1),
    // which is trueHistory[s - 2] because trueHistory[0] is o_1.
    if (mismatches.empty()) {
        return false;
    }
    const int step = mismatches.back().decisionStep;
    if (step < 2) {
        return false;
    }
    const std::size_t index = static_cast<std::size_t>(step - 2);
    if (index >= trueHistory.size()) {
        return false;
    }
    return record.newPosition == trueHistory[index].state.object;
}

Scorer::Scorer(std::vector<FaultEvent> events, WorldState trueLayout,
               const std::vector<Observation>* trueHistory)
    : events_(std::move(events)), trueLayout_(trueLayout),
      trueHistory_(trueHistory) {
    scores_.resize(events_.size());
}

const std::vector<EventScore>& Scorer::events() const {
    return scores_;
}

const RunScore& Scorer::run() const {
    return run_;
}

bool Scorer::activeNow(const ScoredView& view, const int index) const {
    const FaultEvent& event = events_[static_cast<std::size_t>(index)];
    if (event.type == FaultType::F1) {
        if (view.belief == nullptr) {
            return false;
        }
        return isWall(*view.belief, event.cell) !=
               isWall(trueLayout_, event.cell);
    }
    if (event.type == FaultType::F2 || event.type == FaultType::F5) {
        if (view.belief == nullptr) {
            return false;
        }
        if (trueHistory_ == nullptr || trueHistory_->empty()) {
            return false;
        }
        return view.belief->object != trueHistory_->back().state.object;
    }
    if (event.type == FaultType::F3) {
        if (view.model == nullptr) {
            return false;
        }
        return !view.model->isRestored(event.action);
    }
    // F4: active while the cell is not among the "dead" corrections.
    if (view.deadOverrides == nullptr) {
        return true;
    }
    for (const Position cell : *view.deadOverrides) {
        if (cell == event.cell) {
            return false;
        }
    }
    return true;
}

void Scorer::endPhaseA(const ScoredView& view, const int decisionStep) {
    currentStep_ = decisionStep;
    endPhase(view);
}

void Scorer::afterActivation(const ScoredView& view,
                             const std::vector<ActivationRecord>& records,
                             const int decisionStep) {
    currentStep_ = decisionStep;
    for (std::size_t i = 0; i < events_.size(); ++i) {
        if (i >= records.size()) {
            continue;
        }
        if (records[i].status != ActivationStatus::Applied) {
            continue;
        }
        if (scores_[i].activated) {
            continue;
        }
        scores_[i].activated = true;
        scores_[i].activationStep = decisionStep;
        scores_[i].active = true;
        if (!activeNow(view, static_cast<int>(i))) {
            // For example, F2 moved the believed object onto the
            // true one: the fault ended at once, with no record.
            scores_[i].active = false;
            scores_[i].endState = EndState::Overwritten;
            scores_[i].endStep = decisionStep;
        }
    }
}

void Scorer::endFinalCheck(const ScoredView& view) {
    // D-045: no new step; the current step is left unchanged.
    endPhase(view);
}

void Scorer::finish(const ScoredView& view) {
    if (view.corrections == nullptr) {
        return;
    }
    for (EventScore& score : scores_) {
        if (score.endState != EndState::Corrected) {
            continue;
        }
        if (score.correctingId == 0) {
            continue;
        }
        for (const CorrectionRecord& record : *view.corrections) {
            if (record.id == score.correctingId) {
                score.correctionStatus = record.status;
            }
        }
    }
}

}  // namespace ai_hologram
