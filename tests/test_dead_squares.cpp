// Purpose: Checks the dead-square analysis and the local check.
// Introduced in Stage 3.
// Architecture: DS1, DS2, DS3 (first part), DS4 and DS5 live here; DS6,
// DS7 and DS8 live in test_dead_squares_cases.cpp so each file stays
// under about 200 lines. Every state used is checked valid with check().

#include "agent/dead_squares.h"
#include "agent/heuristic.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

// The Stage 1 demo layout D: agent (3, 3), object (5, 3), destination
// (8, 3), interior walls (4, 6) and (5, 6).
WorldState demoWorld() {
    const ParseResult parsed = parseWorld({
        "##########", "#........#", "#........#", "#..A.O..D#", "#........#",
        "#........#", "#...##...#", "#........#", "#........#", "##########"});
    return parsed.world.value_or(WorldState{});
}

bool deadAt(const DeadSquareMap& map, const Position cell) {
    return map.dead[static_cast<std::size_t>(cell.y)]
                   [static_cast<std::size_t>(cell.x)];
}

int distanceAt(const DeadSquareMap& map, const Position cell) {
    return map.pushDistance[static_cast<std::size_t>(cell.y)]
                           [static_cast<std::size_t>(cell.x)];
}

void checkDS1(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "DS1: demo layout D is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(belief, {}, {}, counts);
    check(counter, deadAt(map, {1, 1}),
          "DS1: corner (1, 1), not the destination, is dead");
    check(counter, !deadAt(map, {8, 3}) && distanceAt(map, {8, 3}) == 0,
          "DS1: destination (8, 3) is not dead and has pushDistance 0");
    bool wallsNeverDead = true;
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            if (isWall(belief, {x, y}) && deadAt(map, {x, y})) {
                wallsNeverDead = false;
            }
        }
    }
    check(counter, wallsNeverDead, "DS1: wall cells are never dead");
}

void checkDS2(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "DS2: demo layout D is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(belief, {}, {}, counts);
    // Expected grid from the prompt: -2 = wall, -3 = dead ('x'), any
    // other number is the pushDistance of that cell.
    const int expected[gridSize][gridSize] = {
        {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2},
        {-2, -3, -3, -3, -3, -3, -3, -3, -3, -2},
        {-2, -3, 7, 6, 5, 4, 3, 2, 1, -2},
        {-2, -3, 6, 5, 4, 3, 2, 1, 0, -2},
        {-2, -3, 7, 6, 5, 4, 3, 2, 1, -2},
        {-2, -3, 8, 7, 6, 5, 4, 3, 2, -2},
        {-2, -3, 9, 8, -2, -2, 5, 4, 3, -2},
        {-2, -3, 10, 9, 8, 7, 6, 5, 4, -2},
        {-2, -3, -3, -3, -3, -3, -3, -3, -3, -2},
        {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2}};
    bool everyCellMatches = true;
    int deadCount = 0;
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            const int want = expected[y][x];
            if (deadAt(map, {x, y})) {
                ++deadCount;
            }
            if (want == -2) {
                everyCellMatches = everyCellMatches && isWall(belief, {x, y}) &&
                                   !deadAt(map, {x, y});
            } else if (want == -3) {
                everyCellMatches = everyCellMatches && deadAt(map, {x, y}) &&
                                   distanceAt(map, {x, y}) == -1;
            } else {
                everyCellMatches = everyCellMatches && !deadAt(map, {x, y}) &&
                                   distanceAt(map, {x, y}) == want;
            }
        }
    }
    check(counter, everyCellMatches,
          "DS2: every cell of D matches the expected dead/pushDistance grid");
    check(counter, deadCount == 22, "DS2: D has exactly 22 dead cells");
    check(counter, counts.analysisRuns == 1 && counts.cellOperations > 0,
          "DS2: one analysis run counted, with cell operations counted");
}

void checkDS3FirstPart(TestCounter& counter) {
    // Layout L2: the object's region (left) is sealed off from the agent
    // (right) by walls, but relaxed pushes ignore agent reachability.
    const ParseResult parsed = parseWorld({
        "##########", "#....#...#", "#.O..#.A.#", "#....#...#", "#D...#...#",
        "######...#", "#........#", "#........#", "#........#", "##########"});
    const WorldState belief = parsed.world.value_or(WorldState{});
    check(counter, isValidWorld(belief), "DS3: layout L2 is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(belief, {}, {}, counts);
    // The object cell is NOT a dead square even though the agent can
    // never reach it: a dead square is about the object only, while a
    // full-state dead end also depends on the agent (plan 3.3, 9.1).
    // The search part of DS3 (Exhausted) is added in sub-stage 3c.
    check(counter, !deadAt(map, {2, 2}) && distanceAt(map, {2, 2}) == 3,
          "DS3: object cell (2, 2) of L2 is not dead, pushDistance 3");
}

void checkDS4(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "DS4: demo layout D is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(belief, {{1, 1}}, {}, counts);
    check(counter, !localCheckSaysNotDead(belief, map, {1, 1}, counts),
          "DS4: local check is false at overridden dead square (1, 1)");
    check(counter, localCheckSaysNotDead(belief, map, {5, 3}, counts),
          "DS4: local check is true at non-dead cell (5, 3)");
    check(counter, localCheckSaysNotDead(belief, map, {8, 3}, counts),
          "DS4: local check is true at the destination (8, 3)");
    check(counter, counts.localChecks == 3,
          "DS4: localChecks counts each of the three calls");
}

void checkDS5(TestCounter& counter) {
    const WorldState belief = demoWorld();
    check(counter, isValidWorld(belief), "DS5: demo layout D is a valid world");
    FreeComputationCounts counts;
    const DeadSquareMap plain = computeDeadSquares(belief, {}, {}, counts);
    const DeadSquareMap overridden =
        computeDeadSquares(belief, {{1, 1}}, {}, counts);
    int differentCells = 0;
    bool distancesUnchanged = true;
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            if (deadAt(plain, {x, y}) != deadAt(overridden, {x, y})) {
                ++differentCells;
            }
            if (distanceAt(plain, {x, y}) != distanceAt(overridden, {x, y})) {
                distancesUnchanged = false;
            }
        }
    }
    check(counter, differentCells == 1 && !deadAt(overridden, {1, 1}) &&
                       deadAt(plain, {1, 1}),
          "DS5: 'not dead' override on (1, 1) changes exactly one cell");
    check(counter, distancesUnchanged,
          "DS5: the override changes no cell's pushDistance");
    const DeadSquareMap corrected =
        computeDeadSquares(belief, {{1, 1}}, {{1, 1}}, counts);
    check(counter, deadAt(corrected, {1, 1}),
          "DS5: 'dead' correction on the same cell wins (applied last)");
}

}  // namespace

void checkDS6(TestCounter& counter);
void checkDS7(TestCounter& counter);
void checkDS8(TestCounter& counter);

void runDeadSquareTests(TestCounter& counter) {
    std::cout << "--- Dead squares and heuristic ---\n";
    checkDS1(counter);
    checkDS2(counter);
    checkDS3FirstPart(counter);
    checkDS4(counter);
    checkDS5(counter);
    checkDS6(counter);
    checkDS7(counter);
    checkDS8(counter);
}

}  // namespace ai_hologram
