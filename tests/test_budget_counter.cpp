// Purpose: Checks the budget counter's limits and reset behavior.
// Introduced in Stage 2.
// Architecture: These tests verify the shared cost-control component.

#include "model/budget_counter.h"
#include "test_suites.h"

#include <iostream>

namespace ai_hologram {
namespace {

void checkInitialValues(TestCounter& counter) {
    const BudgetCounter budget(3);
    check(counter, budget.used() == 0 && budget.remaining() == 3,
          "BC1: A new counter has no used evaluations and its full limit remains");
}

void checkLimit(TestCounter& counter) {
    BudgetCounter budget(3);
    const bool first = budget.tryUse();
    const bool second = budget.tryUse();
    const bool third = budget.tryUse();
    const bool afterLimit = budget.tryUse();
    check(counter, first && second && third && !afterLimit &&
                       budget.used() == 3 && budget.remaining() == 0,
          "BC2: Use succeeds up to the limit and then leaves the count unchanged");
}

void checkReset(TestCounter& counter) {
    BudgetCounter budget(1);
    budget.tryUse();
    budget.reset();
    check(counter, budget.used() == 0 && budget.remaining() == 1 &&
                       budget.tryUse() && budget.used() == 1,
          "BC3: Reset restores the counter and allows another use");
}

void checkZeroAndNegativeLimits(TestCounter& counter) {
    BudgetCounter zeroBudget(0);
    BudgetCounter negativeBudget(-2);
    check(counter, !zeroBudget.tryUse() && zeroBudget.used() == 0 &&
                       zeroBudget.remaining() == 0 &&
                       !negativeBudget.tryUse() && negativeBudget.used() == 0 &&
                       negativeBudget.remaining() == 0,
          "BC4: Zero and negative limits do not allow use");
}

}  // namespace

void runBudgetCounterTests(TestCounter& counter) {
    std::cout << "--- Budget counter ---\n";
    checkInitialValues(counter);
    checkLimit(counter);
    checkReset(counter);
    checkZeroAndNegativeLimits(counter);
}

}  // namespace ai_hologram
