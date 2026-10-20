// Purpose: Implements the runs.csv header and row, and the
// file-writing helper (plan 3.11, D-044).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. The
// event rows live in csv_output_event.cpp, split off so each file
// stays under about 200 lines. Rows are made only from records;
// this file reads nothing from an agent directly.

#include "experiment/csv_output.h"

#include <fstream>
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

}  // namespace

std::string runCsvHeader() {
    return "run_id,condition,layout_seed,run_seed,outcome,decision_steps,"
           "world_steps,safe_stops,justified_safe_stops,mismatches,"
           "mismatches_without_fault_cause,source_labels_correct,"
           "source_labels_scored,claims,false_alarms,"
           "wrong_accepted_corrections,evaluations,events_scheduled,"
           "events_activated";
}

std::string runCsvRow(const std::string& condition,
                      const std::uint64_t layoutSeed, const int runSeed,
                      const FaultRunResult& result) {
    int activated = 0;
    for (const ActivationRecord& record : result.activations) {
        if (record.status == ActivationStatus::Applied) {
            ++activated;
        }
    }
    std::vector<std::string> fields;
    fields.push_back(std::to_string(layoutSeed) + "-" +
                     std::to_string(runSeed));
    fields.push_back(condition);
    fields.push_back(std::to_string(layoutSeed));
    fields.push_back(std::to_string(runSeed));
    fields.push_back(result.run.outcome);
    fields.push_back(std::to_string(result.decisionSteps));
    fields.push_back(std::to_string(result.run.worldSteps));
    fields.push_back(std::to_string(result.run.safeStops));
    fields.push_back(std::to_string(result.score.justifiedSafeStops));
    fields.push_back(std::to_string(result.score.mismatches));
    fields.push_back(std::to_string(result.score.mismatchesWithoutCause));
    fields.push_back(std::to_string(result.score.labelsCorrect));
    fields.push_back(std::to_string(result.score.labelsScored));
    fields.push_back(std::to_string(result.score.claims));
    fields.push_back(std::to_string(result.score.falseAlarms));
    fields.push_back(std::to_string(result.score.wrongAccepted));
    fields.push_back(std::to_string(result.run.evaluations));
    fields.push_back(std::to_string(result.activations.size()));
    fields.push_back(std::to_string(activated));
    return joinFields(fields);
}

bool writeTextFile(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << text;
    out.flush();
    return out.good();
}

}  // namespace ai_hologram
