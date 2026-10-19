# Stage 2 — Prediction and Comparison

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Stage** | 2 of the roadmap in *AI_Hologram_Project_Brief_v1.1.txt*, Section 10 |
| **Plan version** | 1.1 |
| **Date** | October 19, 2026 |
| **Status** | **APPROVED by the owner on October 19, 2026**, following ChatGPT's recommendation, with the two v1.1 clarifications. Ready for implementation, beginning with Step 1 (owner commits this plan); then sub-stage 2a only. |
| **Governing specification** | AI_Hologram_Project_Brief_v1.1.txt (approved) |
| **Related** | `docs/EXPERIMENT_1_PROTOCOL.md` — Phase A approved (framework only, not frozen) |
| **Builds on** | Stage 1, complete and tagged `stage-1` (commit `72a11fe`) |

---

## 1. Purpose of Stage 2

From the brief (Section 10, Stage 2):
> Goal: represent a PREDICTED state separately from the OBSERVED state, and compare them. Contents: a Belief/WorldModel that predicts the next state; a comparison function that reports exactly which parts differ (agent position, object position, blocked cell, etc.); tests. Learning focus: separating "what I think" from "what is."

Stage 2 builds the **core of the project's idea** (Brief 1.1 and 2.2): a disagreement between a prediction and an observation is **information**. It adds:
- a **belief**: what a reasoner *thinks* the world is, kept separate from the true world;
- a **world model** that *predicts* the result of an action from a belief, using a replaceable "action model";
- a **budget counter** that every prediction must go through (Brief 11.5.3, Copilot rule 13d);
- a **comparison** that reports exactly how a prediction differs from an observation;
- a **demonstration** showing a correct belief (no mismatch) and a wrong belief (a mismatch, reported precisely).

**What Stage 2 does not do:** nothing in Stage 2 *decides* anything. There is no planning, no choosing actions, no correcting beliefs, no mismatch-source labels (S1–S5), no safe stops, no agents, and no fault injection. Those belong to Stages 3–6. Stage 2 only predicts, compares, and reports.

[FACT] Stage 2 is the code that Phase B of the Experiment 1 protocol will describe (protocol Section 10, "Representations"). Phase B is written **after** Stage 2 is complete, to match this code.

## 2. What Stage 2 must NOT contain

- No planning, search, or choosing of actions. The demo's actions are hand-written, as in Stage 1.
- No belief correction. *(The demo simply replaces the belief with the observation after each step, and says so. This is a demo simplification, not a correction policy; see 5.4.)*
- No mismatch-source labels, safe stops, messages, agents, or reasoners.
- No fault injection, fault schedules, or experiment runner. The demo's wrong belief is written by hand.
- No random numbers.
- No exceptions, assertions that stop the program, or validation frameworks (D-010).
- No inheritance hierarchies. The replaceable action model uses a standard-library function object (4.2), not a class hierarchy.
- No changes to the Stage 1 rules (`applyAction`), world representation, or text format.
- No external libraries, test frameworks, networking, threads, or GUI.

If anything here seems necessary, **stop and ask the owner**.

---

## 3. Concepts and exact behaviour

### 3.1 Belief vs truth
- The **true world** is held only by `Environment` (Stage 1, D-008).
- A **belief** is a separate `WorldState` that a reasoner *thinks* is true. It may be wrong. It is wrapped in its own type, `Belief`, so the code can never accidentally mix "what I think" with "what is".

### 3.2 Prediction
- A **prediction** answers: "if I take action *a* in the world I believe in, what will happen?"
- It is produced by a **world model**, which applies an **action model** to the belief.
- The default action model is the true rules (`applyAction` from Stage 1). A *different* action model can be supplied. *(v1.1)* If an **empty** action model is supplied, the world model falls back to the default, so an empty function is never called (4.2). This is how a faulty action model (fault type F3) will be represented in later stages. In Stage 2 a different model is used **only in tests**.
- A prediction contains: the action, the predicted outcome (`Moved`, `Pushed`, `Blocked`, or `InvalidState`), and the predicted next state.
- Predicting **never changes the belief**.

