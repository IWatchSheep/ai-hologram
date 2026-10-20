// Purpose: Implements the events.csv header and row (plan 3.11,
// D-044), split off from csv_output.cpp so each file stays under
// about 200 lines.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. An
// event that was not activated has empty measurement fields, not
// zeros: empty means "did not happen", while 0 would look like a
// measured miss (D-044).

#include "experiment/csv_output.h"

#include <array>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

std::string joinFields(const std::vector<std::string>& fields) {
    std::string row;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) {
            row += ",";
        }
        row += fields[i];
    }
    return row;
}

std::string faultTypeText(const FaultType type) {
    const std::array<std::string, 5> names{"F1", "F2", "F3", "F4", "F5"};
    return names[static_cast<std::size_t>(type)];
}

std::string eventTargetText(const FaultTarget target) {
    if (target == FaultTarget::Both) {
        return "both";
    }
    if (target == FaultTarget::R1) {
        return "R1";
    }
    return "R2";
}

std::string optionalStep(const std::optional<int>& step) {
    if (!step.has_value()) {
        return "";
    }
    return std::to_string(step.value());
}

std::string yesNo(const bool value) {
    return value ? "1" : "0";
}

std::string endStateText(const EndState endState) {
    if (endState == EndState::Corrected) {
        return "Corrected";
    }
    if (endState == EndState::Overwritten) {
        return "Overwritten without correction";
    }
    return "Active at end of run";
}

std::string correctionStatusText(const EventScore& score) {
    if (score.endState != EndState::Corrected) {
        return "";
    }
    if (!score.correctionStatus.has_value()) {
        return "";
    }
    if (score.correctionStatus.value() == CorrectionStatus::Consistent) {
        return "CONSISTENT";
    }
    if (score.correctionStatus.value() == CorrectionStatus::Verified) {
        return "VERIFIED";
    }
    return "REJECTED";
}

void addEventDetails(std::vector<std::string>& fields,
                     const FaultEvent& event) {
    std::string cell;
    if ((event.type == FaultType::F1 || event.type == FaultType::F4) &&
        event.hasCell) {
        cell = std::to_string(event.cell.x) + ":" +
               std::to_string(event.cell.y);
    }
    fields.push_back(cell);
    std::string direction;
    if (event.type == FaultType::F1) {
        direction = event.direction == F1Direction::FreeToBlocked
                        ? "free->blocked"
                        : "blocked->free";
    }
    fields.push_back(direction);
    std::string order;
    if (event.type == FaultType::F2) {
        order = actionName(event.order[0]) + " " + actionName(event.order[1]) +
                " " + actionName(event.order[2]) + " " +
                actionName(event.order[3]);
    }
    fields.push_back(order);
    std::string action;
    std::string rotation;
    if (event.type == FaultType::F3) {
        action = actionName(event.action);
        rotation = event.rotation == Rotation::Left ? "left" : "right";
    }
    fields.push_back(action);
    fields.push_back(rotation);
}

void addNotActivatedFields(std::vector<std::string>& fields,
                           const ActivationRecord& activation) {
    fields.push_back("");
    fields.push_back("");
    std::string reason;
    if (activation.status == ActivationStatus::NotApplicable ||
        activation.status == ActivationStatus::Pending) {
        reason = activation.reason;
    }
    fields.push_back(reason);
    for (int i = 0; i < 12; ++i) {
        fields.push_back("");
    }
}

void addActivatedFields(std::vector<std::string>& fields,
                        const ActivationRecord& activation,
                        const EventScore& score) {
    fields.push_back(std::to_string(activation.applicationStep));
    fields.push_back(std::to_string(activationDelay(activation)));
    fields.push_back("");
    fields.push_back(yesNo(score.opportunityStep.has_value()));
    fields.push_back(optionalStep(score.opportunityStep));
    fields.push_back(yesNo(score.revealing));
    fields.push_back(yesNo(score.mismatchStep.has_value()));
    fields.push_back(optionalStep(score.mismatchStep));
    fields.push_back(yesNo(score.strictStep.has_value()));
    fields.push_back(optionalStep(score.strictStep));
    std::string delay;
    if (score.strictStep.has_value()) {
        delay = std::to_string(score.strictStep.value() -
                               activation.applicationStep);
    }
    fields.push_back(delay);
    fields.push_back(endStateText(score.endState));
    std::string endStep;
    if (score.endState != EndState::None) {
        endStep = std::to_string(score.endStep);
    }
    fields.push_back(endStep);
    fields.push_back(correctionStatusText(score));
    bool verified = false;
    if (score.correctionStatus.has_value()) {
        verified = score.correctionStatus.value() == CorrectionStatus::Verified;
    }
    fields.push_back(yesNo(verified));
}

}  // namespace

std::string eventCsvHeader() {
    return "event_id,condition,reasoner,layout_seed,run_seed,type,target,"
           "common_mode,cell,direction,order,action,rotation,scheduled_step,"
           "reported_as,activation_step,activation_delay,"
           "not_activated_reason,opportunity,opportunity_step,"
           "revealing_opportunity,mismatch_detected,mismatch_step,"
           "strictly_detected,strict_step,detection_delay,end_state,"
           "end_step,correction_status,verified_correction";
}

std::string eventCsvRow(const std::string& condition,
                        const FaultEvent& event,
                        const ActivationRecord& activation,
                        const EventScore& score) {
    std::vector<std::string> fields;
    fields.push_back(event.id);
    fields.push_back(condition);
    fields.push_back("R");
    fields.push_back(std::to_string(event.layoutSeed));
    fields.push_back(std::to_string(event.runSeed));
    fields.push_back(faultTypeText(event.type));
    fields.push_back(eventTargetText(event.target));
    fields.push_back(yesNo(event.commonMode));
    addEventDetails(fields, event);
    fields.push_back(std::to_string(event.step));
    fields.push_back(reportedAs(activation));
    if (activation.status != ActivationStatus::Applied) {
        addNotActivatedFields(fields, activation);
    } else {
        addActivatedFields(fields, activation, score);
    }
    return joinFields(fields);
}

}  // namespace ai_hologram
