// Purpose: Declares the model that predicts actions from a belief.
// Introduced in Stage 2.
// Architecture: Predictions use Stage 1 rules by default and always consume budget.

#pragma once

#include "model/belief.h"
#include "model/budget_counter.h"
#include "model/prediction.h"
#include "world/action.h"
#include "world/rules.h"

#include <functional>
#include <optional>

namespace ai_hologram {

// std::function stores a callable; this alias names the action-model signature.
// It lets tests supply another model without introducing a class hierarchy.
using ActionModel = std::function<ActionResult(const WorldState&, Action)>;

class WorldModel {
public:
    WorldModel();
    explicit WorldModel(ActionModel actionModel);

    // This const model still updates the caller's budget through the reference.
    // An empty optional means the budget did not allow a prediction.
    std::optional<Prediction> predict(const Belief& belief, Action action,
                                      BudgetCounter& budget) const;

private:
    ActionModel actionModel_;
};

}  // namespace ai_hologram
