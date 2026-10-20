// Purpose: Defines the project information functions declared in the header.
// Introduced in Stage 0.
// Architecture: Implements part of the shared core library used by programs.

#include "core/project_info.h"

namespace ai_hologram {

// Keeping declarations in the header lets multiple files use this code while compiling its implementation once.
std::string projectName() {
    return "AI Hologram";
}

int currentStage() {
    return 4;
}

}  // namespace ai_hologram
