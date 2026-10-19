// Purpose: Declares the project's name and current stage for other files to use.
// Introduced in Stage 0.
// Architecture: This header is the interface to the shared core library.

// Prevents this header's contents from being processed more than once in one
// source file, avoiding duplicate declarations.
#pragma once

// #include makes declarations from another file available here. Angle brackets
// are used for standard-library headers; quotes are used for project headers.
#include <string>

// A namespace groups related names and helps prevent them from clashing with
// names used by other parts of a program.
namespace ai_hologram {

// Returns the project name.
std::string projectName();

// Returns the current project stage number.
int currentStage();

}  // namespace ai_hologram
