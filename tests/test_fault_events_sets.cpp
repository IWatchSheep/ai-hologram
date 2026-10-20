// Purpose: Checks FE2, FE3 and FE4: the pilot and confirmatory
// fault-event lists (material only) have exactly the reference
// counts and lines, and every list's steps and cells are valid
// against the true layouts (plan 3.3 and 6.3).
// Introduced in Stage 4.
// Architecture: These checks verify experimenter code only. No agent
// (C0 or C1) is run in this file; the pilot and confirmatory sets are
// generated as material only and nothing is run on them. This file
// was split off from test_fault_events.cpp so that file stays under
// about 200 lines. It prints no new section heading.

#include "agent/dead_squares.h"
#include "experiment/fault_events.h"
#include "experiment/layout_generator.h"
#include "test_suites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

struct Material {
    std::vector<RunKey> runs;
    std::vector<WorldState> layouts;
    std::vector<std::vector<FaultEvent>> events;
};

// Builds one whole set: layouts first..first+count-1, run seeds
// 1..runSeeds, in the order layout ascending then run seed ascending.
// Each layout is generated once and reused for its runs.
Material buildMaterial(std::uint64_t first, int count, int runSeeds) {
    Material material;
    for (int i = 0; i < count; ++i) {
        const std::uint64_t seed = first + static_cast<std::uint64_t>(i);
        const WorldState world = generateLayout(seed).world;
        for (int runSeed = 1; runSeed <= runSeeds; ++runSeed) {
            material.runs.push_back({seed, runSeed});
            material.layouts.push_back(world);
        }
    }
    material.events = generateFaultEvents(material.runs, material.layouts, 4242);
    return material;
}

// One aggregate check of per-type counts for a whole list.
bool typeCountsOk(const Material& material, int perType, int common,
                  int perTarget) {
    std::array<int, 5> totals{};
    std::array<int, 5> commons{};
    std::array<int, 5> r1{};
    std::array<int, 5> r2{};
    for (const std::vector<FaultEvent>& run : material.events) {
        for (const FaultEvent& event : run) {
            const std::size_t t = static_cast<std::size_t>(event.type);
            ++totals[t];
            if (event.target == FaultTarget::Both) {
                ++commons[t];
            } else if (event.target == FaultTarget::R1) {
                ++r1[t];
            } else {
                ++r2[t];
            }
        }
    }
    bool ok = true;
    for (int t = 0; t < 5; ++t) {
        const std::size_t k = static_cast<std::size_t>(t);
        ok = ok && totals[k] == perType && commons[k] == common;
        ok = ok && r1[k] == perTarget && r2[k] == perTarget;
    }
    return ok;
}

// F1 directions are split in halves, and no F1 or F4 event lacks a cell.
bool directionsAndCellsOk(const Material& material, int half) {
    int freeToBlocked = 0;
    int blockedToFree = 0;
    bool cellsOk = true;
    for (const std::vector<FaultEvent>& run : material.events) {
        for (const FaultEvent& event : run) {
            if (event.type == FaultType::F1) {
                if (event.direction == F1Direction::FreeToBlocked) {
                    ++freeToBlocked;
                } else {
                    ++blockedToFree;
                }
            }
            if (event.type == FaultType::F1 || event.type == FaultType::F4) {
                cellsOk = cellsOk && event.hasCell;
            }
        }
    }
    return cellsOk && freeToBlocked == half && blockedToFree == half;
}

bool runLinesOk(const std::vector<FaultEvent>& run,
                const std::vector<std::string>& expected) {
    if (run.size() != expected.size()) {
        return false;
    }
    bool ok = true;
    for (std::size_t i = 0; i < run.size(); ++i) {
        ok = ok && faultEventText(run[i]) == expected[i];
    }
    return ok;
}