### 3.3 Budget
- Every prediction costs exactly **one model evaluation** (Brief 11.5.3).
- Predictions are only made through `WorldModel::predict`, which **must** take a `BudgetCounter` and use one evaluation from it.
- If the budget is used up, `predict` returns **no prediction** (an empty `std::optional`). It does not throw, and it does not change the belief or the counter.
- Stage 2 does not decide how big budgets are, or when they reset per step. That is Phase C of the protocol and Stage 3. The counter only offers `tryUse()`, `used()`, `remaining()` and `reset()`.

### 3.4 Comparison
- `compareStates(predicted, observed)` compares two `WorldState`s and reports **every** difference, in this fixed order:
  1. agent position;
  2. object position;
  3. destination position;
  4. wall cells, row by row from the top (y = 0 to 9), left to right within each row (x = 0 to 9).
- For each wall difference it records the cell, whether the prediction had it blocked, and whether the observation had it blocked.
- `comparePrediction(prediction, observedResult)` compares a prediction with what actually happened (the `ActionResult` returned by `Environment::step`): whether the **outcomes** differ, plus the full state comparison.
- A prediction **matches** only if the outcomes are equal **and** there are no state differences.
- Comparison never changes either input, and is safe for any input (it reads walls only with its own 0–9 loop counters, as `renderWorld` does).

### 3.5 Observation in Experiment 1
[FACT] In Experiment 1 the world is fully observed (Brief 11.2), so the observation after a step is the true `WorldState`. Stage 2 therefore compares a predicted `WorldState` with the observed `WorldState`. Partial observation is out of scope (Brief Q9).

---

## 4. Code structure

### 4.1 New and changed files

```
src/
├── main.cpp                         (changed) header line 2; also runs the prediction demos
├── core/project_info.cpp            (changed) currentStage() returns 2
├── model/
│   ├── belief.h                     NEW  Belief: a reasoner's (possibly wrong) WorldState
│   ├── prediction.h                 NEW  Prediction: action, predicted outcome, predicted state
│   ├── budget_counter.h / .cpp      NEW  counts model evaluations; refuses when exhausted
│   └── world_model.h / .cpp         NEW  ActionModel type; WorldModel::predict
├── compare/
│   ├── state_difference.h / .cpp    NEW  compareStates, comparePrediction
│   └── difference_text.h / .cpp     NEW  plain-text description of differences
└── demo/
    └── prediction_demo.h / .cpp     NEW  two hand-written prediction demonstrations

tests/
├── test_suites.h                    (changed) declares the new groups
├── test_main.cpp                    (changed) runs the new groups
├── test_project_info.cpp            (changed) P2 checks currentStage() == 2
├── test_budget_counter.cpp          NEW
├── test_world_model.cpp             NEW
└── test_compare.cpp                 NEW
```

Also changed: `CMakeLists.txt` (new files), `README.md` (status paragraph only), `docs/DECISIONS.md` (new decisions), and the Copilot handoff-notes file (appended notes).

### 4.2 Component descriptions

All new code is in namespace `ai_hologram` and compiled into `ai_hologram_core`.

**`model/belief.h`**
- `struct Belief { WorldState state; };`
- A comment explains why a separate type is used even though it holds a `WorldState`: the compiler then stops "what I think" being passed where "what is" is expected.

**`model/prediction.h`**
- `struct Prediction { Action action; MoveOutcome outcome; WorldState state; };`

**`model/budget_counter.h / .cpp`**
- `class BudgetCounter`:
  - `explicit BudgetCounter(int limit);` — a negative limit is treated as 0 (no exception);
  - `bool tryUse();` — if `used() < limit`, adds 1 and returns `true`; otherwise returns `false` and changes nothing;
  - `int used() const;`
  - `int remaining() const;`
  - `void reset();` — sets the used count back to 0.
- Comment: this is the **only** way a prediction may be paid for (Copilot rule 13d).

