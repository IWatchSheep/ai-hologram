// Purpose: Declares CSV output for runs and fault events, and a
// small helper that writes text to a file (plan 3.11, D-044).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. It only
// reads the records made by the run with faults and the scorer; no
// agent file includes this header, and it changes no agent state.

#pragma once

#include "experiment/fault_event.h"
#include "experiment/fault_injector.h"
#include "experiment/fault_run.h"
#include "experiment/scorer.h"

#include <cstdint>
#include <string>

namespace ai_hologram {

// CSV means comma-separated values: one line per record, with the
// fields of each line separated by commas and the first line naming
// the columns. No field ever contains a comma, so no quoting is used.
// An empty field means "this value does not exist"; it does not mean
// zero (D-044). Stage 4 writes no result files into the repository:
// the demonstration prints its CSV lines, and writing the experiment's
// files is Stage 7's job.

std::string runCsvHeader();
std::string eventCsvHeader();

std::string runCsvRow(const std::string& condition,
                      std::uint64_t layoutSeed, int runSeed,
                      const FaultRunResult& result);

std::string eventCsvRow(const std::string& condition,
                        const FaultEvent& event,
                        const ActivationRecord& activation,
                        const EventScore& score);

// Writes text to a file, exactly as given, and reports success.
// Binary mode keeps "\n" as "\n" on every system. No exceptions.
bool writeTextFile(const std::string& path, const std::string& text);

}  // namespace ai_hologram
