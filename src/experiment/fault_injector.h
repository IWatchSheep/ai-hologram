// Purpose: Declares fault injection: applying a scheduled fault to a
// reasoner, the F3 faulty model, and the activation records (plan 3.4-3.5).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent
// file may include this header. The injector changes a reasoner's
// belief or reasoning only; it never touches the true world, never
// charges any budget, and the agent never reads anything from it.

#pragma once

#include "agent/agent_model.h"
#include "agent/naive_agent.h"
#include "agent/observation.h"
#include "agent/single_agent.h"
#include "agent/step_predictor.h"
#include "experiment/fault_event.h"
#include "model/world_model.h"
#include "world/world_state.h"

#include <array>
#include <string>
#include <vector>

namespace ai_hologram {

// Applying a fault means changing what one reasoner believes or how
// it reasons, never the world itself. A value fault (F1, F2, F5)
// corrupts one stored belief value once. A process fault (F3, F4)
// installs an ongoing cause: F3 in the action model, F4 in the trap
// analysis overrides, so it affects every later use until a correction
// ends it (plan 3.4, protocol 12.2-12.3).
//
// Amendment A3 in plain words: an impossible belief is never created.
// If a fault would put a wall under the believed agent or object, or
// (F5) put the remembered object on the believed agent square or on a
// believed wall, the fault is not applied at that step. Amendment A4:
// an F5 fault that cannot be applied waits (stays pending) and is
// tried again at every later step; F1-F4 faults that cannot be applied
// lapse for good. After any applied fault the prediction cache is
// emptied (D-036), because the belief or the model has changed, so a
// cached prediction may no longer be what the reasoner would predict.

// Which actions are faulted (F3), and how each one is rotated.
// Indexed by actionIndex: North 0, East 1, South 2, West 3.
struct FaultedActions {
    std::array<bool, 4> faulted{};
    std::array<Rotation, 4> rotation{};
};

// Builds the faulty action model for a table of F3 faults (plan 3.4).
ActionModel makeFaultyModel(const FaultedActions& faults);

// The parts of one reasoner that the injector is allowed to change.
// This is a small bundle of pointers, not inheritance: C1 and C0 each
// have the same four parts, named in their own state structs.
struct ReasonerParts {
    WorldState* belief;
    AgentActionModel* model;
    std::vector<Position>* notDeadOverrides;
    StepPredictor* predictor;
};

// Points the parts at the corresponding members of one agent's state.
ReasonerParts partsOf(SingleAgentState& state);
ReasonerParts partsOf(NaiveAgentState& state);

// Whether one fault was applied, and the exact reason when it was not.
struct ApplyOutcome {
    bool applied = false;
    std::string reason;
};

// Tries to apply one event to one reasoner (plan 3.4, exactly).
// trueHistory holds the TRUE observations of the run so far, including
// the current one; F5 reads the object's previous square from it, but
// A3's checks use this reasoner's own belief. On any applied fault the
// prediction cache is emptied and nothing else is changed.
ApplyOutcome tryApplyFault(const FaultEvent& event, ReasonerParts parts,
                           FaultedActions& faults,
                           const std::vector<Observation>& trueHistory);

// The life of one event for one reasoner (plan 3.5).
enum class ActivationStatus { Scheduled, Applied, NotApplicable, Pending };

struct ActivationRecord {
    std::string eventId;
    FaultType type = FaultType::F1;
    int scheduledStep = 0;
    ActivationStatus status = ActivationStatus::Scheduled;
    int applicationStep = 0;  // when Applied
    std::string reason;  // NotApplicable: the reason; Pending: latest reason
};

// applicationStep - scheduledStep when Applied, otherwise 0.
int activationDelay(const ActivationRecord& record);

// Exactly one of the five reporting classes of protocol 13.12.
std::string reportedAs(const ActivationRecord& record);

// Applies the scheduled faults of one run to one reasoner. One
// injector exists per reasoner per run, and it is experimenter code:
// the agent never reads it, it only calls the activation hook that
// the run loop connected to atActivationPoint. C0 and C1 apply every
// event, whatever its target (protocol 13.8).
class FaultInjector {
public:
    FaultInjector(std::vector<FaultEvent> events,
                  const std::vector<Observation>* trueHistory);

    // Called once per decision step at the fault activation point.
    void atActivationPoint(ReasonerParts parts, int decisionStep);

    // One record per event, in event order.
    const std::vector<ActivationRecord>& records() const;

private:
    std::vector<FaultEvent> events_;
    std::vector<ActivationRecord> records_;
    const std::vector<Observation>* trueHistory_;
    FaultedActions faults_;
};

}  // namespace ai_hologram
