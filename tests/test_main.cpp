// Purpose: Checks that the shared core functions return their expected values.
// Introduced in Stage 0.
// Architecture: The test executable uses the same core library as the main program.
//
// These are correctness tests: they check that code behaves as expected.
// They are not research measurements, which record experimental outcomes.
// Returning 0 tells CTest every check passed; returning a nonzero value tells
// CTest at least one check failed.

#include "core/project_info.h"

#include <iostream>
#include <string>

// Prints the result of one check and returns 1 only when that check failed.
// static limits this function to this .cpp file, so other .cpp files cannot call it.
// const prevents changing the description; & passes it without making a string copy.
static int check(const bool condition, const std::string& description) {
    if (condition) {
        std::cout << "PASS: " << description << '\n';
        return 0;
    }

    std::cout << "FAIL: " << description << '\n';
    return 1;
}

int main() {
    int failures = 0;

    // Check the actual functions from the shared core library.
    failures += check(ai_hologram::projectName() == "AI Hologram",
                      "projectName() returns \"AI Hologram\"");
    failures += check(ai_hologram::currentStage() == 0,
                      "currentStage() returns 0");

    // constexpr means this value is fixed and known at compile time.
    constexpr int checks = 2;
    std::cout << checks << " checks, " << failures << " failures\n";

    // CTest treats 0 as success and any nonzero return value as failure.
    // The ? : operator selects 0 when the condition is true, or 1 when it is false.
    return failures == 0 ? 0 : 1;
}
