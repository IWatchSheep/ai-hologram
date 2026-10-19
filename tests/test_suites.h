// Purpose: Declares each correctness-test group.
// Introduced in Stage 1.
// Architecture: test_main.cpp calls these groups and shares their counter.

#pragma once

#include "test_support.h"

namespace ai_hologram {

void runProjectInfoTests(TestCounter& counter);
void runWorldTextTests(TestCounter& counter);

}  // namespace ai_hologram
