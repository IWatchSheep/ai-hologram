// Purpose: Implements the agent's action model: restored actions use
// the standard rule, all other actions use the base model (plan 3.8).
// Introduced in Stage 3.
// Architecture: current() only assembles an ActionModel. It makes no
// prediction itself, so it spends no budget; the StepPredictor
// charges one evaluation each time it calls the assembled model.

#include "agent/agent_model.h"
#include "world/rules.h"

namespace ai_hologram {
namespace {

// Turns an action into an index 0..3 (North, East, South, West), so
// the restored marks can live in a small fixed-size array.
int actionIndex(const Action action) {
    switch (action) {
        case Action::North: return 0;
        case Action::East: return 1;
        case Action::South: return 2;
        case Action::West: return 3;
    }
    return 0;
}

}  // namespace

AgentActionModel::AgentActionModel() : baseModel_(applyAction) {
}

AgentActionModel::AgentActionModel(ActionModel baseModel)
    : baseModel_(baseModel) {
    // An empty std::function would fail when called, so fall back to
    // the standard rule, as WorldModel itself does.
    if (!baseModel_) {
        baseModel_ = applyAction;
    }
}

void AgentActionModel::restore(const Action action) {
    restored_[static_cast<std::size_t>(actionIndex(action))] = true;
}

void AgentActionModel::unrestore(const Action action) {
    restored_[static_cast<std::size_t>(actionIndex(action))] = false;
}

bool AgentActionModel::isRestored(const Action action) const {
    return restored_[static_cast<std::size_t>(actionIndex(action))];
}

ActionModel AgentActionModel::current() const {
    // Copy the base model and the marks now, so later restore() or
    // unrestore() calls do not change a model already handed out.
    const ActionModel baseModel = baseModel_;
    const std::array<bool, 4> restored = restored_;
    return [baseModel, restored](const WorldState& state,
                                  const Action action) -> ActionResult {
        if (restored[static_cast<std::size_t>(actionIndex(action))]) {
            return applyAction(state, action);  // the standard rule
        }
        return baseModel(state, action);
    };
}

ActionModel AgentActionModel::base() const {
    return baseModel_;
}

}  // namespace ai_hologram
