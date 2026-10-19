// Purpose: Implements the model-evaluation budget counter.
// Introduced in Stage 2.
// Architecture: WorldModel consults this component before predicting.

#include "model/budget_counter.h"

namespace ai_hologram {

BudgetCounter::BudgetCounter(const int limit)
    : limit_(limit < 0 ? 0 : limit), used_(0) {
}

bool BudgetCounter::tryUse() {
    if (used_ >= limit_) {
        return false;
    }

    ++used_;
    return true;
}

int BudgetCounter::used() const {
    return used_;
}

int BudgetCounter::remaining() const {
    return limit_ - used_;
}

void BudgetCounter::reset() {
    used_ = 0;
}

}  // namespace ai_hologram
