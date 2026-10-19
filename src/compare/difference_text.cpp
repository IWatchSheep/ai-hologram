// Purpose: Formats each state difference as one readable line.
// Introduced in Stage 2.
// Architecture: This component reports comparison results without changing them.

#include "compare/difference_text.h"

namespace ai_hologram {
namespace {

std::string positionText(const Position position) {
    return "(" + std::to_string(position.x) + ", " +
           std::to_string(position.y) + ")";
}

const char* wallText(const bool blocked) {
    return blocked ? "blocked" : "free";
}

void appendPositionDifference(std::vector<std::string>& lines,
                              const char* name,
                              const PositionDifference& difference) {
    if (difference.differs) {
        lines.push_back("  - " + std::string(name) + ": predicted " +
                        positionText(difference.predicted) + ", observed " +
                        positionText(difference.observed));
    }
}

void appendWallDifferences(std::vector<std::string>& lines,
                           const std::vector<WallDifference>& walls) {
    for (const WallDifference& wall : walls) {
        lines.push_back("  - wall at " + positionText(wall.cell) +
                        ": predicted " + wallText(wall.predictedBlocked) +
                        ", observed " + wallText(wall.observedBlocked));
    }
}

}  // namespace

std::vector<std::string> describeDifferences(
    const StateDifference& difference) {
    std::vector<std::string> lines;
    appendPositionDifference(lines, "agent", difference.agent);
    appendPositionDifference(lines, "object", difference.object);
    appendPositionDifference(lines, "destination", difference.destination);
    appendWallDifferences(lines, difference.walls);
    return lines;
}

}  // namespace ai_hologram
