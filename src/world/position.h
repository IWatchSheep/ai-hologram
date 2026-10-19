// Purpose: Defines grid positions and compares them.
// Introduced in Stage 1.
// Architecture: Positions are shared by the world model and its helpers.

#pragma once

namespace ai_hologram {

// A struct groups related values; x is the column and y is the row.
struct Position {
    int x;
    int y;
};

// These overloads enable position comparisons; constexpr also permits compile-time use.
constexpr bool operator==(const Position& left, const Position& right) {
    return left.x == right.x && left.y == right.y;
}

constexpr bool operator!=(const Position& left, const Position& right) {
    return !(left == right);
}

}  // namespace ai_hologram