**`model/world_model.h / .cpp`**
- `using ActionModel = std::function<ActionResult(const WorldState&, Action)>;` — a comment explains what `std::function` is, and why it is used instead of a class hierarchy (it lets a test, and later a fault, supply a different action model without inheritance).
- `class WorldModel`:
  - `WorldModel();` — uses the true rules (`applyAction`) as its action model;
  - `explicit WorldModel(ActionModel actionModel);` — uses the given model. *(v1.1)* **If the supplied function is empty** (it holds no function, so calling it would throw), the constructor uses the default `applyAction` instead. An empty `std::function` is therefore **never called**, keeping the no-exceptions rule (D-010). A comment explains this;
  - `std::optional<Prediction> predict(const Belief& belief, Action action, BudgetCounter& budget) const;`
    - calls `budget.tryUse()` first; if that fails, returns an empty optional;
    - otherwise applies the action model to `belief.state` and returns the prediction;
    - never changes `belief`.

**`compare/state_difference.h / .cpp`**
- `struct PositionDifference { bool differs; Position predicted; Position observed; };`
- `struct WallDifference { Position cell; bool predictedBlocked; bool observedBlocked; };`
- `struct StateDifference { PositionDifference agent; PositionDifference object; PositionDifference destination; std::vector<WallDifference> walls; };`
- `bool hasDifferences(const StateDifference&);`
- `StateDifference compareStates(const WorldState& predicted, const WorldState& observed);`
- `struct PredictionComparison { MoveOutcome predictedOutcome; MoveOutcome observedOutcome; StateDifference state; };`
- `bool outcomesDiffer(const PredictionComparison&);`
- `bool matches(const PredictionComparison&);` — true only when the outcomes are equal and there are no state differences.
- `PredictionComparison comparePrediction(const Prediction& prediction, const ActionResult& observed);`

**`compare/difference_text.h / .cpp`**
- `std::vector<std::string> describeDifferences(const StateDifference&);` — one line per difference, in the order of 3.4, exactly in these formats:
  - `  - agent: predicted (4, 3), observed (5, 3)`
  - `  - object: predicted (5, 3), observed (6, 3)`
  - `  - destination: predicted (8, 3), observed (7, 3)`
  - `  - wall at (6, 3): predicted blocked, observed free`

  (Two leading spaces, a hyphen, a space; positions written `(x, y)`; walls described as `blocked` or `free`.)

**`demo/prediction_demo.h / .cpp`**
- `void runPredictionDemos(std::ostream& out);` — runs the two demos in Section 5 and prints the note in 5.4.
- A comment at the top states plainly: **the actions and the wrong belief are hand-written; nothing chooses them; this is not fault injection.**

**`main.cpp`**
- Prints the three header lines (5.1), then calls `runScriptedDemo(std::cout)` (Stage 1), then `runPredictionDemos(std::cout)`.

---

## 5. Expected program output

### 5.1 Header lines
```
AI Hologram -- Stage 2
Grid world with prediction and comparison (scripted demonstrations).
No reasoning components are implemented yet.
```

### 5.2 Stage 1 demonstration
Unchanged from `STAGE_01.md` Section 5.3, from the blank line before `Initial world:` to `Steps taken: 5`.

### 5.3 Prediction demonstrations
Both demos use the Stage 1 demo layout and the same five actions (`East` ×5). Each step:
1. predict from the belief, using a fresh `BudgetCounter` with limit 10 per demo;
2. apply the action to a fresh `Environment` for that demo;
3. compare the prediction with the observed result;
4. print the step line and any difference lines;
5. replace the belief with the observed state (5.4).

- **Demo 1:** the belief starts **equal** to the true world.
- **Demo 2:** the belief starts equal to the true world **except** that it wrongly has a wall at (6, 3). That cell is a free floor cell, and the belief is still a valid world.

### 5.4 Demo simplification (must be printed)
After every step the demo replaces the belief with the observed state. This is **only** a demonstration shortcut. **How beliefs are corrected is a Stage 3 decision**, governed by Brief 4.5 and protocol Phase B.

### 5.5 Exact expected output of the `ai_hologram` program

