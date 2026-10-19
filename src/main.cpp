// Purpose: Starts the program and displays the scripted demonstration.
// Introduced in Stage 0.
// Architecture: The executable connects the shared core library to console output.
// main.cpp must stay short: it only connects components; real work belongs in the core library.

#include "core/project_info.h"
#include "demo/prediction_demo.h"
#include "demo/scripted_demo.h"

#include <iostream>

int main() {
    // std::cout sends text to the console. '\n' moves output to the next line.
    std::cout << ai_hologram::projectName() << " -- Stage " << ai_hologram::currentStage() << '\n';
    std::cout << "Grid world with prediction and comparison (scripted demonstrations).\n";
    std::cout << "No reasoning components are implemented yet.\n";
    ai_hologram::runScriptedDemo(std::cout);
    ai_hologram::runPredictionDemos(std::cout);
    // Returning 0 tells the operating system that the program succeeded.
    return 0;
}
