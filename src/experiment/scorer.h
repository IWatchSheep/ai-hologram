// Purpose: Declares the scorer (the referee): phases, claims, the
// life of each fault event, the committed-prediction shadow,
// mismatch causes and source labels (plan 3.7, 3.8 and 3.9).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// scorer only READS an agent's public state; it never writes to it,
// and no agent file includes this header. The referee never touches
// the dog: every pointer below is to const data or used read-only.

#pragma once

#include "agent/naive_agent.h"
#include "agent/records.h"
#include "agent/single_agent.h"
#include "experiment/fault_event.h"
#include "experiment/fault_injector.h"
#include "world/world_state.h"

#include <optional>
#include <vector>

namespace ai_hologram {

// A read-only view of one reasoner (plan 4.2). For C0 the last
// three pointers are null: C0 keeps no corrections, no mismatches
// and no committed prediction, so those scoring steps do nothing.
struct ScoredView {
    const WorldState* belief = nullptr;
    const AgentActionModel* model = nullptr;
    const std::vector<Position>* notDeadOverrides = nullptr;
    const std::vector<Position>* deadOverrides = nullptr;
    const std::vector<CorrectionRecord>* corrections = nullptr;
    const std::vector<MismatchRecord>* mismatches = nullptr;
    const CommittedPrediction* committed = nullptr;
};

ScoredView viewOf(const SingleAgentState& state);
ScoredView viewOf(const NaiveAgentState& state);

// How one activated event ended (plan 3.7). None with activated
// true means "Active at end of run"; None with activated false
// means the event was never activated.
enum class EndState { None, Corrected, Overwritten };

// The scored life of one fault event.
struct EventScore {
    bool activated = false;
    int activationStep = 0;
    bool active = false;
    EndState endState = EndState::None;
    int endStep = 0;
    int correctingId = 0;
    std::optional<CorrectionStatus> correctionStatus;
    std::optional<int> opportunityStep;
    bool revealing = false;
    std::optional<int> mismatchStep;
    std::optional<int> strictStep;
};

// The scored facts of one whole run.
struct RunScore {
    int claims = 0;
    int falseAlarms = 0;
    int wrongAccepted = 0;
    int justifiedSafeStops = 0;
    int mismatches = 0;
    int mismatchesWithoutCause = 0;
    int labelsCorrect = 0;
    int labelsScored = 0;
};

// Whether the value claimed by one correction record is the true
// value (plan 3.8). An object claim is judged at the moment it is
// about (D-042, the "stale address"): the true object position in
// the observation before the one checked by the latest mismatch.
bool claimValueIsTrue(const CorrectionRecord& record,
                      const WorldState& trueLayout,
                      const std::vector<Observation>& trueHistory,
                      const std::vector<MismatchRecord>& mismatches);

// Scores one run from the agent's own records. Credit comes only
// through records: a belief that becomes true without a correction
// record is "Overwritten", never a detection or a correction.
class Scorer {
public:
    Scorer(std::vector<FaultEvent> events, WorldState trueLayout,
           const std::vector<Observation>* trueHistory);

    // Each decision step has two phases (D-037): phase A runs from
    // the start of decide to the activation point, phase B from
    // there to the end of decide. A claim made in phase A comes
    // before a fault activated at that step; one made in phase B
    // comes after it. endPhaseA is the snapshot before injection.
    void endPhaseA(const ScoredView& view, int decisionStep);
    void afterActivation(const ScoredView& view,
                         const std::vector<ActivationRecord>& records,
                         int decisionStep);
    void afterDecide(const ScoredView& view, const Decision& decision);
    // The final check is dated to the last decision step (D-045).
    void endFinalCheck(const ScoredView& view);
    // Records each Corrected event's final record status.
    void finish(const ScoredView& view);

    // The live view used by the three callbacks below (Stage 4e).
    // The view's pointers stay valid while the agent lives; the
    // scorer only reads through them and never writes.
    void attachView(const ScoredView& view);
    // The watch-only observer callbacks (IE-1 H3/H4). Each looks
    // only at activated, still-active events with no opportunity
    // step yet, and records the first effective use (plan 3.9).
    void onEvaluation(const WorldState& state, Action action,
                      const ActionModel* variantModel,
                      const Prediction& result);
    void onDeadRead(Position cell, bool value);
    void onHeuristic(const WorldState& state, int value);
    int observedEvaluations() const;  // counts onEvaluation calls (IE-T3)

    const std::vector<EventScore>& events() const;
    const RunScore& run() const;

private:
    bool activeNow(const ScoredView& view, int index) const;
    bool committedEffective(int index, const WorldState& pre,
                            Action action, const WorldState& resultState,
                            MoveOutcome resultOutcome,
                            const ScoredView& view) const;
    void endPhase(const ScoredView& view);

    std::vector<FaultEvent> events_;
    std::vector<EventScore> scores_;
    WorldState trueLayout_;
    const std::vector<Observation>* trueHistory_;
    RunScore run_;
    int currentStep_ = 0;
    ScoredView liveView_;
    bool hasLiveView_ = false;
    int observedEvaluations_ = 0;
    std::size_t seenCorrections_ = 0;
    std::size_t seenMismatches_ = 0;
    std::vector<int> committedCauses_;
};

}  // namespace ai_hologram
