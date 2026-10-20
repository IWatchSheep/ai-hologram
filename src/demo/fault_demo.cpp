// Purpose: Runs the Stage 4 demonstration of the measuring
// instrument on demonstration material only (plan Section 5).
// Introduced in Stage 4.
// Architecture: This is the measuring instrument working on
// demonstration material only; it is not an experiment and its
// numbers are never compared. Layouts and fault events come from
// src/experiment/, the runs use runWithFaults, and the CSV lines use
// csv_output. Only demonstration seeds 500000-500003 are used.

#include "demo/fault_demo.h"

#include "experiment/csv_output.h"
#include "experiment/fault_events.h"
#include "experiment/fault_run.h"
#include "experiment/layout_generator.h"
#include "world/world_text.h"

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

const AgentSettings kSettings{200, 5, 100, 3};

struct DemoMaterial {
    GeneratedLayout layout500000;
    std::vector<WorldState> layouts;
    std::vector<std::vector<FaultEvent>> events;
};

struct DemoRun {
    std::string condition;
    std::uint64_t layoutSeed = 0;
    int runSeed = 0;
    std::vector<FaultEvent> events;
    FaultRunResult result;
};

DemoMaterial demoMaterial() {
    DemoMaterial material;
    std::vector<RunKey> runs;
    for (std::uint64_t seed = 500000; seed <= 500003; ++seed) {
        const GeneratedLayout generated = generateLayout(seed);
        if (seed == 500000) {
            material.layout500000 = generated;
        }
        for (int runSeed = 1; runSeed <= 2; ++runSeed) {
            runs.push_back({seed, runSeed});
            material.layouts.push_back(generated.world);
        }
    }
    material.events = generateFaultEvents(runs, material.layouts, 4242);
    return material;
}

std::string faultTypeText(const FaultType type) {
    const std::array<std::string, 5> names{"F1", "F2", "F3", "F4", "F5"};
    return names[static_cast<std::size_t>(type)];
}

std::string runId(const DemoRun& run) {
    return std::to_string(run.layoutSeed) + "-" + std::to_string(run.runSeed);
}

DemoRun runC1Demo(const WorldState& world,
                  const std::vector<FaultEvent>& events) {
    DemoRun run;
    run.condition = "C1";
    run.layoutSeed = events[0].layoutSeed;
    run.runSeed = events[0].runSeed;
    run.events = events;
    SingleAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                      kSettings);
    run.result = runWithFaults(world, agent, kSettings, events, true);
    return run;
}

DemoRun runC0Demo(const WorldState& world,
                  const std::vector<FaultEvent>& events) {
    DemoRun run;
    run.condition = "C0";
    run.layoutSeed = events[0].layoutSeed;
    run.runSeed = events[0].runSeed;
    run.events = events;
    NaiveAgent agent(Observation{1, world, std::nullopt, std::nullopt},
                     kSettings);
    run.result = runWithFaults(world, agent, kSettings, events, true);
    return run;
}

std::vector<DemoRun> collectDemoRuns(const DemoMaterial& material) {
    std::vector<DemoRun> runs;
    const std::array<std::size_t, 2> indexes{{1, 6}};
    for (const std::size_t index : indexes) {
        runs.push_back(runC1Demo(material.layouts[index],
                                 material.events[index]));
        runs.push_back(runC0Demo(material.layouts[index],
                                 material.events[index]));
    }
    return runs;
}

void writeLayout(std::ostream& out, const DemoMaterial& material) {
    out << '\n';
    out << "Layout 500000 (found at attempt "
        << material.layout500000.attempts << ", shortest solution "
        << material.layout500000.shortest << " world steps):\n";
    const std::vector<std::string> lines =
        renderWorld(material.layout500000.world);
    for (const std::string& line : lines) {
        out << line << '\n';
    }
}

void writeFaultEvents(std::ostream& out, const DemoMaterial& material) {
    out << '\n';
    out << "Fault events of runs 500000-1 and 500003-1 (fault master seed 4242):\n";
    const std::array<std::size_t, 2> indexes{{0, 6}};
    for (const std::size_t index : indexes) {
        for (const FaultEvent& event : material.events[index]) {
            out << faultEventText(event) << '\n';
        }
    }
}

std::string activationSuffix(const ActivationRecord& record) {
    const std::string text = reportedAs(record);
    if (text == "Activated at its scheduled step") {
        return " (step " + std::to_string(record.applicationStep) + ")";
    }
    if (text == "Activated after deferral") {
        return " (step " + std::to_string(record.applicationStep) +
               ", delay " + std::to_string(activationDelay(record)) + ")";
    }
    if (!record.reason.empty()) {
        return " (" + record.reason + ")";
    }
    return "";
}

void writeDemoRun(std::ostream& out, const DemoRun& run) {
    out << '\n';
    out << "Run " << runId(run) << ", " << run.condition << ": "
        << run.result.run.outcome << " after " << run.result.run.worldSteps
        << " world steps, " << run.result.decisionSteps
        << " decision steps, " << run.result.run.safeStops << " safe stops, "
        << run.result.run.evaluations << " model evaluations\n";
    for (std::size_t i = 0; i < run.events.size(); ++i) {
        const FaultEvent& event = run.events[i];
        const ActivationRecord& record = run.result.activations[i];
        out << "  " << event.id << " " << faultTypeText(event.type) << ": "
            << reportedAs(record) << activationSuffix(record) << '\n';
    }
}

void writeCsvSections(std::ostream& out,
                      const std::vector<DemoRun>& runs) {
    out << '\n';
    out << "runs.csv:\n";
    out << runCsvHeader() << '\n';
    for (const DemoRun& run : runs) {
        out << runCsvRow(run.condition, run.layoutSeed, run.runSeed,
                         run.result)
            << '\n';
    }
    out << '\n';
    out << "events.csv:\n";
    out << eventCsvHeader() << '\n';
    for (const DemoRun& run : runs) {
        for (std::size_t i = 0; i < run.events.size(); ++i) {
            out << eventCsvRow(run.condition, run.events[i],
                               run.result.activations[i],
                               run.result.eventScores[i])
                << '\n';
        }
    }
}

}  // namespace

void runFaultDemo(std::ostream& out) {
    const DemoMaterial material = demoMaterial();
    const std::vector<DemoRun> runs = collectDemoRuns(material);
    out << '\n';
    out << "Stage 4: the measuring instrument, on demonstration material (not an experiment)\n";
    out << "Faults are injected into a reasoner's beliefs or reasoning, never into the world.\n";
    out << "Demonstration seeds 500000-500003 only; no pilot or confirmatory layout is used.\n";
    writeLayout(out, material);
    writeFaultEvents(out, material);
    for (const DemoRun& run : runs) {
        writeDemoRun(out, run);
    }
    writeCsvSections(out, runs);
    out << '\n';
    out << "These lines show the instrument working; they are not results and are never compared.\n";
}

}  // namespace ai_hologram
