// Purpose: Defines the plain fault-event types used by the fault-event
// list (plan 3.3).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent
// file may include this header, and no agent ever uses these types.

#pragma once

#include "world/action.h"
#include "world/position.h"

#include <array>
#include <cstdint>
#include <string>

namespace ai_hologram {

// The fault-event list is defined per system and is identical for every
// condition (protocol Phase A 4.2 and Section 13.8). C0 and C1 apply
// every event, whatever its target; the targets (R1, R2 or both) are
// generated now because Stage 6's two-reasoner conditions will need
// them, and they must be fixed when the list is generated.
//
// Value faults (F1, F2 and F5) corrupt one stored belief value once.
// Process faults (F3 and F4) install an ongoing cause in the action
// model or the trap analysis, so they affect every later use until a
// correction ends them (plan 3.7, protocol 12.3).

// The five fault types of protocol Section 14 and plan 3.3.
enum class FaultType { F1, F2, F3, F4, F5 };

// Which reasoner an event is aimed at in two-reasoner conditions.
enum class FaultTarget { R1, R2, Both };

// The direction of an F1 wall flip.
enum class F1Direction { FreeToBlocked, BlockedToFree };

// The direction of an F3 model rotation.
enum class Rotation { Left, Right };

// One scheduled fault event. Fields that do not apply to an event's
// type keep their default values.
struct FaultEvent {
    std::string id;                 // "<layoutSeed>-<runSeed>-<j>", j = 1, 2, 3
    std::uint64_t layoutSeed = 0;   // layout seed of this event's run
    int runSeed = 0;                // run seed of this event's run
    int index = 0;                  // j: 1, 2 or 3 within the run
    int step = 0;                   // scheduled decision step, 1..10
    FaultType type = FaultType::F1; // fault type F1..F5
    FaultTarget target = FaultTarget::R1; // R1, R2 or Both
    bool commonMode = false;        // true exactly when target == Both
    bool hasCell = false;           // F1, F4: false when no candidate cell exists
    Position cell{0, 0};            // F1, F4
    F1Direction direction = F1Direction::FreeToBlocked;   // F1
    std::array<Action, 4> order{};  // F2: the seeded direction order
    Action action = Action::North;  // F3
    Rotation rotation = Rotation::Left;                   // F3
};

}  // namespace ai_hologram
