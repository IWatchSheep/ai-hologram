// Purpose: Declares the per-step predictor: the single route through
// which every model evaluation of a reasoner is charged and cached.
// Introduced in Stage 3.
// Architecture: StepPredictor owns one WorldModel and one BudgetCounter
// that is recreated at the start of every decision step (plan 3.4). The
// planner and all later checks obtain successor states only through it,
// so no successor state is ever created "for free" (protocol 10.7).

#pragma once

#include "model/budget_counter.h"
#include "model/prediction.h"
#include "model/world_model.h"
#include "world/action.h"
#include "world/world_state.h"

#include <array>
#include <optional>
#include <vector>

namespace ai_hologram {

// All model evaluations of a reasoner go through this one class.
//
// The cache: within one decision step, repeating exactly the same
// prediction (same agent position, object position and action, from the
// same believed walls and destination, with the same model) is free,
// because protocol 10.7 allows reuse within the same step. The key must
// remember the walls and destination the entries belong to: the same
// positions and action can give a different result when a wall differs,
// so a prediction made with different walls or destination first clears
// the cache instead of reusing a result computed under other walls.
//
// A variant prediction is a prediction from a different belief (such as
// a hypothetical state) or with a different model (such as the standard
// rule instead of the agent's current model). Variants are never read
// from the cache and never stored in it, because they do not describe
// what the current belief and model predict; each one always costs
// exactly one evaluation from the same step budget.
class StepPredictor {
public:
    // The model is the reasoner's own WorldModel; budgetPerStep is B,
    // the maximum number of evaluations charged in one decision step.
    StepPredictor(WorldModel model, int budgetPerStep);

    // Starts a new decision step: a fresh BudgetCounter with limit B
    // (unused budget from the last step is lost) and an empty cache.
    void startStep();

    // Predicts `action` from `state` with the reasoner's own model.
    // A cache hit is free; a miss costs exactly one evaluation and is
    // stored in the cache. When the budget cannot pay for a miss, this
    // returns no prediction and changes nothing (nothing is cached).
    std::optional<Prediction> predict(const WorldState& state, Action action);

    // True when predict(state, action) would be served from the cache,
    // including the walls and destination the cache belongs to.
    bool isCached(const WorldState& state, Action action) const;

    // Predicts with `model` instead of the reasoner's own model. Never
    // reads or fills the cache, and costs exactly one evaluation from
    // the same step budget whenever the budget can pay for it.
    std::optional<Prediction> predictVariant(const WorldState& state,
                                             Action action,
                                             const ActionModel& model);

    // Empties the cache, for when the model or the trap overrides change
    // (the cached results may no longer match what would be predicted).
    void clearCache();

    int remaining() const;    // evaluations still available this step
    int usedThisStep() const; // evaluations charged in this step
    int usedTotal() const;    // evaluations charged over all steps

private:
    struct CacheEntry {
        Position agent{};
        Position object{};
        Action action = Action::North;
        Prediction prediction{};
    };

    // Finds a cached entry for this key, or nullptr. The caller must
    // already have made sure the cache belongs to state's walls.
    const CacheEntry* findCached(const WorldState& state, Action action) const;

    WorldModel model_;
    int budgetPerStep_;
    BudgetCounter budget_;
    std::vector<CacheEntry> cache_;

    // The walls and destination the cached entries were computed under.
    // They are part of the cache key in effect: a state whose walls or
    // destination differ makes every cached entry meaningless.
    bool hasCacheContext_ = false;
    std::array<std::array<bool, gridSize>, gridSize> cacheWalls_{};
    Position cacheDestination_{0, 0};

    int usedTotal_ = 0;
};

}  // namespace ai_hologram
