// Purpose: Builds every exact log line of plan 5.3.
// Introduced in Stage 3.
// Architecture: These are plain text builders. They make no predictions and
// change nothing; SingleAgent decides when each line is written.

#include "agent/agent_log.h"

namespace ai_hologram {
namespace {

std::string blockedText(bool blocked) { return blocked ? "blocked" : "free"; }

}  // namespace

std::string positionText(Position cell) {
    return "(" + std::to_string(cell.x) + ", " + std::to_string(cell.y) + ")";
}

std::string planSearchLine(int actions) {
    return "Plan: search found a plan of " + std::to_string(actions) + " actions";
}

std::string planFallbackLine(int actions) {
    return "Plan: fallback toward the best state found (" +
           std::to_string(actions) + " actions)";
}

std::string planActiveTestLine(int correctionId, int actions) {
    return "Plan: active test of correction " + std::to_string(correctionId) +
           " (" + std::to_string(actions) + " actions)";
}

std::string planCautiousLine() {
    return "Plan: cautious mode (investigation unfinished)";
}

std::string stepLine(int step, Action action, MoveOutcome predicted,
                     MoveOutcome observed, bool matched) {
    return "Step " + std::to_string(step) + ": " + actionName(action) +
           " | predicted " + outcomeName(predicted) + ", observed " +
           outcomeName(observed) + (matched ? " | match" : " | MISMATCH");
}

std::string safeStopLine(int step, const std::string& reason) {
    return "Step " + std::to_string(step) + ": safe stop (" + reason + ")";
}

std::string correctionVerifiedLine(int correctionId) {
    return "  - correction " + std::to_string(correctionId) + ": VERIFIED";
}

std::string correctionRejectedLine(int correctionId) {
    return "  - correction " + std::to_string(correctionId) + ": REJECTED";
}

std::string investigationWallLine(Position cell, bool believedBlocked,
                                  bool recordedBlocked) {
    return "  - investigation: wall at " + positionText(cell) + " believed " +
           blockedText(believedBlocked) + ", recorded " +
           blockedText(recordedBlocked) + " (source S2)";
}

std::string investigationObjectLine(Position believed, Position recorded) {
    return "  - investigation: object believed at " + positionText(believed) +
           ", recorded at " + positionText(recorded) + " (source S2)";
}

std::string investigationModelLine(Action action) {
    return "  - investigation: belief matched the record; action model for " +
           actionName(action) + " suspected (source S1)";
}

std::string anomalyAgentLine() {
    return "  - anomaly: believed agent position differs from the record";
}

std::string investigationDeferredLine() {
    return "  - investigation: budget too small for verification; retried next step";
}

std::string correctionWallLine(int correctionId, Position cell, bool blocked) {
    return "  - correction " + std::to_string(correctionId) + ": wall at " +
           positionText(cell) + " set to " + blockedText(blocked) +
           ": CONSISTENT";
}

std::string correctionObjectLine(int correctionId, Position position) {
    return "  - correction " + std::to_string(correctionId) +
           ": object position set to " + positionText(position) + ": CONSISTENT";
}

std::string correctionModelLine(int correctionId, Action action) {
    return "  - correction " + std::to_string(correctionId) +
           ": action model for " + actionName(action) +
           " restored to the standard rule: CONSISTENT";
}

std::string unexplainedLine() {
    return "  - investigation: no candidate passed V1 (source S5); unexplained update";
}

std::string consequenceSurpriseLine(Position cell) {
    return "  - consequence surprise: search exhausted with the object at " +
           positionText(cell);
}

std::string fullStateDeadEndLine() {
    return "  - full-state dead end (dead-square classification consistent; no claim)";
}

std::string consequenceMismatchLine(Position cell) {
    return "  - consequence mismatch: " + positionText(cell) +
           " violates the dead-square rule (source S1)";
}

std::string correctionTrapLine(int correctionId, Position cell) {
    return "  - correction " + std::to_string(correctionId) + ": " +
           positionText(cell) + " classified as a dead square: CONSISTENT";
}

std::string anomalyInvalidLine() {
    return "  - anomaly: invalid hypothetical state skipped";
}

std::string resultLine(const std::string& outcome, int worldSteps, int safeStops) {
    return "Result: " + outcome + " after " + std::to_string(worldSteps) +
           " world steps, " + std::to_string(safeStops) + " safe stops";
}

std::string mismatchesLine(int mismatches) {
    return "Mismatches: " + std::to_string(mismatches);
}

std::string correctionsLine(int corrections, int verified) {
    return "Corrections: " + std::to_string(corrections) + " (" +
           std::to_string(verified) + " verified)";
}

std::string evaluationsLine(int evaluations) {
    return "Model evaluations used: " + std::to_string(evaluations);
}

}  // namespace ai_hologram
