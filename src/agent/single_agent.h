// Purpose: Declares C1, the competent single agent: its settings, its
// plain public state, and its decision functions (plan 3.6 and 3.9).
// Introduced in Stage 3.
// Architecture: C1 sees the world ONLY through the Observations it is
// given. It never reads Environment (only the run loop touches
// Environment), because its belief must be checked through its own
// predictions, never by looking at the true world directly. Every
// prediction goes through state.predictor, the StepPredictor.

#pragma once

#include "agent/agent_model.h"
#include "agent/dead_squares.h"
#include "agent/investigation.h"
#include "agent/observation.h"
#include "agent/planner.h"
#include "agent/records.h"
#include "agent/step_predictor.h"
#include "agent/verification.h"
#include "model/prediction.h"

#include <optional>
#include <string>
#include <vector>

namespace ai_hologram {

struct AgentSettings {
    int budgetPerStep = 200;   // B
    int safeStopLimit = 5;     // S_max
    int maxWorldSteps = 100;   // T_max
    int activeTestDelay = 3;   // T_v
};

struct Decision {
    bool isAction = false;
    Action action = Action::North;
    std::string stopReason;
};

enum class PlanKind { None, Goal, Fallback, Test };

// The prediction C1 committed to at the last acting step, if any.
struct CommittedPrediction {
    bool has = false;
    Prediction prediction{Action::North, MoveOutcome::Blocked, WorldState{}};
    WorldState beliefBefore{};
    int decisionStep = 0;
    std::vector<int> testedCorrectionIds;
};

// An investigation deferred for a fresh budget (plan 3.4). The history
// snapshot is kept so the retry compares with the same observations.
struct DeferredInvestigation {
    bool has = false;
    WorldState beliefBefore{};
    Action action = Action::North;
    std::vector<Observation> history;
};

struct WaitingTrapTest {
    bool has = false;
    int correctionId = 0;
    Position cell{0, 0};
};

// All of C1's state, as one plain public struct (plan 3.6), so tests
// can set up any state directly. The predictor's model asks this
// state's AgentActionModel for its current model on every call, so a
// restoration takes effect without rebuilding the predictor; because
// that model refers back to this state, copying is not allowed.
struct SingleAgentState {
    SingleAgentState(WorldState startBelief, AgentSettings settings,
                     AgentActionModel model,
                     std::vector<Position> notDead);
    SingleAgentState(const SingleAgentState&) = delete;
    SingleAgentState& operator=(const SingleAgentState&) = delete;

    AgentSettings settings;
    WorldState belief;
    std::vector<Observation> history;
    DeadSquareCache deadCache;
    FreeComputationCounts freeCounts;
    std::vector<Position> notDeadOverrides;
    std::vector<Position> deadOverrides;
    AgentActionModel agentModel;
    StepPredictor predictor;
    std::vector<PlanStep> plan;
    PlanKind planKind = PlanKind::None;
    CommittedPrediction committed;
    std::vector<CorrectionRecord> corrections;
    std::vector<MismatchRecord> mismatches;
    std::optional<ModelV1Work> pendingV1;
    DeferredInvestigation deferred;
    WaitingTrapTest waitingTrap;
    int safeStops = 0;
    int decisionStep = 0;
    int nextCorrectionId = 1;
    int trapAnomalies = 0;
    std::vector<std::string> log;
};

class SingleAgent {
public:
    // The first observation is history[0]. Without a starting belief,
    // the belief is that observation's state. A base model and "not
    // dead" overrides are supplied by tests only.
    SingleAgent(const Observation& first, AgentSettings settings,
                std::optional<WorldState> startingBelief = std::nullopt,
                std::optional<ActionModel> baseModel = std::nullopt,
                std::vector<Position> notDeadOverrides = {});
    SingleAgent(const SingleAgent&) = delete;
    SingleAgent& operator=(const SingleAgent&) = delete;

    Decision decide(const Observation& observation);
    void finalCheck(const Observation& observation);
    const std::vector<std::string>& logLines() const;
    // The cautious chooser of plan 3.9, callable alone in tests.
    std::optional<PlanStep> chooseCautiousAction();

    int mismatchCount() const;
    int correctionCount() const;
    int verifiedCount() const;
    int safeStopCount() const;
    int totalEvaluations() const;

    SingleAgentState state;

private:
    int currentReserve() const;
    const DeadSquareMap& currentMap();
    CorrectionRecord* findCorrection(int id);
    void addLog(const std::string& line);
    Decision makeSafeStop(const std::string& reason);
    void applyCorrection(const CorrectionRecord& correction);
    void revertCorrection(const CorrectionRecord& correction);
    CorrectionRecord& addCorrectionRecord(CorrectionRecord record);

    void checkCommitted(const Observation& observation);
    void handleInvestigation(const InvestigationResult& result,
                             const WorldState& beliefBefore, Action action,
                             const Observation& after, bool logLines,
                             const std::vector<Observation>& historyUsed);
    std::optional<Decision> retryDeferred();
    void continuePendingWork();
    void attemptTrapV2(int correctionId, Position cell);
    bool tryActiveTest(const DeadSquareMap& map, int reserve);
    Decision chooseAndCommit();
    Decision commitAction();
};

}  // namespace ai_hologram
