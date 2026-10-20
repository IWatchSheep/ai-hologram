// Purpose: Declares the agent's own action model: a base model plus
// the actions that have been restored to the standard rule (plan 3.8).
// Introduced in Stage 3.
// Architecture: A model(a) correction never rewrites the base model.
// It only marks action `a` as restored, and current() builds the
// ActionModel the StepPredictor should use from then on. Building
// that model evaluates nothing; evaluations happen only when the
// StepPredictor calls the model and charges the step budget.
// Stage 4 IE-1 H2 adds setBase below (used only by the fault injector).

#pragma once

#include "model/world_model.h"
#include "world/action.h"

#include <array>

namespace ai_hologram {

// The agent's action model (plan 3.8, "The agent's action model").
// In Experiment 1 the world rules are fully known, so the only
// candidate correction of model(a) is "use the standard rule for a".
class AgentActionModel {
public:
    // With no base model given, the base model is the standard rule.
    AgentActionModel();
    explicit AgentActionModel(ActionModel baseModel);

    // Marks `action` as restored to the standard rule (the applying
    // of a model(action) correction), or removes that mark again
    // (reverting a correction that was later REJECTED).
    void restore(Action action);
    void unrestore(Action action);
    bool isRestored(Action action) const;

    // Replaces the base model and keeps the restored marks. Used only
    // by the experimenter's fault injector to install a faulty action
    // model (F3, plan 3.4); agent code never calls it.
    void setBase(ActionModel model);

    // The model to predict with now: the standard rule for restored
    // actions, the base model for all others. The returned model is
    // a snapshot of the restored marks as they are at this call.
    ActionModel current() const;

    // The model in use before any restoration. This is the "old
    // model" of a model(a) correction, used for its alternative
    // prediction in V2 bookkeeping (plan 3.8).
    ActionModel base() const;

private:
    ActionModel baseModel_;
    // One mark per action, in the order North, East, South, West.
    std::array<bool, 4> restored_{};
};

}  // namespace ai_hologram
