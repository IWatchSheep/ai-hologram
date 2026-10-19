// Purpose: Declares the shared counter and helper used by correctness tests.
// Introduced in Stage 1.
// Architecture: Test groups share this helper to report one accurate summary.

#pragma once

#include <string>

namespace ai_hologram {

struct TestCounter {
    int checks = 0;
    int failures = 0;
};

// Checks one condition and records its result in the shared counter.
void check(TestCounter& counter, bool condition, const std::string& description);

}  // namespace ai_hologram