// FE4 for one list: 3 sorted distinct steps per run; F1 and F4 cells
// are interior, not the destination, and match the true layout.
bool listIsValid(const Material& material) {
    bool ok = material.events.size() == material.runs.size();
    for (std::size_t r = 0; r < material.events.size() && ok; ++r) {
        const std::vector<FaultEvent>& run = material.events[r];
        const WorldState& layout = material.layouts[r];
        ok = ok && run.size() == 3;
        FreeComputationCounts counts;
        const DeadSquareMap map = computeDeadSquares(layout, {}, {}, counts);
        for (int j = 0; j < 3 && ok; ++j) {
            const FaultEvent& event = run[static_cast<std::size_t>(j)];
            ok = ok && event.index == j + 1 && event.step >= 1 && event.step <= 10;
            if (j > 0) {
                ok = ok && run[static_cast<std::size_t>(j - 1)].step < event.step;
            }
            const std::size_t cx = static_cast<std::size_t>(event.cell.x);
            const std::size_t cy = static_cast<std::size_t>(event.cell.y);
            const bool inside = event.cell.x >= 1 && event.cell.x <= 8 &&
                                event.cell.y >= 1 && event.cell.y <= 8;
            if (event.hasCell && event.type == FaultType::F1) {
                const bool wall = layout.walls[cy][cx];
                const bool statusOk =
                    event.direction == F1Direction::FreeToBlocked ? !wall : wall;
                ok = ok && inside && event.cell != layout.destination && statusOk;
            }
            if (event.hasCell && event.type == FaultType::F4) {
                ok = ok && inside && event.cell != layout.destination && map.dead[cy][cx];
            }
        }
    }
    return ok;
}

void checkPilot(TestCounter& counter, const Material& pilot) {
    check(counter, pilot.events.size() == 80 && typeCountsOk(pilot, 48, 10, 19),
          "FE2: pilot list counts per type, common-mode, R1 and R2");
    check(counter, directionsAndCellsOk(pilot, 24),
          "FE2: pilot F1 directions 24/24 and no F1 or F4 lacks a cell");
    const std::vector<std::string> expected{
        "900000-1-1 step 1 F4 target both | (4, 1)",
        "900000-1-2 step 2 F5 target both | -",
        "900000-1-3 step 8 F3 target both | West right"};
    check(counter, runLinesOk(pilot.events[0], expected),
          "FE2: run 900000-1 lines match exactly");
}

void checkConfirmatory(TestCounter& counter, const Material& confirmatory) {
    check(counter, confirmatory.events.size() == 2000 &&
              typeCountsOk(confirmatory, 1200, 240, 480),
          "FE3: confirmatory counts per type, common-mode, R1 and R2");
    check(counter, directionsAndCellsOk(confirmatory, 600),
          "FE3: confirmatory F1 directions 600/600, no F1 or F4 lacks a cell");
    std::array<int, 10> steps{};
    for (const std::vector<FaultEvent>& run : confirmatory.events) {
        for (const FaultEvent& event : run) {
            ++steps[static_cast<std::size_t>(event.step - 1)];
        }
    }
    const std::array<int, 10> wanted{580, 620, 603, 578, 609, 577, 614, 597, 605, 617};
    check(counter, steps == wanted,
          "FE3: confirmatory events per step 1..10 match exactly");
    const std::vector<std::string> first{
        "100000-1-1 step 2 F2 target both | South West East North",
        "100000-1-2 step 7 F2 target both | South North East West",
        "100000-1-3 step 8 F3 target R2 | South right"};
    const std::vector<std::string> last{
        "100399-5-1 step 2 F5 target R2 | -",
        "100399-5-2 step 5 F4 target R1 | (8, 3)",
        "100399-5-3 step 6 F1 target both | (1, 3) free->blocked"};
    check(counter, runLinesOk(confirmatory.events.front(), first) &&
              runLinesOk(confirmatory.events.back(), last),
          "FE3: first and last confirmatory runs match exactly");
}

}  // namespace

void runFaultEventSetTests(TestCounter& counter) {
    const Material demo = buildMaterial(500000, 4, 2);
    const Material pilot = buildMaterial(900000, 40, 2);
    const Material confirmatory = buildMaterial(100000, 400, 5);
    checkPilot(counter, pilot);
    checkConfirmatory(counter, confirmatory);
    check(counter, listIsValid(demo),
          "FE4: demonstration steps sorted/distinct, F1/F4 cells true");
    check(counter, listIsValid(pilot),
          "FE4: pilot steps sorted/distinct, F1/F4 cells true");
    check(counter, listIsValid(confirmatory),
          "FE4: confirmatory steps sorted/distinct, F1/F4 cells true");
}

}  // namespace ai_hologram
