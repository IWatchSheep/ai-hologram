// Purpose: Checks LG1-LG4: the experimenter's true-state search and
// the seeded layout generator give exactly the reference values
// (plan 3.2).
// Introduced in Stage 4.
// Architecture: These checks verify experimenter code only. No agent
// (C0 or C1) is run in this file. LG5 and LG6 are in
// test_layout_generator_sets.cpp, split off so this file stays under
// about 200 lines with one statement per line.

#include "agent/dead_squares.h"
#include "experiment/layout_generator.h"
#include "experiment/true_search.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

// Layout D, the hand-built layout of the prompt (rows y = 0..9).
WorldState layoutD(TestCounter& counter) {
    const auto parsed = parseWorld({"##########", "#........#", "#........#",
                                    "#..A.O..D#", "#........#", "#........#",
                                    "#...##...#", "#........#", "#........#",
                                    "##########"});
    check(counter, parsed.world && isValidWorld(*parsed.world),
          "LG1: hand-built layout D is a valid world");
    return parsed.world ? *parsed.world : WorldState{};
}

// Dead interior cells other than the destination (plan 3.2), no overrides.
int deadInteriorCount(const WorldState& world) {
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(world, {}, {}, counts);
    int total = 0;
    for (int y = 1; y <= 8; ++y) {
        for (int x = 1; x <= 8; ++x) {
            if (Position{x, y} != world.destination &&
                map.dead[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]) {
                ++total;
            }
        }
    }
    return total;
}

int interiorWallCount(const WorldState& world) {
    int total = 0;
    for (int y = 1; y <= 8; ++y) {
        for (int x = 1; x <= 8; ++x) {
            if (world.walls[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]) {
                ++total;
            }
        }
    }
    return total;
}

bool bordersAreWalls(const WorldState& world) {
    for (int i = 0; i < gridSize; ++i) {
        const std::size_t k = static_cast<std::size_t>(i);
        if (!world.walls[0][k] || !world.walls[gridSize - 1][k] ||
            !world.walls[k][0] || !world.walls[k][gridSize - 1]) {
            return false;
        }
    }
    return true;
}

void checkTrueSearch(TestCounter& counter) {
    const WorldState d = layoutD(counter);
    const auto length = shortestSolutionLength(d);
    check(counter, length && *length == 4, "LG1: shortestSolutionLength(D) == 4");
    WorldState corner = d;
    corner.object = {1, 1};
    check(counter, isValidWorld(corner), "LG1: D with object at (1, 1) is valid");
    check(counter, !shortestSolutionLength(corner).has_value(),
          "LG1: D with the object in a corner has no solution length");
    WorldState solved = d;
    solved.object = {8, 3};
    check(counter, isValidWorld(solved),
          "LG1: D with object on the destination is valid");
    const auto zero = shortestSolutionLength(solved);
    check(counter, zero && *zero == 0, "LG1: object on destination gives 0");
    WorldState invalid = d;
    invalid.agent = {10, 3};
    check(counter, !isValidWorld(invalid) &&
              !shortestSolutionLength(invalid).has_value(),
          "LG1: an invalid world (agent outside the grid) has no solution length");
}

struct ExpectedLayout {
    std::uint64_t seed;
    int attempts;
    int shortest;
    Position agent;
    Position object;
    Position destination;
    int deadCells;
    int walls;
};

void checkExactLayouts(TestCounter& counter) {
    const ExpectedLayout expected[] = {
        {500000, 3, 12, {7, 2}, {6, 5}, {2, 6}, 25, 7},
        {500001, 6, 16, {8, 1}, {4, 5}, {8, 4}, 23, 9},
        {500002, 1, 29, {4, 1}, {4, 7}, {8, 5}, 27, 8},
        {500003, 3, 14, {1, 1}, {4, 5}, {7, 4}, 26, 7},
        {900000, 2, 17, {7, 1}, {7, 6}, {2, 8}, 19, 7},
        {900001, 4, 19, {4, 3}, {3, 7}, {5, 6}, 26, 11}};
    for (const ExpectedLayout& e : expected) {
        const GeneratedLayout layout = generateLayout(e.seed);
        check(counter, layout.found && layout.attempts == e.attempts &&
                  layout.shortest == e.shortest && layout.world.agent == e.agent &&
                  layout.world.object == e.object &&
                  layout.world.destination == e.destination &&
                  deadInteriorCount(layout.world) == e.deadCells &&
                  interiorWallCount(layout.world) == e.walls,
              "LG2: layout " + std::to_string(e.seed) + " matches reference");
    }
}

void checkRenderedLayouts(TestCounter& counter) {
    const std::vector<std::string> expected500000 = {
        "##########", "#...#....#", "#.....#A.#", "##.......#", "#.#......#",
        "#.....O..#", "##D...#..#", "#........#", "#......#.#", "##########"};
    const std::vector<std::string> expected900000 = {
        "##########", "#....#.A##", "##...#...#", "#........#", "#.....#..#",
        "#.#......#", "#......O.#", "#......#.#", "#.D......#", "##########"};
    check(counter, renderWorld(generateLayout(500000).world) == expected500000,
          "LG3: renderWorld of layout 500000 matches exactly");
    check(counter, renderWorld(generateLayout(900000).world) == expected900000,
          "LG3: renderWorld of layout 900000 matches exactly");
    const int attempts[] = {3, 1, 3, 8, 5, 2};
    const int shortest[] = {20, 16, 12, 12, 12, 16};
    bool matches = true;
    for (int i = 0; i < 6; ++i) {
        const GeneratedLayout layout = generateLayout(500004ULL + i);
        matches = matches && layout.found && layout.attempts == attempts[i] &&
                  layout.shortest == shortest[i];
    }
    check(counter, matches, "LG3: layouts 500004-500009 attempts/shortest match");
}

void checkLayoutProperties(TestCounter& counter) {
    for (std::uint64_t seed = 500000; seed <= 500009; ++seed) {
        const GeneratedLayout layout = generateLayout(seed);
        const auto shortest = shortestSolutionLength(layout.world);
        check(counter, layout.found && isValidWorld(layout.world) &&
                  bordersAreWalls(layout.world) && layout.shortest >= 12 &&
                  layout.shortest <= 40 && shortest && *shortest == layout.shortest &&
                  deadInteriorCount(layout.world) >= 1,
              "LG4: layout " + std::to_string(seed) + " valid/bordered/12..40/dead");
    }
}

}  // namespace

void runLayoutGeneratorTests(TestCounter& counter) {
    std::cout << "--- Layout generator ---\n";
    checkTrueSearch(counter);
    checkExactLayouts(counter);
    checkRenderedLayouts(counter);
    checkLayoutProperties(counter);
}

}  // namespace ai_hologram
