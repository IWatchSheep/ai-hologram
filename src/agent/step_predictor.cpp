// Purpose: Implements the per-step budget, prediction cache and counts.
// Introduced in Stage 3.
// Architecture: Every successor state in Stage 3 comes from a budgeted
// model evaluation made here (plan 3.4, protocol 10.7). This class is
// the only place that calls WorldModel::predict for the reasoner, so
// the number of model calls always equals the charged evaluations.
// Stage 4 IE-1 H3 addition: charged evaluations are reported to a watch-only observer.

#include "agent/step_predictor.h"

#include <utility>

namespace ai_hologram {

StepPredictor::StepPredictor(WorldModel model, const int budgetPerStep)
    : model_(std::move(model)),
      budgetPerStep_(budgetPerStep),
      budget_(budgetPerStep) {
}

void StepPredictor::startStep() {
    // A brand-new counter: last step's unused budget is lost, not saved.
    budget_ = BudgetCounter(budgetPerStep_);
    clearCache();
}

void StepPredictor::setObserver(const ComputationObserver* observer) {
    observer_ = observer;
}

void StepPredictor::clearCache() {
    cache_.clear();
    hasCacheContext_ = false;
}

const StepPredictor::CacheEntry* StepPredictor::findCached(
    const WorldState& state, const Action action) const {
    for (const CacheEntry& entry : cache_) {
        if (entry.agent == state.agent && entry.object == state.object &&
            entry.action == action) {
            return &entry;
        }
    }
    return nullptr;
}

bool StepPredictor::isCached(const WorldState& state,
                             const Action action) const {
    // Entries computed under other walls or another destination do not
    // count as cached for this state (they will be cleared on predict).
    if (!hasCacheContext_ || state.walls != cacheWalls_ ||
        state.destination != cacheDestination_) {
        return false;
    }
    return findCached(state, action) != nullptr;
}

std::optional<Prediction> StepPredictor::predict(const WorldState& state,
                                                 const Action action) {
    // The cache belongs to one set of walls and one destination. Other
    // walls can change every result, so they clear the cache first.
    if (hasCacheContext_ &&
        (state.walls != cacheWalls_ || state.destination != cacheDestination_)) {
        clearCache();
    }
    if (const CacheEntry* entry = findCached(state, action)) {
        return entry->prediction;  // a cache hit is free (protocol 10.7)
    }

    // A miss costs exactly one evaluation. WorldModel returns nothing
    // and charges nothing when the budget is exhausted; in that case
    // this predictor likewise changes nothing and caches nothing.
    const std::optional<Prediction> result =
        model_.predict(Belief{state}, action, budget_);
    if (!result.has_value()) {
        return std::nullopt;
    }
    ++usedTotal_;
    if (!hasCacheContext_) {
        cacheWalls_ = state.walls;
        cacheDestination_ = state.destination;
        hasCacheContext_ = true;
    }
    cache_.push_back(CacheEntry{state.agent, state.object, action, *result});
    if (observer_ && observer_->onEvaluation) {
        observer_->onEvaluation(state, action, nullptr, *result);
    }
    return result;
}

std::optional<Prediction> StepPredictor::predictVariant(
    const WorldState& state, const Action action, const ActionModel& model) {
    // A variant prediction uses a different model (or a hypothetical
    // state), so its result must never be reused as if the reasoner's
    // own model had made it: the cache is neither read nor filled.
    // It still pays exactly one evaluation from the same step budget,
    // through a WorldModel built from the given model.
    const WorldModel variantModel(model);
    const std::optional<Prediction> result =
        variantModel.predict(Belief{state}, action, budget_);
    if (!result.has_value()) {
        return std::nullopt;
    }
    ++usedTotal_;
    if (observer_ && observer_->onEvaluation) {
        observer_->onEvaluation(state, action, &model, *result);
    }
    return result;
}

int StepPredictor::remaining() const {
    return budget_.remaining();
}

int StepPredictor::usedThisStep() const {
    return budget_.used();
}

int StepPredictor::usedTotal() const {
    return usedTotal_;
}

}  // namespace ai_hologram