```
AI Hologram -- Stage 2
Grid world with prediction and comparison (scripted demonstrations).
No reasoning components are implemented yet.

Initial world:
##########
#........#
#........#
#..A.O..D#
#........#
#........#
#...##...#
#........#
#........#
##########

Step 1: East -> Moved
Step 2: East -> Pushed
Step 3: East -> Pushed
Step 4: East -> Pushed
Step 5: East -> Blocked

Final world:
##########
#........#
#........#
#......A*#
#........#
#........#
#...##...#
#........#
#........#
##########
Goal reached: yes
Steps taken: 5

Prediction demo 1: correct belief
Step 1: East | predicted Moved, observed Moved | match
Step 2: East | predicted Pushed, observed Pushed | match
Step 3: East | predicted Pushed, observed Pushed | match
Step 4: East | predicted Pushed, observed Pushed | match
Step 5: East | predicted Blocked, observed Blocked | match
Mismatches: 0
Model evaluations used: 5

Prediction demo 2: belief wrongly includes a wall at (6, 3)
Step 1: East | predicted Moved, observed Moved | MISMATCH
  - wall at (6, 3): predicted blocked, observed free
Step 2: East | predicted Pushed, observed Pushed | match
Step 3: East | predicted Pushed, observed Pushed | match
Step 4: East | predicted Pushed, observed Pushed | match
Step 5: East | predicted Blocked, observed Blocked | match
Mismatches: 1
Model evaluations used: 5
Note: in these demos the belief is replaced by the observation after every step.
How beliefs are corrected is decided in Stage 3.
```

This output was produced by simulating the rules exactly as specified in this plan.

### 5.6 An observation from planning this demo (for Phase B) — [OBSERVATION] / [OPEN]

[OBSERVATION] In Demo 2, the wrong wall is detected at **step 1**, before it affects any move. The outcomes match ("Moved"/"Moved"), but comparing the **whole** predicted state with the **whole** observed state reveals the wrong wall at once, because the world is fully observed.

[OPEN] **This matters for Experiment 1.** If every reasoner compares full predicted states with full observations every step, wrong-belief faults about the world (F1 wrong wall, F2 wrong object position, and possibly F5 stale memory) could be detected **immediately and trivially by every condition, including C1**. That would make H1 hard to distinguish from a ceiling effect, and would not test reciprocal reasoning. Phase B of the protocol must therefore decide, before Stage 3 is approved:
- what each reasoner observes and compares, and when (for example, only the cells its prediction depends on, or only outcomes and positions);
- whether belief faults are applied to parts of the belief that observation does not immediately overwrite;
- how F3 (faulty action model) and F4 (faulty consequence reasoning), which observation does **not** reveal directly, are weighted in the design.

Stage 2 does **not** decide this. It provides `compareStates` (full comparison) and `comparePrediction` (outcome plus full state) as neutral tools. Restricting what is compared is a Phase B / Stage 3 design decision, to be reviewed by ChatGPT.

