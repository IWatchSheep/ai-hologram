// Purpose: Runs and prints prediction demonstrations against observed outcomes.
// Introduced in Stage 2.
// Architecture: The demos connect the model and comparison components to console output.
// The actions and the wrong belief are hand-written; nothing chooses them; this is not fault injection.
// Replacing the belief with the observation is only a demonstration shortcut; belief correction is a Stage 3 decision.

#include "demo/prediction_demo.h"

#include "compare/difference_text.h"
#include "compare/state_difference.h"
#include "model/belief.h"
#include "model/budget_counter.h"
#include "model/world_model.h"
#include "world/action.h"
#include "world/environment.h"
#include "world/world_text.h"

#include <array>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

std::vector<std::string> makeDemoLayout() {
    return {
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
}

void printDifferenceLines(std::ostream& out,
                          const PredictionComparison& comparison) {
    for (const std::string& line : describeDifferences(comparison.state)) {
        out << line << '\n';
    }
}

int runPredictionSteps(std::ostream& out, Environment& environment,
                       Belief& belief, const WorldModel& model,
                       BudgetCounter& budget) {
    constexpr std::array<Action, 5> actions{
        Action::East, Action::East, Action::East, Action::East, Action::East
    };
    int mismatchCount = 0;
    int stepNumber = 1;
    for (const Action action : actions) {
        // An empty optional means the shared budget did not allow this prediction.
        const std::optional<Prediction> prediction =
            model.predict(belief, action, budget);
        if (!prediction) {
            out << "No prediction available; stopping this demo.\n";
            break;
        }

        // The environment supplies the observation after the belief-based prediction.
        const ActionResult observed = environment.step(action);
        const PredictionComparison comparison =
            comparePrediction(*prediction, observed);
        const bool stepMatches = matches(comparison);
        out << "Step " << stepNumber << ": " << actionName(action)
            << " | predicted " << outcomeName(prediction->outcome)
            << ", observed " << outcomeName(observed.outcome) << " | "
            << (stepMatches ? "match" : "MISMATCH") << '\n';
        printDifferenceLines(out, comparison);
        if (!stepMatches) {
            ++mismatchCount;
        }
        // This assignment is a demo shortcut, not a belief-correction rule.
        belief.state = observed.state;
        ++stepNumber;
    }
    return mismatchCount;
}

void runOnePredictionDemo(std::ostream& out, const char* title,
                          const bool addWrongWall) {
    const ParseResult parsed = parseWorld(makeDemoLayout());
    if (!parsed.world) {
        out << "Prediction demo layout is invalid: " << parsed.error << '\n';
        return;
    }

    Environment environment(*parsed.world);
    Belief belief{*parsed.world};
    if (addWrongWall) {
        belief.state.walls[3][6] = true;
    }
    const WorldModel model;
    BudgetCounter budget(10);
    out << '\n' << title << '\n';
    const int mismatchCount =
        runPredictionSteps(out, environment, belief, model, budget);
    out << "Mismatches: " << mismatchCount << '\n';
    out << "Model evaluations used: " << budget.used() << '\n';
}

}  // namespace

void runPredictionDemos(std::ostream& out) {
    runOnePredictionDemo(out, "Prediction demo 1: correct belief", false);
    runOnePredictionDemo(
        out, "Prediction demo 2: belief wrongly includes a wall at (6, 3)", true);
    out << "Note: in these demos the belief is replaced by the observation after every step.\n";
    out << "How beliefs are corrected is decided in Stage 3.\n";
}

}  // namespace ai_hologram
