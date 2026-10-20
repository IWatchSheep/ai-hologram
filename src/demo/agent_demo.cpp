// Purpose: Runs the two C1 demonstrations of plan 5.4 and writes
// their exact lines (plan 5.5).
// Introduced in Stage 3.
// Architecture: Each demonstration builds a SingleAgent (C1) and runs
// it with runSingleAgent, the only code that touches Environment for
// an agent run. Demo 2's wrong belief is written by hand below, as in
// Stage 2; it is not fault injection.

#include "demo/agent_demo.h"

#include "agent/run_loop.h"
#include "agent/single_agent.h"
#include "world/world_text.h"

#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

// The Stage 1 demo layout (also layout D of the tests).
std::vector<std::string> demoLayout() {
    return {"##########", "#........#", "#........#", "#..A.O..D#",
            "#........#", "#........#", "#...##...#", "#........#",
            "#........#", "##########"};
}

// Layout D with a wall added at (4, 3): demo 2's true world.
std::vector<std::string> demoLayoutWithWall() {
    return {"##########", "#........#", "#........#", "#..A#O..D#",
            "#........#", "#........#", "#...##...#", "#........#",
            "#........#", "##########"};
}

void writeLines(std::ostream& out, const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        out << line << '\n';
    }
}

// Runs one demonstration and writes its title and run lines. When a
// starting belief is given, C1 begins with that hand-written belief
// instead of the true state (demo 2 only).
void runDemo(std::ostream& out, const char* title,
             const WorldState& trueWorld,
             const std::optional<WorldState>& startingBelief,
             const AgentSettings& settings) {
    const Observation first{1, trueWorld, std::nullopt, std::nullopt};
    SingleAgent agent(first, settings, startingBelief);
    const RunResult result = runSingleAgent(trueWorld, agent, settings);
    out << title << '\n';
    writeLines(out, result.lines);
}

}  // namespace

void runAgentDemos(std::ostream& out) {
    // Illustrative values only (D-027): the real B, S_max, T_max and
    // T_v are Phase C decisions, so the program says what it uses.
    const AgentSettings settings{200, 5, 100, 3};
    const ParseResult demo = parseWorld(demoLayout());
    const ParseResult walled = parseWorld(demoLayoutWithWall());
    if (!demo.world || !walled.world) {
        out << "Agent demo layout is invalid.\n";
        return;
    }
    out << '\n';
    out << "Single-agent demonstrations use illustrative settings: budget 200 per step,\n";
    out << "safe-stop limit 5, step limit 100, active-test delay 3 (not the experiment's values).\n";
    out << '\n';
    runDemo(out, "C1 demo 1: correct belief", *demo.world, std::nullopt,
            settings);
    out << '\n';
    // Demo 2: the true world has the wall at (4, 3), but the
    // hand-written starting belief is the demo layout without it.
    runDemo(out, "C1 demo 2: belief wrongly has no wall at (4, 3)",
            *walled.world, *demo.world, settings);
    out << "Note: demo 2's wrong belief is hand-written; it is not fault injection.\n";
}

}  // namespace ai_hologram