**Agreed constraint (v1.1, from ChatGPT's review):** whatever Phase B decides, the approved Phase A definitions are not changed. **Information must never be withheld from the single-agent baseline to favour the two-agent architecture.** All conditions continue to receive the same information and the same computational budget. Phase B must specify what is compared, when comparisons happen, and how detection opportunities are defined, **before** Stage 3 is approved.

---

## 6. Required correctness tests

Every test world, directly constructed or parsed, **must be valid** (`isValidWorld`) unless the test is specifically about invalid states. *(Lesson from Stage 1, T10.)*

### 6.1 Project info
- P1. `projectName()` returns `"AI Hologram"`.
- P2. `currentStage()` returns `2`.

### 6.2 Budget counter (`test_budget_counter.cpp`)
- BC1. A new counter has `used() == 0` and `remaining() == limit`.
- BC2. `tryUse()` succeeds exactly `limit` times, then returns `false`, and `used()` stays at `limit`.
- BC3. `reset()` returns `used()` to 0, and `tryUse()` succeeds again.
- BC4. A limit of 0 never allows use; a negative limit behaves like 0.

### 6.3 World model (`test_world_model.cpp`)
- WM1. With the default model and a belief equal to the true world, the prediction's outcome and state equal `applyAction` on the true world, for **each** of the four actions.
- WM2. Each successful prediction uses exactly one evaluation.
- WM3. With an exhausted budget, `predict` returns no prediction, and the counter and belief are unchanged.
- WM4. `predict` never changes the belief (compared before and after).
- WM5. A prediction from an **invalid** belief returns outcome `InvalidState` with the state unchanged, and still uses one evaluation.
- WM6. A **custom** action model is actually used: a model that always returns `Blocked` with the state unchanged makes the prediction `Blocked`, even where the true rules would give `Moved`.
- WM7. The prediction records the action that was asked for.
- WM8. *(v1.1)* A `WorldModel` constructed with an **empty** `ActionModel` does not crash, and gives the same predictions as the default model (it falls back to `applyAction`).

### 6.4 Comparison (`test_compare.cpp`)

**Valid fixtures for comparison tests** *(v1.1)*: every `WorldState` passed to `compareStates` or `comparePrediction` in these tests must itself be **valid** under `STAGE_01.md` 3.6. Stage 1's validation rules are **not** weakened to make a fixture work. In particular:
- **Wall differences:** change only an **interior** cell (x and y each from 1 to 8) that is **not** the agent's, object's, or destination's cell. Never change a border cell.
- **Agent, object, or destination differences:** the changed position must be **inside the grid**, on a **non-wall** cell, and must **not** make the agent and object share a cell.
- Each test checks `isValidWorld` on both of its fixtures, so a broken fixture is reported instead of silently testing the wrong thing.
- CM1. Two identical states: no differences, and `hasDifferences` is false.
- CM2. Agent positions differ: only the agent difference is reported, with both positions.
- CM3. Object positions differ: only the object difference is reported.
- CM4. Destination positions differ: only the destination difference is reported.
- CM5. One wall differs, in each direction (predicted blocked/observed free, and the reverse): exactly one wall difference, with the correct cell and flags.
- CM6. Several differences at once (agent + two walls): all reported, walls in row-then-column order (3.4).
- CM7. `comparePrediction` reports differing outcomes, and `matches` is false.
- CM8. `comparePrediction` with equal outcomes and identical states: `matches` is true.
- CM9. `comparePrediction` with equal outcomes but a state difference: `matches` is false. *(This is exactly the Demo 2, step 1 case.)*
- CM10. `describeDifferences` produces exactly the line formats in 4.2, in the order of 3.4.

---

## 7. Implementation order

As in Stage 1: **one sub-stage at a time**, each built, checked by the owner, reviewed by Claude, and committed before the next. **Copilot never commits.** Each sub-stage starts in a **fresh Copilot chat** with a self-contained instruction.

### Step 1 — Preserve the approved plan (owner)
Save the approved plan as `docs/stages/STAGE_02.md`, then commit by name: `Stage 2: add approved Stage 2 plan`. Push.

### Step 2 — Sub-stage 2a: belief, prediction, budget, world model
- Files: `belief.h`, `prediction.h`, `budget_counter.h/.cpp`, `world_model.h/.cpp`, `test_budget_counter.cpp`, `test_world_model.cpp`; register in `CMakeLists.txt`, `test_suites.h`, `test_main.cpp`.
- Tests: BC1–BC4, WM1–WM8. Leave `currentStage()` at 1.
- Commit: `Stage 2a: add belief, prediction, budget counter, and world model`

### Step 3 — Sub-stage 2b: comparison and difference text
- Files: `state_difference.h/.cpp`, `difference_text.h/.cpp`, `test_compare.cpp`; register them.
- Tests: CM1–CM10.
- **Deliberate failure check** (Brief Lesson L7) on the **comparison code**: Claude will give one exact temporary edit after reviewing the code, for example making `compareStates` skip wall differences. The owner confirms that exactly the expected tests fail, undoes the edit, rebuilds, and confirms everything passes. The temporary change is not committed.
- Commit: `Stage 2b: add state comparison and difference descriptions`

### Step 4 — Sub-stage 2c: demonstrations, stage number, documentation
- Files: `prediction_demo.h/.cpp`; update `main.cpp`, `project_info.cpp` (2), test P2 (2), `README.md` status (9.4), `docs/DECISIONS.md` (Section 8).
- Check: `ai_hologram` output matches **5.5 exactly**.
- Commit: `Stage 2c: add prediction demonstrations and update stage number`

### Step 5 — Fresh-build check
Reset the CMake cache, rebuild all, no warnings; output and tests still correct.

### Step 6 — Owner verification gate (Section 10)

### Step 7 — Record, commit, and tag `stage-2` (owner only, after Step 6)

### Running CTest from PowerShell
After **Build → Build Project** (CTest does not rebuild), in the **same terminal tab**:
```
$env:Path = "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\mingw\bin;" + $env:Path
& "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\cmake\win\x64\bin\ctest.exe" --test-dir cmake-build-debug --output-on-failure
```
Or run **All CTest** inside CLion.

---

## 8. Decisions to record in `docs/DECISIONS.md` (sub-stage 2c)

| ID | Decision | Reason | Alternatives considered |
|---|---|---|---|
| D-012 | A reasoner's belief is a separate type, `Belief`, wrapping a `WorldState`; only `Environment` holds the true state. | The compiler then prevents "what I think" being used as "what is" (Brief Stage 2). | Use `WorldState` for both. |
| D-013 | `WorldModel` predicts by applying a replaceable `ActionModel` (`std::function`), defaulting to the true rules `applyAction`. | Lets a faulty action model (F3) be supplied later without a class hierarchy; keeps prediction separate from the true world. | Hard-code `applyAction`; an abstract base class with subclasses. |
| D-014 | Every prediction is paid for through one `BudgetCounter` (one model evaluation each); when exhausted, `predict` returns no prediction instead of throwing. | Enforces equal computational budgets (Brief 11.5.3, rule 13d) and the no-exceptions rule (D-010). | Count predictions separately in each component; throw when over budget. |
| D-015 | `compareStates` reports every difference in a fixed order (agent, object, destination, then walls row by row); a prediction matches only if outcomes are equal and no state differences exist. | A complete and stable report makes mismatches easy to read, test, and log. | Report only the first difference; compare outcomes only. |
| D-016 | The Stage 2 demos replace the belief with the observation after each step, and say so in the output. This is a demonstration shortcut, not a correction policy. | Keeps Stage 2 free of reasoning; belief correction is a Stage 3 decision governed by Brief 4.5 and protocol Phase B. | Implement a correction rule now. |

---

## 9. Documentation changes

### 9.1 Code comments
Every new file has the three-part header (Purpose / "Introduced in Stage 2" / Architecture). Beginner comments explain, where each first appears: `std::function` and why it is used instead of inheritance; `using` type aliases; `std::optional` as a return value meaning "no prediction"; a `const` member function that takes a non-`const` reference parameter (`predict` changes the budget, not the model); and why comparisons never index walls with stored positions.

### 9.2 `README.md` status
Replace the status paragraph with:
> **Current status: Stage 2 — prediction and comparison.** The project contains a 10×10 grid world with tested movement rules, plus a world model that predicts the result of an action from a belief (which may be wrong), a budget counter that every prediction must go through, and a comparison that reports exactly how a prediction differs from what was observed. Hand-written demonstrations show a correct belief and a wrong belief. **No reasoning components exist yet: nothing plans, chooses actions, or corrects beliefs. No experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

### 9.3 `.github/copilot-instructions.md`
Unchanged in Stage 2.

### 9.4 `docs/EXPERIMENT_LOG.md`
Unchanged. Stage 2 runs no experiments.

---

## 10. Stage 2 completion checklist (owner verification gate)

**Approval and history**
- [ ] This plan is approved and was committed before any Stage 2 code.
- [ ] Git history is intact; the gate was passed before the completion commit and the `stage-2` tag.

**Scope**
- [ ] Only the files in 4.1 (plus `CMakeLists.txt`, `README.md`, `docs/DECISIONS.md`, this plan, and the handoff notes) were added or changed.
- [ ] Nothing from Section 2; no file renamed, moved, or deleted; Stage 1 rules unchanged.

**Build, run, test**
- [ ] No warnings, including after a cache reset.
- [ ] `ai_hologram` output matches **5.5 exactly**.
- [ ] `ai_hologram_tests`: all PASS, 0 failures; every test in Section 6 is covered.
- [ ] CTest passes.
- [ ] The deliberate failure check on the comparison code was done; the expected tests failed; everything passed after undoing it.

**Code and documentation**
- [ ] Every prediction goes through `BudgetCounter::tryUse()`; no other code predicts states.
- [ ] No exceptions; no inheritance; functions under ~40 lines; files under ~200 lines (or a recorded reason).
- [ ] Header blocks and beginner comments (9.1).
- [ ] README status matches 9.2; DECISIONS.md contains D-012 to D-016.
- [ ] The owner can explain the items in Section 11.

---

## 11. Things the owner should understand by the end

1. Why a belief is a different type from the true world, even though both hold a `WorldState`.
2. What a prediction is, and why predicting never changes the belief.
3. What `std::function` is, and how it lets a different (possibly faulty) action model be used.
4. Why every prediction must go through the budget counter.
5. Why `predict` returns a `std::optional` instead of throwing when the budget is exhausted.
6. What `compareStates` reports, and in what order.
7. Why a prediction can have the right outcome and still not match (Demo 2, step 1).
8. Why the demo replacing the belief with the observation is not "belief correction".
9. Why the observation in 5.6 matters for designing Experiment 1.

---

## 12. What actually happened (filled in by the owner at the end)

```
Date completed:
Commit tagged stage-2:
Total number of checks in the final test run (N):

Did everything go as planned?  yes / no
Differences from the plan, and why:

Problems encountered and how they were solved:

Anything learned that should change future stage plans:
```

---

## 13. Instructions to GitHub Copilot for this stage

1. Read `.github/copilot-instructions.md`, this plan, and the brief before writing anything.
2. Implement **one sub-stage at a time**, only when the owner asks for it by name.
3. Before creating or changing any file, check whether it exists and read it first.
4. **From the start**:
   - every new file has the three-part header and the beginner comments in 9.1;
   - every function is under about 40 lines (split helpers as needed);
   - every file is under about 200 lines;
   - **no exceptions**, no assertions, no inheritance.
5. Implement Sections 3 and 4 **exactly**. If something is unclear or impossible, **stop and ask**; never weaken a rule or a test to make something pass.
6. **Every test world must be valid** unless the test is about invalid states. Check each fixture with the rules in `STAGE_01.md` 3.6 before using it.
7. Every prediction must go through `BudgetCounter::tryUse()`. Never predict a state any other way.
8. Do not rename, move, or delete any file, and do not use refactoring tools on files. Do not change Stage 1 rules or tests except P2.
9. Do not commit, tag, or push.
10. When finished, **append** a handoff note (rule 2f) saying honestly what was built and run, and tell the owner which files changed and exactly what output to expect. Remind the owner to use **Build → Build Project** before CTest.

---

## Revision history of this plan

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-10-19 | First draft by Claude after Stage 1 was tagged and Phase A of the Experiment 1 protocol was approved. Incorporates Stage 1 lessons: test fixtures must be valid; requirements for comments, function length and no exceptions stated up front; a fresh Copilot chat per sub-stage. Expected output (5.5) produced by simulating the specified rules. Includes an [OBSERVATION]/[OPEN] item (5.6) for Phase B: full-state comparison makes wrong-belief faults visible immediately. For independent review by ChatGPT and approval by the owner. |
| 1.1 | 2026-10-19 | ChatGPT reviewed v1.0 and recommended approval of the architecture; the owner approved. Two clarifications: **(A)** an empty `ActionModel` supplied to `WorldModel` falls back to the default `applyAction`, so an empty `std::function` is never called (no exceptions); documented in 3.2 and 4.2, with new test WM8. **(B)** Comparison-test fixtures must be valid under Stage 1 rules: wall changes only on interior cells away from the agent, object and destination; position changes inside the grid, on non-wall cells, and without agent/object overlap; each test checks `isValidWorld` on its fixtures (6.4). Section 5.6 kept, with ChatGPT's agreed constraint added: Phase A unchanged; no information withheld from the baseline; same information and budget for all conditions; Phase B must settle comparisons before Stage 3 is approved. No architectural change. **Approved by the owner on 2026-10-19.** |
