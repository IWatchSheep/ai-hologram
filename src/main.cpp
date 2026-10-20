// Purpose: Starts the program and displays the scripted, prediction
// and agent demonstrations.
// Introduced in Stage 0.
// Architecture: The executable connects the shared core library to console output.
// main.cpp must stay short: it only connects components; real work belongs in the core library.

#include "core/project_info.h"
#include "demo/agent_demo.h"
#include "demo/prediction_demo.h"
#include "demo/scripted_demo.h"

#include <iostream>

int main() {
    // std::cout sends text to the console. '\n' moves output to the next line.
    std::cout << ai_hologram::projectName() << " -- Stage " << ai_hologram::currentStage() << '\n';
    std::cout << "Grid world with a competent single agent (C1) and scripted demonstrations.\n";
    std::cout << "No faults are injected, no two-agent reasoning exists, and no experiment is run.\n";
    ai_hologram::runScriptedDemo(std::cout);
    ai_hologram::runPredictionDemos(std::cout);
    ai_hologram::runAgentDemos(std::cout);
    // Returning 0 tells the operating system that the program succeeded.
    return 0;
}
