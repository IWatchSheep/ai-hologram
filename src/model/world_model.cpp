// Purpose: Implements budgeted predictions from a belief.
// Introduced in Stage 2.
// Architecture: Applies a replaceable action model without changing the belief.

#include "model/world_model.h"

namespace ai_hologram {

WorldModel::WorldModel()
    : actionModel_(applyAction) {
}

WorldModel::WorldModel(ActionModel actionModel)
    : actionModel_(actionModel) {
    // Calling an empty std::function would throw, so use the safe default instead.
    if (!actionModel_) {
        actionModel_ = applyAction;
    }
}

std::optional<Prediction> WorldModel::predict(
    const Belief& belief, const Action action, BudgetCounter& budget) const {
    if (!budget.tryUse()) {
        return std::nullopt;
    }

    const ActionResult result = actionModel_(belief.state, action);
    return Prediction{action, result.outcome, result.state};
}

}  // namespace ai_hologram
