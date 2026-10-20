// Purpose: Checks SC5: the demonstration experiment scored by
// runWithFaults gives exactly the plan 6.6 values for every event
// and run, for C1 and C0 (opportunityStep is checked in 4e, not 4d).
// Introduced in Stage 4.
// Architecture: Layouts and events come from generateLayout and
// generateFaultEvents (4242), as in FR2; one check per (run,
// condition) for event values and one for run values.

#include "experiment/fault_events.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "experiment/scorer.h"
#include "test_suites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

const AgentSettings kSettings{200, 5, 100, 3};

// End kinds: 0 Active, 1 Corrected, 2 Overwritten, 3 not activated.
// Statuses: -1 none, 0 CONSISTENT, 1 VERIFIED, 2 REJECTED.
// mismatch and strict: -1 means none.
struct ExpectedEvent {
    int activated;
    bool revealing;
    int mismatch;
    int strict;
    int endKind;
    int endStep;
    int status;
};

const std::array<std::array<ExpectedEvent, 3>, 8> kEventsC1 = {{
    {{{4, false, -1, -1, 0, 0, -1}, {6, false, -1, -1, 0, 0, -1},
      {10, false, -1, -1, 0, 0, -1}}},
    {{{2, false, -1, -1, 0, 0, -1}, {9, true, 10, 10, 1, 10, 0},
      {10, false, -1, -1, 0, 0, -1}}},
    {{{4, false, -1, -1, 0, 0, -1}, {6, false, -1, -1, 0, 0, -1},
      {0, false, -1, -1, 3, 0, -1}}},
    {{{2, false, -1, -1, 0, 0, -1}, {4, true, 12, 12, 1, 12, 1},
      {10, false, -1, -1, 0, 0, -1}}},
    {{{3, false, -1, -1, 0, 0, -1}, {6, false, -1, -1, 0, 0, -1},
      {7, false, -1, -1, 0, 0, -1}}},
    {{{5, false, -1, -1, 0, 0, -1}, {8, false, -1, -1, 0, 0, -1},
      {12, true, 15, 15, 1, 15, 1}}},
    {{{9, true, 10, 10, 1, 10, 1}, {2, false, -1, -1, 0, 0, -1},
      {8, false, -1, -1, 0, 0, -1}}},
    {{{1, true, 8, 8, 1, 8, 2}, {9, false, -1, -1, 0, 0, -1},
      {10, true, 12, 12, 1, 12, 1}}},
}};

const std::array<std::array<ExpectedEvent, 3>, 8> kEventsC0 = {{
    {{{4, false, -1, -1, 2, 4, -1}, {6, false, -1, -1, 0, 0, -1},
      {10, false, -1, -1, 0, 0, -1}}},
    {{{2, false, -1, -1, 2, 2, -1}, {9, false, -1, -1, 2, 10, -1},
      {10, false, -1, -1, 0, 0, -1}}},
    {{{4, false, -1, -1, 2, 4, -1}, {6, false, -1, -1, 2, 6, -1},
      {15, false, -1, -1, 2, 15, -1}}},
    {{{2, false, -1, -1, 2, 2, -1}, {4, false, -1, -1, 2, 4, -1},
      {10, false, -1, -1, 0, 0, -1}}},
    {{{3, false, -1, -1, 0, 0, -1}, {6, false, -1, -1, 0, 0, -1},
      {7, false, -1, -1, 2, 7, -1}}},
    {{{5, false, -1, -1, 0, 0, -1}, {8, false, -1, -1, 0, 0, -1},
      {12, false, -1, -1, 2, 12, -1}}},
    {{{11, false, -1, -1, 2, 11, -1}, {2, false, -1, -1, 0, 0, -1},
      {8, false, -1, -1, 2, 8, -1}}},
    {{{1, false, -1, -1, 0, 0, -1}, {9, false, -1, -1, 0, 0, -1},
      {10, false, -1, -1, 0, 0, -1}}},
}};

struct ExpectedRunScore {
    int justified;
    int mismatches;
    int noCause;
    int labelsCorrect;
    int labelsScored;
    int claims;
    int falseAlarms;
    int wrongAccepted;
};

const ExpectedRunScore kZeroRun{0, 0, 0, 0, 0, 0, 0, 0};
const ExpectedRunScore kOneRun{0, 1, 0, 1, 1, 1, 0, 0};

const std::array<ExpectedRunScore, 8> kRunsC1 = {{
    kZeroRun, kOneRun, kZeroRun, kOneRun,
    kZeroRun, kOneRun, kOneRun, {0, 2, 0, 2, 2, 2, 0, 0},
}};

const std::array<ExpectedRunScore, 8> kRunsC0 = {{
    kZeroRun, kZeroRun, kZeroRun, kZeroRun,
    kZeroRun, kZeroRun, kZeroRun, kZeroRun,
}};

