// Purpose: Holds the plain records C1 keeps about mismatches and
// corrections: source labels, correction statuses and the elements a
// correction can name (plan 3.7 and 3.8).
// Introduced in Stage 3.
// Architecture: These are only records. They change nothing by
// themselves; sub-stage 3e will create them, apply corrections and
// move corrections between statuses when it runs the decision step.

#pragma once

#include "agent/footprint.h"
#include "world/action.h"
#include "world/position.h"

#include <vector>

namespace ai_hologram {

// Where a mismatch is suspected to have come from (brief 4.4). The
// labels keep the possible sources separate, so a mismatch is never
// automatically blamed on reasoning. C1 in Experiment 1 uses only
// S1, S2 and S5: S3 (the world's rules themselves differ) and S4
// (the observation was wrong) exist in this list but C1 never uses
// them, because the plan says they should not occur (plan 3.7 A.6).
enum class SourceLabel { S1, S2, S3, S4, S5 };

// How far a correction has been verified (plan 3.8). A correction is
// not accepted just because it fits past observations (brief 4.5):
// CONSISTENT means it passed the history checks (V1); VERIFIED means
// it also passed an independent test (V2); REJECTED means a check
// failed and the old value is restored where that is possible.
enum class CorrectionStatus { Consistent, Verified, Rejected };

// The kind of belief element a correction is about.
enum class ElementKind { Wall, Object, ActionModel, Trap };

// One belief element. Which other field matters depends on `kind`:
// - Wall and Trap use `cell` (the cell whose status is in question);
// - ActionModel uses `action` (the action whose rule is in question);
// - Object uses neither: the object is a single position, carried by
//   the record's old and new positions below.
struct Element {
    ElementKind kind = ElementKind::Wall;
    Position cell{0, 0};
    Action action = Action::North;
};

// A record of one proposed repair to the belief (plan 3.8). It keeps
// both the new value and the old value, so a rejected correction can
// put the old value back. Only the pair belonging to the element is
// used: `newBlocked`/`oldBlocked` for a wall, `newPosition`/
// `oldPosition` for the object. An action-model correction's new
// value is always "the standard rule" and a trap correction's new
// value is always "dead", so those need no value fields.
struct CorrectionRecord {
    int id = 0;                    // 1, 2, 3, ... per run
    Element element{};             // what this correction is about
    bool newBlocked = false;       // wall only: believed blocked now
    bool oldBlocked = false;       // wall only: blocked before it
    Position newPosition{0, 0};    // object only: position now
    Position oldPosition{0, 0};    // object only: position before it
    CorrectionStatus status = CorrectionStatus::Consistent;
    int stepConsistent = 0;        // decision step it became Consistent
    bool tested = false;           // has a V2 test been made for it?
};

// A record of one mismatch between prediction and observation
// (plan 3.7). It stores what differed, which candidate elements were
// considered, and which source is suspected. `anomaly` flags a
// believed agent position that differs from the record, which the
// plan treats as a possible bug rather than an ordinary fault.
struct MismatchRecord {
    int decisionStep = 0;                 // step whose check failed
    Action action = Action::North;        // the action that was taken
    FootprintComparison differences{};    // the footprint differences
    std::vector<Element> candidates;      // elements considered
    SourceLabel suspected = SourceLabel::S5;  // suspected source
    bool anomaly = false;                 // believed agent != record
};

}  // namespace ai_hologram
