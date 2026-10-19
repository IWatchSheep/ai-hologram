// Purpose: Declares the count of model evaluations available to predictions.
// Introduced in Stage 2.
// Architecture: WorldModel must use this counter before making each prediction.

#pragma once

namespace ai_hologram {

class BudgetCounter {
public:
    explicit BudgetCounter(int limit);

    // This is the only way a prediction may pay for its model evaluation.
    bool tryUse();
    int used() const;
    int remaining() const;
    void reset();

private:
    int limit_;
    int used_;
};

}  // namespace ai_hologram
