// Purpose: Implements the scorer's phase B and the shadow of each
// committed prediction (plan 3.9). Split from scorer.cpp to stay
// under about 200 lines, as the owner approved.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// scorer only READS an agent's public state; it never writes to it,
// and no agent file includes this header. A shadow repeats the
// committed prediction with one faulted element at its true value;
// it calls the model function itself, so no budget is used and no
// reasoner ever sees it.

#include "experiment/scorer.h"

#include "agent/footprint.h"
#include "world/rules.h"

namespace ai_hologram {

bool Scorer::committedEffective(const int index, const WorldState& pre,
                                const Action action,
                                const WorldState& resultState,
                                const MoveOutcome resultOutcome,
                                const ScoredView& view) const {
    const FaultEvent& event = events_[static_cast<std::size_t>(index)];
    if (event.type == FaultType::F4) {
        return false;
    }
    if (event.type == FaultType::F3) {
        if (action != event.action) {
            return false;
        }
        if (view.model == nullptr || view.model->isRestored(action)) {
            return false;
        }
        const ActionResult standard = applyAction(pre, action);
        return standard.outcome != resultOutcome ||
               standard.state != resultState;
    }
    WorldState shadow = pre;
    if (event.type == FaultType::F1) {
        const bool trueWall = isWall(trueLayout_, event.cell);
        if (isWall(pre, event.cell) == trueWall) {
            return false;
        }
        if (!isInsideGrid(event.cell)) {
            return false;
        }
        shadow.walls[static_cast<std::size_t>(event.cell.y)]
                    [static_cast<std::size_t>(event.cell.x)] = trueWall;
    } else {
        // F2 and F5: the shadow holds the true current object
        // position, but only when the prediction started from the
        // reasoner's current (wrong) believed object position.
        if (view.belief == nullptr) {
            return false;
        }
        if (trueHistory_ == nullptr || trueHistory_->empty()) {
            return false;
        }
        const Position trueObject = trueHistory_->back().state.object;
        if (view.belief->object == trueObject) {
            return false;
        }
        if (pre.object != view.belief->object) {
            return false;
        }
        shadow.object = trueObject;
    }
    if (!isValidWorld(shadow)) {
        return true;
    }
    if (view.model == nullptr) {
        return false;
    }
    const ActionResult shadowResult = view.model->current()(shadow, action);
    if (shadowResult.outcome != resultOutcome) {
        return true;
    }
    const std::vector<Position> cells = computeFootprintForTwoPredictions(
        pre, resultOutcome, shadow, shadowResult.outcome, action);
    const FootprintComparison compared = compareFootprint(
        resultState, resultOutcome, shadowResult.state,
        shadowResult.outcome, cells);
    return compared.outcomesDiffer || !compared.cells.empty();
}

void Scorer::afterDecide(const ScoredView& view, const Decision& decision) {
    // Phase B: everything recorded since the activation point is
    // dated to this phase, after any fault activated at this step.
    endPhase(view);
    if (!decision.isAction) {
        bool anyActive = false;
        for (const EventScore& score : scores_) {
            if (score.activated && score.endState == EndState::None &&
                score.active) {
                anyActive = true;
            }
        }
        if (anyActive) {
            ++run_.justifiedSafeStops;
        }
        return;
    }
    if (view.committed == nullptr || !view.committed->has) {
        return;
    }
    // A revealing opportunity is an effective use in a committed
    // prediction that was then checked. Every committed prediction
    // of C1 is checked, so the shadow is computed here, directly
    // from the belief at commitment, when decide returns.
    const WorldState& pre = view.committed->beliefBefore;
    const Action action = view.committed->prediction.action;
    const WorldState& resultState = view.committed->prediction.state;
    const MoveOutcome resultOutcome = view.committed->prediction.outcome;
    for (std::size_t i = 0; i < events_.size(); ++i) {
        if (!scores_[i].activated) {
            continue;
        }
        if (scores_[i].endState != EndState::None || !scores_[i].active) {
            continue;
        }
        if (!committedEffective(static_cast<int>(i), pre, action,
                                resultState, resultOutcome, view)) {
            continue;
        }
        committedCauses_.push_back(static_cast<int>(i));
        scores_[i].revealing = true;
        if (!scores_[i].opportunityStep.has_value()) {
            scores_[i].opportunityStep = currentStep_;
        }
    }
}

}  // namespace ai_hologram
