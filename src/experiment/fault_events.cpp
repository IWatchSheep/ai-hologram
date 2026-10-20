// Purpose: Implements generation of the fault-event list for a whole
// experiment and the exact text form of one event (plan 3.3).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent
// file may include its header, and no agent ever calls these functions.

#include "experiment/fault_events.h"

#include "agent/dead_squares.h"
#include "experiment/seeded_random.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

// A common-mode event is one event applied to both reasoners at once,
// with the same details. It models shared mistakes: both reasoners can
// be wrong in the same way at the same time (Lesson L5). In C0 and C1
// it is simply applied to the single reasoner (protocol 13.8).
// The draw order here must never change: every draw advances its
// generator, so one reordered draw changes every later event.

// Interior cells (y 1..8 outer, x 1..8 inner) other than the
// destination: for F1, those whose TRUE wall status matches the flip;
// for F4, those truly dead in the true layout.
std::vector<Position> candidatesFor(const FaultEvent& event,
                                    const WorldState& layout) {
    DeadSquareMap map;
    if (event.type == FaultType::F4) {
        FreeComputationCounts counts;
        map = computeDeadSquares(layout, {}, {}, counts);
    }
    std::vector<Position> candidates;
    for (int y = 1; y <= 8; ++y) {
        for (int x = 1; x <= 8; ++x) {
            const Position cell{x, y};
            const std::size_t cx = static_cast<std::size_t>(x);
            const std::size_t cy = static_cast<std::size_t>(y);
            bool wanted = map.dead[cy][cx];
            if (event.type == FaultType::F1) {
                const bool blocked = layout.walls[cy][cx];
                wanted = (event.direction == F1Direction::BlockedToFree) == blocked;
            }
            if (cell != layout.destination && wanted) {
                candidates.push_back(cell);
            }
        }
    }
    return candidates;
}

// Gives every event of one type its target, and F1 its direction.
void assignTypeTargets(std::vector<FaultEvent>& events, FaultType type,
                       SeededRandom& random) {
    std::vector<int> idx;
    for (int i = 0; i < static_cast<int>(events.size()); ++i) {
        if (events[static_cast<std::size_t>(i)].type == type) {
            idx.push_back(i);
        }
    }
    std::vector<int> perm = idx;
    random.shuffle(perm);
    const int common = commonModeCount(static_cast<int>(idx.size()));
    for (int p = 0; p < static_cast<int>(perm.size()); ++p) {
        const std::size_t at = static_cast<std::size_t>(perm[static_cast<std::size_t>(p)]);
        FaultEvent& event = events[at];
        if (p < common) {
            event.target = FaultTarget::Both;
            event.commonMode = true;
        } else {
            event.target = ((p - common) % 2 == 0) ? FaultTarget::R1 : FaultTarget::R2;
            event.commonMode = false;
        }
    }
    if (type == FaultType::F1) {
        std::vector<int> perm2 = idx;
        random.shuffle(perm2);
        for (int p = 0; p < static_cast<int>(perm2.size()); ++p) {
            const std::size_t at = static_cast<std::size_t>(perm2[static_cast<std::size_t>(p)]);
            events[at].direction = (p % 2 == 0) ? F1Direction::FreeToBlocked
                                               : F1Direction::BlockedToFree;
        }
    }
}

// Draws the details of one event from its run's own stream. When no
// candidate cell exists, no number is drawn (plan 3.3).
void fillEventDetail(FaultEvent& event, const WorldState& layout,
                     SeededRandom& random) {
    if (event.type == FaultType::F1 || event.type == FaultType::F4) {
        const std::vector<Position> cells = candidatesFor(event, layout);
        if (!cells.empty()) {
            const int pick = random.below(static_cast<int>(cells.size()));
            event.hasCell = true;
            event.cell = cells[static_cast<std::size_t>(pick)];
        }
    } else if (event.type == FaultType::F2) {
        std::vector<Action> order{Action::North, Action::East, Action::South, Action::West};
        random.shuffle(order);
        event.order = {order[0], order[1], order[2], order[3]};
    } else if (event.type == FaultType::F3) {
        const std::array<Action, 4> actions{Action::North, Action::East, Action::South, Action::West};
        event.action = actions[static_cast<std::size_t>(random.below(4))];
        const std::array<Rotation, 2> rotations{Rotation::Left, Rotation::Right};
        event.rotation = rotations[static_cast<std::size_t>(random.below(2))];
    }
}

