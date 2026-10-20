// Purpose: Declares generation of the fault-event list for a whole
// experiment and the exact text form of one fault event (plan 3.3).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent
// file may include this header, and no agent ever calls these functions.

#pragma once

#include "experiment/fault_event.h"
#include "world/world_state.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ai_hologram {

// One run of an experiment, identified by its layout seed and run seed.
// Runs are given in the order layout seed ascending, then run seed
// ascending (plan 3.3).
struct RunKey {
    std::uint64_t layoutSeed = 0; // seed that generated this run's layout
    int runSeed = 0;              // run seed within that layout
};

// The number of common-mode events for a type with this many events:
// round-half-up of count / 5 (20% of events, per type, Section 14),
// computed as (2 * count + 5) / 10 in integer arithmetic.
int commonModeCount(int count);

// The seed of one run's own random stream. The fault master seed 4242
// is fixed before any confirmatory run (D-032), but here it is only a
// parameter, so tests can also use other master seeds.
std::uint64_t runStreamSeed(std::uint64_t masterSeed,
                            std::uint64_t layoutSeed, int runSeed);

// Generates the fault-event list once for a whole experiment. The list
// is made once for the whole experiment, not run by run, because the
// type mix, the 20% common-mode shares and the halves for targets and
// F1 directions are exact only over the whole experiment (protocol
// Section 14 and 13.8). layouts[r] is the TRUE layout of runs[r]; the
// two lists must have the same length, and an empty list is returned
// if they do not. The result has one vector of exactly 3 events per
// run, in run order.
std::vector<std::vector<FaultEvent>> generateFaultEvents(
    const std::vector<RunKey>& runs,
    const std::vector<WorldState>& layouts, std::uint64_t masterSeed);

// The exact one-line text of one event, in the form the tests compare:
// "<id> step <step> <F1..F5> target <R1|R2|both> | <details>".
std::string faultEventText(const FaultEvent& event);

}  // namespace ai_hologram
