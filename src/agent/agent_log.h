// Purpose: Declares functions that build every exact log line of plan 5.3.
// Introduced in Stage 3.
// Architecture: SingleAgent and the run loop only append lines made here, so
// the exact wording lives in one place. Positions are written "(x, y)".

#pragma once

#include "world/action.h"
#include "world/position.h"
#include "world/rules.h"

#include <string>

namespace ai_hologram {

std::string positionText(Position cell);
std::string planSearchLine(int actions);
std::string planFallbackLine(int actions);
std::string planActiveTestLine(int correctionId, int actions);
std::string planCautiousLine();
std::string stepLine(int step, Action action, MoveOutcome predicted,
                     MoveOutcome observed, bool matched);
std::string safeStopLine(int step, const std::string& reason);
std::string correctionVerifiedLine(int correctionId);
std::string correctionRejectedLine(int correctionId);
std::string investigationWallLine(Position cell, bool believedBlocked,
                                  bool recordedBlocked);
std::string investigationObjectLine(Position believed, Position recorded);
std::string investigationModelLine(Action action);
std::string anomalyAgentLine();
std::string investigationDeferredLine();
std::string correctionWallLine(int correctionId, Position cell, bool blocked);
std::string correctionObjectLine(int correctionId, Position position);
std::string correctionModelLine(int correctionId, Action action);
std::string unexplainedLine();
std::string consequenceSurpriseLine(Position cell);
std::string fullStateDeadEndLine();
std::string consequenceMismatchLine(Position cell);
std::string correctionTrapLine(int correctionId, Position cell);
std::string anomalyInvalidLine();
std::string resultLine(const std::string& outcome, int worldSteps, int safeStops);
std::string mismatchesLine(int mismatches);
std::string correctionsLine(int corrections, int verified);
std::string evaluationsLine(int evaluations);

}  // namespace ai_hologram