bool statusMatches(const EventScore& score, const int status) {
    if (status < 0) {
        return !score.correctionStatus.has_value();
    }
    const CorrectionStatus wanted[3] = {CorrectionStatus::Consistent,
                                        CorrectionStatus::Verified,
                                        CorrectionStatus::Rejected};
    return score.correctionStatus.has_value() &&
           score.correctionStatus.value() == wanted[status];
}

bool optMatches(const std::optional<int>& value, const int wanted) {
    if (wanted < 0) {
        return !value.has_value();
    }
    return value.has_value() && value.value() == wanted;
}

bool eventMatches(const EventScore& score, const ExpectedEvent& wanted) {
    if (wanted.endKind == 3) {
        return !score.activated;
    }
    bool ok = score.activated;
    ok = ok && score.activationStep == wanted.activated;
    ok = ok && score.revealing == wanted.revealing;
    ok = ok && optMatches(score.mismatchStep, wanted.mismatch);
    ok = ok && optMatches(score.strictStep, wanted.strict);
    if (wanted.endKind == 0) {
        ok = ok && score.endState == EndState::None && score.active;
    }
    if (wanted.endKind == 1) {
        ok = ok && score.endState == EndState::Corrected;
        ok = ok && score.endStep == wanted.endStep && !score.active;
    }
    if (wanted.endKind == 2) {
        ok = ok && score.endState == EndState::Overwritten;
        ok = ok && score.endStep == wanted.endStep && !score.active;
    }
    ok = ok && statusMatches(score, wanted.status);
    return ok;
}

bool runMatches(const RunScore& score, const ExpectedRunScore& wanted) {
    bool ok = score.justifiedSafeStops == wanted.justified;
    ok = ok && score.mismatches == wanted.mismatches;
    ok = ok && score.mismatchesWithoutCause == wanted.noCause;
    ok = ok && score.labelsCorrect == wanted.labelsCorrect;
    ok = ok && score.labelsScored == wanted.labelsScored;
    ok = ok && score.claims == wanted.claims;
    ok = ok && score.falseAlarms == wanted.falseAlarms;
    ok = ok && score.wrongAccepted == wanted.wrongAccepted;
    return ok;
}

std::string runName(const int index) {
    return std::to_string(500000 + index / 2) + "-" +
           std::to_string(index % 2 + 1);
}

void checkRun(TestCounter& counter, const int index,
              const WorldState& world,
              const std::vector<FaultEvent>& events) {
    const std::size_t at = static_cast<std::size_t>(index);
    SingleAgent c1(Observation{1, world, std::nullopt, std::nullopt}, kSettings);
    const FaultRunResult r1 = runWithFaults(world, c1, kSettings, events);
    bool ok1 = r1.eventScores.size() == 3;
    for (std::size_t e = 0; e < 3 && ok1; ++e) {
        ok1 = eventMatches(r1.eventScores[e], kEventsC1[at][e]);
    }
    if (!ok1) {
        std::cout << "SC5 C1 " << runName(index) << " event mismatch\n";
    }
    check(counter, ok1, "SC5: run " + runName(index) + " C1 events exact");
    check(counter, runMatches(r1.score, kRunsC1[at]),
          "SC5: run " + runName(index) + " C1 run score exact");
    NaiveAgent c0(Observation{1, world, std::nullopt, std::nullopt}, kSettings);
    const FaultRunResult r0 = runWithFaults(world, c0, kSettings, events);
    bool ok0 = r0.eventScores.size() == 3;
    for (std::size_t e = 0; e < 3 && ok0; ++e) {
        ok0 = eventMatches(r0.eventScores[e], kEventsC0[at][e]);
    }
    if (!ok0) {
        std::cout << "SC5 C0 " << runName(index) << " event mismatch\n";
    }
    check(counter, ok0, "SC5: run " + runName(index) + " C0 events exact");
    check(counter, runMatches(r0.score, kRunsC0[at]),
          "SC5: run " + runName(index) + " C0 run score exact");
}

}  // namespace

void runScorerDemoTests(TestCounter& counter) {
    std::cout << "--- Scorer demonstration ---\n";
    std::vector<RunKey> runs;
    std::vector<WorldState> layouts;
    for (std::uint64_t seed = 500000; seed <= 500003; ++seed) {
        const WorldState world = generateLayout(seed).world;
        for (int runSeed = 1; runSeed <= 2; ++runSeed) {
            runs.push_back({seed, runSeed});
            layouts.push_back(world);
        }
    }
    const std::vector<std::vector<FaultEvent>> events =
        generateFaultEvents(runs, layouts, 4242);
    for (int i = 0; i < 8; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        checkRun(counter, i, layouts[index], events[index]);
    }
}

}  // namespace ai_hologram
