// Purpose: Implements the seeded layout generator (plan 3.2).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent file
// may include its header, and no agent ever calls this function. It uses
// the experimenter's true-state search and the Stage 3 dead-square
// analysis (as a plain function on the true world) to accept layouts.

#include "experiment/layout_generator.h"

#include "agent/dead_squares.h"
#include "experiment/seeded_random.h"
#include "experiment/true_search.h"

#include <cstddef>
#include <vector>

namespace ai_hologram {
namespace {

// True when an interior cell (x and y from 1 to 8) other than the
// destination is dead in the true analysis with no overrides. Layouts
// must contain a true dead square because fault type F4 needs one
// (Brief 11.2, protocol Section 14).
bool hasTrueDeadSquare(const WorldState& world) {
    FreeComputationCounts counts;
    const DeadSquareMap map = computeDeadSquares(world, {}, {}, counts);
    for (int y = 1; y <= 8; ++y) {
        for (int x = 1; x <= 8; ++x) {
            const Position cell{x, y};
            if (cell != world.destination &&
                map.dead[static_cast<std::size_t>(y)]
                        [static_cast<std::size_t>(x)]) {
                return true;
            }
        }
    }
    return false;
}

// Builds one candidate world for this attempt, drawing numbers exactly
// as plan 3.2 fixes: border cells are walls and draw nothing; each
// interior cell draws below(100) and is a wall when the draw is < 12.
// ("About 12% interior walls" is read as each interior cell
// independently a wall with probability 12/100, D-031.) The agent,
// object and destination are then three distinct free cells, drawn with
// one pickDistinct call, in that order.
WorldState buildCandidate(SeededRandom& random, bool& hasThreeFree) {
    WorldState world{};
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            const bool border = x == 0 || y == 0 || x == gridSize - 1 ||
                                y == gridSize - 1;
            if (border) {
                world.walls[static_cast<std::size_t>(y)]
                           [static_cast<std::size_t>(x)] = true;
            } else {
                world.walls[static_cast<std::size_t>(y)]
                           [static_cast<std::size_t>(x)] =
                    random.below(100) < 12;
            }
        }
    }
    // Every non-wall cell, row by row (y outer, x inner).
    std::vector<Position> free;
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            if (!world.walls[static_cast<std::size_t>(y)]
                            [static_cast<std::size_t>(x)]) {
                free.push_back({x, y});
            }
        }
    }
    hasThreeFree = free.size() >= 3;
    if (!hasThreeFree) {
        return world;
    }
    const std::vector<Position> chosen = random.pickDistinct(free, 3);
    world.agent = chosen[0];
    world.object = chosen[1];
    world.destination = chosen[2];
    return world;
}

}  // namespace

GeneratedLayout generateLayout(const std::uint64_t layoutSeed) {
    SeededRandom random(layoutSeed);
    for (int attempt = 1; attempt <= 10000; ++attempt) {
        bool hasThreeFree = false;
        const WorldState world = buildCandidate(random, hasThreeFree);
        if (!hasThreeFree || !isValidWorld(world)) {
            continue;
        }
        const std::optional<int> shortest = shortestSolutionLength(world);
        if (!shortest || *shortest < 12 || *shortest > 40) {
            continue;
        }
        if (!hasTrueDeadSquare(world)) {
            continue;
        }
        GeneratedLayout accepted;
        accepted.found = true;
        accepted.world = world;
        accepted.attempts = attempt;
        accepted.shortest = *shortest;
        return accepted;
    }
    GeneratedLayout none;
    none.found = false;
    none.attempts = 10000;
    return none;
}

}  // namespace ai_hologram
