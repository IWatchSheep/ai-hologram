// Purpose: Runs and prints the fixed scripted demonstration.
// Introduced in Stage 1.
// Architecture: The demo connects parsed layouts, environment rules, and text output.
// The actions are hand-written; nothing chooses them.

#include "demo/scripted_demo.h"

#include "world/action.h"
#include "world/environment.h"
#include "world/rules.h"
#include "world/world_text.h"

#include <array>
#include <ostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

void printWorld(std::ostream& out, const WorldState& state) {
    for (const std::string& line : renderWorld(state)) {
        out << line << '\n';
    }
}

void printSteps(
    std::ostream& out,
    Environment& environment,
    const std::array<Action, 5>& actions) {
    out << '\n';
    int stepNumber = 1;
    for (const Action action : actions) {
        const ActionResult result = environment.step(action);
        out << "Step " << stepNumber << ": " << actionName(action)
            << " -> " << outcomeName(result.outcome) << '\n';
        ++stepNumber;
    }
}

}  // namespace

void runScriptedDemo(std::ostream& out) {
    const std::vector<std::string> layout{
        "##########",
        "#........#",
        "#........#",
        "#..A.O..D#",
        "#........#",
        "#........#",
        "#...##...#",
        "#........#",
        "#........#",
        "##########"
    };
    const ParseResult parsed = parseWorld(layout);
    if (!parsed.world) {
        out << "Demo layout is invalid: " << parsed.error << '\n';
        return;
    }

    Environment environment(*parsed.world);
    const std::array<Action, 5> actions{
        Action::East, Action::East, Action::East, Action::East, Action::East
    };

    out << "\nInitial world:\n";
    printWorld(out, environment.state());
    printSteps(out, environment, actions);

    out << "\nFinal world:\n";
    printWorld(out, environment.state());
    out << "Goal reached: " << (isGoalReached(environment.state()) ? "yes" : "no") << '\n';
    out << "Steps taken: " << environment.stepCount() << '\n';
}

}  // namespace ai_hologram
