// Purpose: Implements the scorer's two phases, claims, mismatch
// causes and source labels, and the life of each fault event
// (plan 3.7 and 3.8). Split from scorer.cpp to stay under about
// 200 lines, as the owner approved.
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// scorer only READS an agent's public state; it never writes to it,
// and no agent file includes this header.

#include "experiment/scorer.h"

namespace ai_hologram {
namespace {

// The element one fault event is about (plan 3.7).
Element elementOfEvent(const FaultEvent& event) {
    Element element;
    if (event.type == FaultType::F1) {
        element.kind = ElementKind::Wall;
        element.cell = event.cell;
    }
    if (event.type == FaultType::F2 || event.type == FaultType::F5) {
        element.kind = ElementKind::Object;
    }
    if (event.type == FaultType::F3) {
        element.kind = ElementKind::ActionModel;
        element.action = event.action;
    }
    if (event.type == FaultType::F4) {
        element.kind = ElementKind::Trap;
        element.cell = event.cell;
    }
    return element;
}

// Two elements are the same when their kinds match and, for Wall
// and Trap, their cells match, or, for ActionModel, their actions
// match. The object is a single position, so it has no cell.
bool sameElement(const Element& left, const Element& right) {
    if (left.kind != right.kind) {
        return false;
    }
    if (left.kind == ElementKind::Wall || left.kind == ElementKind::Trap) {
        return left.cell == right.cell;
    }
    if (left.kind == ElementKind::ActionModel) {
        return left.action == right.action;
    }
    return true;
}

// The ground-truth source of one fault type (plan 3.8): value
// faults in the belief are S2; faults in the model or trap
// analysis are S1.
SourceLabel sourceOf(const FaultType type) {
    if (type == FaultType::F3 || type == FaultType::F4) {
        return SourceLabel::S1;
    }
    return SourceLabel::S2;
}

const CorrectionRecord* findRecord(const ScoredView& view, const int id) {
    if (view.corrections == nullptr) {
        return nullptr;
    }
    for (const CorrectionRecord& record : *view.corrections) {
        if (record.id == id) {
            return &record;
        }
    }
    return nullptr;
}

// claimValueIsTrue with this scorer's true history and the view's
// mismatch records (empty lists when either is absent, as for C0).
bool valueIsTrue(const CorrectionRecord& record, const ScoredView& view,
                 const WorldState& layout,
                 const std::vector<Observation>* history) {
    const std::vector<Observation> noHistory;
    const std::vector<MismatchRecord> noMismatches;
    const std::vector<Observation>& used = history != nullptr ? *history : noHistory;
    const std::vector<MismatchRecord>& mismatches =
        view.mismatches != nullptr ? *view.mismatches : noMismatches;
    return claimValueIsTrue(record, layout, used, mismatches);
}

}  // namespace

void Scorer::endPhase(const ScoredView& view) {
    const std::size_t count = events_.size();
    std::vector<bool> startActive(count, false);
    for (std::size_t i = 0; i < count; ++i) {
        startActive[i] = scores_[i].activated &&
                         scores_[i].endState == EndState::None &&
                         scores_[i].active;
    }
    // New claims: every correction record not yet seen, in order.
    std::vector<const CorrectionRecord*> newRecords;
    if (view.corrections != nullptr) {
        for (std::size_t i = seenCorrections_; i < view.corrections->size(); ++i) {
            const CorrectionRecord& record = (*view.corrections)[i];
            newRecords.push_back(&record);
            ++run_.claims;
            if (!valueIsTrue(record, view, trueLayout_, trueHistory_)) {
                ++run_.wrongAccepted;
            }
            bool found = false;
            for (std::size_t e = 0; e < count; ++e) {
                if (!startActive[e]) {
                    continue;
                }
                if (!sameElement(elementOfEvent(events_[e]), record.element)) {
                    continue;
                }
                found = true;
                if (!scores_[e].strictStep.has_value()) {
                    scores_[e].strictStep = currentStep_;
                }
            }
            if (!found) {
                ++run_.falseAlarms;
            }
        }
        seenCorrections_ = view.corrections->size();
    }
    // New mismatch records. A mismatch with no fault cause is
    // counted and reported as a possible bug, never assumed
    // impossible (plan 3.8, protocol Phase A 8.4).
    if (view.mismatches != nullptr) {
        for (std::size_t i = seenMismatches_; i < view.mismatches->size(); ++i) {
            const MismatchRecord& record = (*view.mismatches)[i];
            ++run_.mismatches;
            std::vector<int> causes;
            for (const int index : committedCauses_) {
                if (startActive[static_cast<std::size_t>(index)]) {
                    causes.push_back(index);
                }
            }
            if (causes.empty()) {
                ++run_.mismatchesWithoutCause;
                continue;
            }
            ++run_.labelsScored;
            bool correct = false;
            for (const int index : causes) {
                const FaultEvent& event = events_[static_cast<std::size_t>(index)];
                if (record.suspected == sourceOf(event.type)) {
                    correct = true;
                }
                EventScore& score = scores_[static_cast<std::size_t>(index)];
                if (!score.mismatchStep.has_value()) {
                    score.mismatchStep = record.decisionStep;
                }
            }
            if (correct) {
                ++run_.labelsCorrect;
            }
        }
        seenMismatches_ = view.mismatches->size();
    }
    // The life of each activated event.
    for (std::size_t i = 0; i < count; ++i) {
        if (!scores_[i].activated) {
            continue;
        }
        const bool now = activeNow(view, static_cast<int>(i));
        if (scores_[i].endState == EndState::None && scores_[i].active && !now) {
            const Element element = elementOfEvent(events_[i]);
            const CorrectionRecord* lastTrue = nullptr;
            for (const CorrectionRecord* record : newRecords) {
                if (!sameElement(element, record->element)) {
                    continue;
                }
                if (valueIsTrue(*record, view, trueLayout_, trueHistory_)) {
                    lastTrue = record;
                }
            }
            if (lastTrue != nullptr) {
                scores_[i].endState = EndState::Corrected;
                scores_[i].correctingId = lastTrue->id;
            } else if (events_[i].type == FaultType::F3 ||
                       events_[i].type == FaultType::F4) {
                int lastId = 0;
                if (view.corrections != nullptr) {
                    for (const CorrectionRecord& record : *view.corrections) {
                        if (!sameElement(element, record.element)) {
                            continue;
                        }
                        if (record.status != CorrectionStatus::Rejected) {
                            lastId = record.id;
                        }
                    }
                }
                scores_[i].endState = EndState::Corrected;
                scores_[i].correctingId = lastId;
            } else {
                scores_[i].endState = EndState::Overwritten;
            }
            scores_[i].endStep = currentStep_;
            scores_[i].active = false;
        } else if (scores_[i].endState == EndState::Corrected &&
                   scores_[i].correctingId != 0 && now) {
            const CorrectionRecord* record =
                findRecord(view, scores_[i].correctingId);
            if (record != nullptr &&
                record->status == CorrectionStatus::Rejected) {
                // The same event is active again (plan 3.7).
                scores_[i].endState = EndState::None;
                scores_[i].active = true;
                scores_[i].correctingId = 0;
                scores_[i].endStep = 0;
            }
        }
    }
    committedCauses_.clear();
}

}  // namespace ai_hologram
