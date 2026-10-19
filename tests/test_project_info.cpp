// Purpose: Checks the project information that was introduced in Stage 0.
// Introduced in Stage 1.
// Architecture: This group preserves the baseline checks in the shared runner.

#include "core/project_info.h"
#include "test_support.h"

#include <iostream>

namespace ai_hologram {

void runProjectInfoTests(TestCounter& counter) {
    std::cout << "--- Project information ---\n";
    check(counter, projectName() == "AI Hologram",
          "projectName() returns \"AI Hologram\"");
    check(counter, currentStage() == 1,
          "currentStage() returns 1");
}

}  // namespace ai_hologram
