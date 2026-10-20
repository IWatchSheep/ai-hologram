// Purpose: Implements the F3 faulty model, the reasoner-parts
// adapters, the activation records and the FaultInjector (plan 3.4-3.5).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// per-type application itself lives in fault_apply.cpp, split off so
// each file stays under about 200 lines. No agent ever calls this code.

#include "experiment/fault_injector.h"
#include "world/rules.h"

#include <utility>

namespace ai_hologram {
namespace {

// Turns an action into an index 0..3 (North, East, South, West).
int faultActionIndex(const Action action) {
    switch (action) {
        case Action::North: return 0;
        case Action::East: return 1;
        case Action::South: return 2;
        case Action::West: return 3;
    }
    return 0;
}

// Left turns counter-clockwise (North->West->South->East->North);
// Right turns clockwise (North->East->South->West->North).
Action rotatedAction(const Action action, const Rotation rotation) {
    const Action left[4] = {Action::West, Action::North, Action::East,
                            Action::South};
    const Action right[4] = {Action::East, Action::South, Action::West,
                             Action::North};
    const std::size_t index =
        static_cast<std::size_t>(faultActionIndex(action));
    if (rotation == Rotation::Left) {
        return left[index];
    }
    return right[index];
}

}  // namespace

ActionModel makeFaultyModel(const FaultedActions& faults) {
    // The table is copied now, so later faults do not change a model
    // that was already handed out.
    return [faults](const WorldState& state, const Action action) {
        const ActionResult standard = applyAction(state, action);
        const std::size_t index =
            static_cast<std::size_t>(faultActionIndex(action));
        if (!faults.faulted[index]) {
            return standard;
        }
        if (standard.outcome != MoveOutcome::Pushed) {
            return standard;
        }
        // The agent moves one cell in the action's direction (onto
        // the object's old cell); the object is predicted one cell
        // further in the ROTATED direction, not the action's.
        const Position agentCell = moved(state.agent, action);
        const Action turned = rotatedAction(action, faults.rotation[index]);
        const Position objectCell = moved(agentCell, turned);
        if (isWall(state, objectCell)) {
            return ActionResult{state, MoveOutcome::Blocked};
        }
        WorldState next = state;
        next.agent = agentCell;
        next.object = objectCell;
        return ActionResult{next, MoveOutcome::Pushed};
    };
}

ReasonerParts partsOf(SingleAgentState& state) {
    return ReasonerParts{&state.belief, &state.agentModel,
                         &state.notDeadOverrides, &state.predictor};
}

ReasonerParts partsOf(NaiveAgentState& state) {
    return ReasonerParts{&state.belief, &state.agentModel,
                         &state.notDeadOverrides, &state.predictor};
}

int activationDelay(const ActivationRecord& record) {
    if (record.status != ActivationStatus::Applied) {
        return 0;
    }
    return record.applicationStep - record.scheduledStep;
}

std::string reportedAs(const ActivationRecord& record) {
    if (record.status == ActivationStatus::Applied) {
        if (record.applicationStep == record.scheduledStep) {
            return "Activated at its scheduled step";
        }
        return "Activated after deferral";
    }
    if (record.status == ActivationStatus::Scheduled) {
        return "Not activated: run ended before the scheduled step";
    }
    if (record.status == ActivationStatus::NotApplicable) {
        return "Not activated: not applicable at the scheduled step";
    }
    return "Not activated: deferred, never applicable";
}

FaultInjector::FaultInjector(std::vector<FaultEvent> events,
                             const std::vector<Observation>* trueHistory)
    : events_(std::move(events)), trueHistory_(trueHistory) {
    for (const FaultEvent& event : events_) {
        ActivationRecord record;
        record.eventId = event.id;
        record.type = event.type;
        record.scheduledStep = event.step;
        records_.push_back(record);
    }
}

const std::vector<ActivationRecord>& FaultInjector::records() const {
    return records_;
}

void FaultInjector::atActivationPoint(ReasonerParts parts,
                                      const int decisionStep) {
    // Events are gone through in list order, every time.
    for (std::size_t i = 0; i < events_.size(); ++i) {
        ActivationRecord& record = records_[i];
        bool shouldTry = false;
        if (record.status == ActivationStatus::Scheduled) {
            if (record.scheduledStep == decisionStep) {
                shouldTry = true;
            }
        }
        if (record.status == ActivationStatus::Pending) {
            if (decisionStep > record.scheduledStep) {
                shouldTry = true;  // A4: F5 waits for its chance
            }
        }
        if (!shouldTry) {
            continue;
        }
        const std::vector<Observation> emptyHistory;
        const std::vector<Observation>& history =
            trueHistory_ != nullptr ? *trueHistory_ : emptyHistory;
        const ApplyOutcome outcome =
            tryApplyFault(events_[i], parts, faults_, history);
        if (outcome.applied) {
            record.status = ActivationStatus::Applied;
            record.applicationStep = decisionStep;
            record.reason = "";
        } else {
            record.reason = outcome.reason;
            if (events_[i].type == FaultType::F5) {
                record.status = ActivationStatus::Pending;
            } else {
                record.status = ActivationStatus::NotApplicable;
            }
        }
    }
}

}  // namespace ai_hologram