std::string faultTypeText(FaultType type) {
    const std::array<std::string, 5> names{"F1", "F2", "F3", "F4", "F5"};
    return names[static_cast<std::size_t>(type)];
}

std::string targetText(FaultTarget target) {
    return target == FaultTarget::Both ? "both" : (target == FaultTarget::R1 ? "R1" : "R2");
}

std::string cellText(Position cell) {
    return "(" + std::to_string(cell.x) + ", " + std::to_string(cell.y) + ")";
}

std::string detailText(const FaultEvent& event) {
    if (event.type == FaultType::F1) {
        const bool toBlocked = event.direction == F1Direction::FreeToBlocked;
        const std::string flip = toBlocked ? "free->blocked" : "blocked->free";
        return (event.hasCell ? cellText(event.cell) : "none") + " " + flip;
    }
    if (event.type == FaultType::F2) {
        return actionName(event.order[0]) + " " + actionName(event.order[1]) + " " +
               actionName(event.order[2]) + " " + actionName(event.order[3]);
    }
    if (event.type == FaultType::F3) {
        const std::string turn = event.rotation == Rotation::Left ? " left" : " right";
        return actionName(event.action) + turn;
    }
    if (event.type == FaultType::F4) {
        return event.hasCell ? cellText(event.cell) : "none";
    }
    return "-";
}

// Creates the flat event list (3 per run, in run order) with ids,
// run fields and the shuffled types already assigned.
std::vector<FaultEvent> makeEvents(const std::vector<RunKey>& runs,
                                   const std::vector<FaultType>& types) {
    std::vector<FaultEvent> events(types.size());
    for (int i = 0; i < static_cast<int>(types.size()); ++i) {
        FaultEvent& event = events[static_cast<std::size_t>(i)];
        const RunKey& run = runs[static_cast<std::size_t>(i / 3)];
        event.layoutSeed = run.layoutSeed;
        event.runSeed = run.runSeed;
        event.index = i % 3 + 1;
        event.type = types[static_cast<std::size_t>(i)];
        event.id = std::to_string(run.layoutSeed) + "-" +
                   std::to_string(run.runSeed) + "-" +
                   std::to_string(event.index);
    }
    return events;
}

}  // namespace

int commonModeCount(int count) {
    return (2 * count + 5) / 10;
}

std::uint64_t runStreamSeed(std::uint64_t masterSeed,
                            std::uint64_t layoutSeed, int runSeed) {
    return (masterSeed << 40) + (layoutSeed << 8) +
           static_cast<std::uint64_t>(runSeed);
}

std::vector<std::vector<FaultEvent>> generateFaultEvents(
    const std::vector<RunKey>& runs,
    const std::vector<WorldState>& layouts, std::uint64_t masterSeed) {
    if (runs.size() != layouts.size()) {
        return {};
    }
    SeededRandom experimentRandom(masterSeed);
    std::vector<FaultType> types;
    for (int i = 0; i < 3 * static_cast<int>(runs.size()); ++i) {
        types.push_back(static_cast<FaultType>(i % 5));
    }
    experimentRandom.shuffle(types);
    std::vector<FaultEvent> events = makeEvents(runs, types);
    for (int ordinal = 0; ordinal < 5; ++ordinal) {
        assignTypeTargets(events, static_cast<FaultType>(ordinal), experimentRandom);
    }
    std::vector<std::vector<FaultEvent>> result;
    for (int r = 0; r < static_cast<int>(runs.size()); ++r) {
        const RunKey& run = runs[static_cast<std::size_t>(r)];
        SeededRandom runRandom(runStreamSeed(masterSeed, run.layoutSeed, run.runSeed));
        const std::vector<int> allSteps{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::vector<int> steps = runRandom.pickDistinct(allSteps, 3);
        std::sort(steps.begin(), steps.end());
        std::vector<FaultEvent> runEvents;
        for (int j = 0; j < 3; ++j) {
            FaultEvent& event = events[static_cast<std::size_t>(3 * r + j)];
            event.step = steps[static_cast<std::size_t>(j)];
            fillEventDetail(event, layouts[static_cast<std::size_t>(r)], runRandom);
            runEvents.push_back(event);
        }
        result.push_back(runEvents);
    }
    return result;
}

std::string faultEventText(const FaultEvent& event) {
    return event.id + " step " + std::to_string(event.step) + " " +
           faultTypeText(event.type) + " target " + targetText(event.target) +
           " | " + detailText(event);
}

}  // namespace ai_hologram
