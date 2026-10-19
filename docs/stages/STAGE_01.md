# Stage 1 — The Grid World

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Stage** | 1 of the roadmap in *AI_Hologram_Project_Brief_v1.1.txt*, Section 10 |
| **Plan version** | 1.4 (v1.3 plus one test-specification correction) |
| **Date** | October 19, 2026 |
| **Status** | **APPROVED by the owner on October 19, 2026**, following ChatGPT's recommendation of approval. Ready for implementation, beginning with Step 1 (owner). |
| **Governing specification** | AI_Hologram_Project_Brief_v1.1.txt (approved October 19, 2026) |
| **Builds on** | Stage 0, complete and tagged `stage-0` (commit `78c0a7b`) |

---

## 1. Purpose of Stage 1

Stage 1 builds the **world** that later stages will reason about: a 10×10 grid with walls, one agent, one movable object, and one destination. It implements only the world's **rules** — what happens when the agent tries to move.

From the brief (Section 10, Stage 1):
> Goal: a 10x10 grid environment with walls, one agent, one movable object, and one destination. Pure environment — no reasoning.

When Stage 1 is complete, the owner should be able to:
- describe a world as 10 lines of text and have the program read it;
- apply one action and see exactly what happened (moved, pushed, or blocked);
- print the world as text;
- run tests that prove each movement rule behaves as written below.

**Why this matters for the research:** Stage 2 will separate the *predicted* state from the *observed* state. That comparison is only meaningful if the true world's rules are simple, exact, and tested. Stage 1 provides that ground truth.

## 2. What Stage 1 must NOT contain

- No reasoning: no planning, search, beliefs, predictions, or comparisons.
- No agents in the research sense. The "agent" in Stage 1 is only a **position on the grid** (the body later reasoners will control). Nothing in Stage 1 decides which action to take.
- No fault injection, budgets, messages, or experiment runner.
- No random numbers. All layouts and action sequences are written by hand.
- No external libraries, test frameworks, networking, threads, or GUI.
- No trap detection. Some test layouts may contain traps (an object pushed into a corner), but Stage 1 code does not recognise them. It only applies the rules.
- No validation framework, exceptions, or assertions that stop the program. Validity is handled by the single check described in 3.6. *(v1.1)*

If anything in this list seems necessary, **stop and ask the owner**.

---

## 3. The world: exact rules

These rules are the specification. The code must implement them exactly, and the tests in Section 6 check each one.

### 3.1 Grid and coordinates

- The grid is **10 columns × 10 rows**.
- A position is written **(x, y)**:
  - `x` is the **column**, 0 to 9, counting **left to right**;
  - `y` is the **row**, 0 to 9, counting **top to bottom** (row 0 is the top line of the text).
- Directions:
  - **North** = up = y − 1
  - **South** = down = y + 1
  - **East** = right = x + 1
  - **West** = left = x − 1

### 3.2 What the world contains

| Thing | How many | Notes |
|---|---|---|
| Walls | any number | **Every cell on the outer border is a wall.** Internal walls are allowed. Walls never move. |
| Agent | exactly 1 | Always on a non-wall cell. |
| Object | exactly 1 | Always on a non-wall cell, never on the same cell as the agent. |
| Destination | exactly 1 | A non-wall cell. The agent and the object may both stand on it. |

### 3.3 Actions

There are exactly four actions: **North, East, South, West.**

There is no "wait" or "do nothing" action in Stage 1. *(The safe stop described in the brief, Section 4.2, means a reasoner chooses not to call the environment at all. It belongs to later stages and needs no environment action.)*

### 3.4 Movement rules

Let **T** be the cell next to the agent in the chosen direction, and **B** the cell beyond T in the same direction.

| Situation | Result | Outcome name |
|---|---|---|
| T is a wall | Nothing changes. | **Blocked** |
| T is free (no wall, no object) | The agent moves to T. | **Moved** |
| T contains the object, and B is free (no wall) | The object moves to B; the agent moves to T. | **Pushed** |
| T contains the object, and B is a wall | Nothing changes. | **Blocked** |

Additional rules:
- The agent can only **push**, never pull. Moving away from the object leaves the object where it is.
- The agent can push only **one** object one cell at a time (there is only one object).
- Walls never change.
- The destination never changes and never blocks movement.
- **The goal is reached** when the object is on the destination. Reaching the goal does not stop the rules from working; further actions still apply normally.
- Every attempted action counts as **one step**, including blocked ones.

Because the outer border is always walls, nothing can ever move outside the grid.

### 3.5 Text format for worlds

A world is written as **exactly 10 lines of exactly 10 characters**:

| Character | Meaning |
|---|---|
| `#` | wall |
| `.` | empty floor |
| `A` | agent |
| `O` | object |
| `D` | destination (empty) |
| `*` | object on the destination |
| `+` | agent on the destination |

The same characters are used to **read** a world (parsing) and to **print** it (rendering). A world that is read and then printed must give back exactly the original text.

**A text world is rejected** (with a plain-language error message) if:
- it does not have exactly 10 lines, or any line is not exactly 10 characters;
- any border cell is not `#`;
- it does not contain exactly one agent (`A` or `+`), exactly one object (`O` or `*`), and exactly one destination (`D`, `*`, or `+`);
- it contains any other character.

### 3.6 Valid world states and safety (*added in v1.1*)

A `WorldState` can be created two ways: by **parsing text** (3.5), or **directly in code** (for example, in tests, or later as a reasoner's belief). Both must follow the same definition of "valid".

**A `WorldState` is valid when all of these are true:**
1. The agent, object, and destination positions are all **inside the grid** (x and y each from 0 to 9).
2. **Every border cell is a wall.**
3. The agent, object, and destination are each on a **non-wall** cell.
4. The agent and the object are on **different** cells.

**One definition, one function:** `isValidWorld(const WorldState&)` checks exactly these four rules. `parseWorld` calls it as its final check, so a parsed world is always valid, and there is no second copy of the rules to drift out of step.

**How the core functions handle their preconditions (kept deliberately simple):**
- **`isWall(state, position)`** returns `true` for any position **outside the grid**, and only reads the `walls` array after confirming the position is inside it. Outside the grid behaves like solid wall, so this function can never read outside the array.
- **`applyAction(state, action)`** first calls `isValidWorld`. If the state is **not** valid, it returns the state **unchanged** with the outcome **`InvalidState`**, and applies no rules. It never crashes and never reads outside the array, whatever it is given.
  - This is not an error-handling framework. It's one check at the top of one function.
  - It's useful later: a faulty belief (Brief fault types F1, F2) could describe an impossible world, and `InvalidState` reports that clearly instead of producing a misleading move.
- **`Environment`** is meant to be created with a valid state, normally one from `parseWorld`. If it is given an invalid one, its `step()` simply returns `InvalidState` through `applyAction`, and the step is still counted (every attempted action counts, 3.4).
- **`renderWorld(state)`** *(added in v1.3)* **requires a valid `WorldState`.** It is called only with states that are known to be valid: states from `parseWorld`, or from `applyAction`/`Environment`, which never produce an invalid state from a valid one. It loops only over the 100 grid cells (x and y from 0 to 9), reads `walls[y][x]` only for those cells, and only **compares** the agent, object and destination positions with each cell. It never uses them to index the array. So even if it were given an invalid state by mistake, it could not read outside the array; the picture would simply be wrong (for example, an agent outside the grid would not appear). No extra checking code is added.
- **`parseWorld`** checks the number of lines **before** reading any line, and the length of each line **before** reading its characters. Every loop is bounded by the grid size. Empty input, too few or too many lines, and short or long lines are all rejected safely with an error message.

---

## 4. Code structure

### 4.1 New folder layout

Stage 1 adds a `world/` component and a `demo/` component to the core library, and splits the tests into several files.

```
src/
├── main.cpp                       (changed) still short; calls the demo
├── core/
│   ├── project_info.h             (unchanged)
│   └── project_info.cpp           (changed) currentStage() returns 1
├── world/
│   ├── position.h                 NEW  Position struct and comparison
│   ├── action.h / action.cpp      NEW  the four actions, names, and direction offsets
│   ├── world_state.h / .cpp       NEW  everything the world contains, plus helper questions
│   ├── rules.h / rules.cpp        NEW  applyAction(): the movement rules of Section 3.4
│   ├── world_text.h / .cpp        NEW  parse text into a world; render a world as text
│   └── environment.h / .cpp       NEW  the "true world": holds the current state, counts steps
└── demo/
    └── scripted_demo.h / .cpp     NEW  a fixed, hand-written action sequence for the program to show

tests/
├── test_support.h / .cpp          NEW  shared check() helper and counters
├── test_suites.h                  NEW  declares each group of tests
├── test_main.cpp                  (changed) only runs the groups and prints the total
├── test_project_info.cpp          NEW  (the Stage 0 checks, moved here)
├── test_world_text.cpp            NEW
├── test_rules.cpp                 NEW
└── test_environment.cpp           NEW
```

### 4.2 Component descriptions

All new code lives in the `ai_hologram` namespace and is compiled into the existing `ai_hologram_core` library, so both the program and the tests use the same code (the Stage 0 pattern).

**`world/position.h`**
- `struct Position { int x; int y; };`
- Equality comparison (`==` and `!=`) so tests can write `state.agent == Position{3, 3}`.
- Comment explains what an operator overload is, in beginner terms.

**`world/action.h / .cpp`**
- `enum class Action { North, East, South, West };`
- `std::string actionName(Action)` → `"North"`, `"East"`, …
- `Position offsetFor(Action)` → the one-cell change for that direction (Section 3.1).
- `Position moved(Position, Action)` → the neighbouring position in that direction.

**`world/world_state.h / .cpp`**
- `constexpr int gridSize = 10;`
- `struct WorldState` containing:
  - `walls` — a 10×10 grid of true/false, stored as `std::array<std::array<bool, gridSize>, gridSize>`, indexed **`walls[y][x]`** (row first). A comment must explain the row-first indexing clearly, because mixing up x and y is a common beginner mistake.
  - `agent`, `object`, `destination` — each a `Position`.
- Equality comparison for whole states (needed by tests to check "nothing changed").
- Helper functions:
  - `bool isInsideGrid(Position)` — true when x and y are each from 0 to 9.
  - `bool isWall(const WorldState&, Position)` — treats any position outside the grid as a wall (3.6).
  - `bool isValidWorld(const WorldState&)` — the four validity rules in 3.6. *(v1.1)*
  - `bool isGoalReached(const WorldState&)`

**`world/rules.h / .cpp`**
- `enum class MoveOutcome { Moved, Pushed, Blocked, InvalidState };` and `std::string outcomeName(MoveOutcome)`. *(`InvalidState` added in v1.1.)*
- `struct ActionResult { WorldState state; MoveOutcome outcome; };`
- **`ActionResult applyAction(const WorldState& current, Action action);`**
  - First checks `isValidWorld`; if the state is invalid, returns it unchanged with `InvalidState` (3.6).
  - Otherwise implements Section 3.4 exactly.
  - Takes the current state by `const` reference and returns a **new** state. It never changes its input.
  - This "pure function" design is deliberate: the same kind of function will later be used to *predict* what an action would do, without changing the real world. A comment must say this.

**`world/world_text.h / .cpp`**
- `struct ParseResult { std::optional<WorldState> world; std::string error; };`
- `ParseResult parseWorld(const std::vector<std::string>& lines);` — applies the rejection rules in 3.5, checking sizes before reading (3.6), and calls `isValidWorld` as its final check. On success, `world` holds the state and `error` is empty; on failure, `world` is empty and `error` explains why in plain language.
- `std::vector<std::string> renderWorld(const WorldState&);` — 10 strings of 10 characters, using the table in 3.5. **Requires a valid state** (precondition documented in a comment; see 3.6).
- Validation must be split into small helper functions so no function exceeds ~40 lines.

**`world/environment.h / .cpp`**
- `class Environment` — the single **true** world for a run.
  - Constructor takes the initial `WorldState`.
  - `ActionResult step(Action action);` — applies the rules to the true state, stores the new state, adds one to the step count, returns the result.
  - `const WorldState& state() const;`
  - `int stepCount() const;`
  - The state is `private`: nothing outside the class can change the true world except through `step()`.
- Comments explain: what a class is, what `private` protects, what a constructor is, and why `state()` and `stepCount()` are marked `const`.

**`demo/scripted_demo.h / .cpp`**
- `void runScriptedDemo(std::ostream& out);`
- Uses the fixed layout and fixed action list in Section 5. Prints the initial world, one line per step, the final world, whether the goal is reached, and the step count.
- Takes an output stream as a parameter (rather than writing to `std::cout` directly) so it could be tested later. A comment explains this.
- A comment at the top states plainly: **the actions are hand-written; nothing chooses them.**

**`main.cpp`**
- Stays short: prints the three header lines, then calls `runScriptedDemo(std::cout)`.

**`core/project_info.cpp`**
- `currentStage()` now returns `1`.

### 4.3 Tests structure

Stage 0 had one test file with a `check()` helper inside it and a hand-typed check count. Stage 1 needs several test files, so:

- `tests/test_support.h / .cpp`:
  - `struct TestCounter { int checks = 0; int failures = 0; };`
  - `void check(TestCounter& counter, bool condition, const std::string& description);` — prints `PASS:`/`FAIL:` and updates the counter.
  - This replaces the hand-typed `constexpr int checks = 2;` from Stage 0: checks are now **counted automatically**, so adding a test can never make the summary wrong.
- `tests/test_suites.h`: declares one function per test group, for example `void runRulesTests(TestCounter& counter);`.
- `tests/test_main.cpp`: creates one `TestCounter`, calls each group in order, prints the total, and returns 0 only if there were no failures.
- Each group prints a short heading, for example `--- World rules ---`, before its checks.

There is still **one** test program and it is still registered with CTest. The CTest test name changes from `stage0_tests` to **`correctness_tests`**, because it is no longer specific to Stage 0. This is recorded as a decision (Section 8).

---

## 5. The scripted demonstration (expected program output)

### 5.1 Demo layout

```
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
```

Agent at (3, 3), object at (5, 3), destination at (8, 3). The internal wall in row 6 is decoration only; it is there to show that internal walls are read and printed.

### 5.2 Demo actions

`East, East, East, East, East`

### 5.3 Exact expected output of the `ai_hologram` program

```
AI Hologram -- Stage 1
Grid world: environment rules only (scripted demonstration).
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
```

Why: step 1 moves the agent next to the object; steps 2–4 push the object east until it lands on the destination (shown as `*`); step 5 tries to push the object into the outer wall and is blocked, so nothing changes, but it still counts as a step.

---

## 6. Required correctness tests

Each item below must be checked by at least one `check()`. Descriptions in `PASS:`/`FAIL:` lines should be readable sentences.

### 6.1 Project info (moved from Stage 0)
- P1. `projectName()` returns `"AI Hologram"`.
- P2. `currentStage()` returns `1`.

### 6.2 Text format (`test_world_text.cpp`)
- T1. A valid layout parses successfully, and the agent, object, destination, and a sample of wall/floor cells are where the text says.
- T2. Wrong number of lines is rejected with a non-empty error message.
- T3. A line of the wrong length is rejected.
- T4. A gap in the outer border wall is rejected.
- T5. Zero agents is rejected; two agents is rejected.
- T6. Zero objects is rejected; two objects is rejected.
- T7. Zero destinations is rejected; two destinations is rejected.
- T8. An unknown character is rejected.
- T9. `*` is read as object on destination, and `+` as agent on destination.
- T10. **Round trip**: for each of **three separate valid layouts**, rendering the parsed world gives back exactly the original 10 lines. The three layouts are: (a) the demo layout; (b) a layout with the object on the destination (`*`); (c) a layout with the agent on the destination (`+`). *(Corrected in v1.4: v1.3 asked for one layout containing both `*` and `+`, which is impossible, because a valid world has exactly one destination.)*
- T11. *(v1.1)* Unsafe-looking input is rejected without crashing: an empty list of lines, a list containing empty strings, and lines much longer than 10 characters.

### 6.2a World validity (`test_world_text.cpp` or a new section of it) *(added in v1.1)*
- V1. The demo world, as parsed, is valid.
- V2. A directly constructed state with the agent on a wall is invalid.
- V3. A directly constructed state with the agent and object on the same cell is invalid.
- V4. A directly constructed state with a gap in the border wall is invalid.
- V5. A directly constructed state with a position outside the grid (for example x = 10, or y = −1) is invalid.
- V6. `isWall` returns `true` for positions outside the grid, including (−1, 0), (10, 0), (0, −1), and (0, 10).

### 6.3 Movement rules (`test_rules.cpp`)
- R1. Moving into a free cell: for **each** of the four directions, the agent moves exactly one cell in that direction, the outcome is `Moved`, and the object does not move.
- R2. Moving into a wall: outcome `Blocked`, and the whole state is unchanged.
- R3. Pushing the object into a free cell: object and agent each move one cell, outcome `Pushed`.
- R4. Pushing the object into a wall: outcome `Blocked`, and **both agent and object are unchanged**.
- R5. Pushing the object onto the destination: outcome `Pushed`, and `isGoalReached` is true.
- R6. The agent stepping onto the destination: outcome `Moved`, and `isGoalReached` is false.
- R7. No pulling: when the agent moves away from an adjacent object, the object stays where it was.
- R8. Walls never change after any action (checked after moves, pushes, and blocks).
- R9. `applyAction` never changes its input: the original state is identical before and after the call.
- R10. An action from a state where the goal is already reached still follows the normal rules.
- R11. *(v1.1)* `applyAction` on an invalid state (for example, agent outside the grid, or agent on a wall) returns `InvalidState` and the state unchanged, without crashing.

### 6.4 Environment (`test_environment.cpp`)
- E1. A new environment reports the initial state and a step count of 0.
- E2. Each call to `step()` adds 1 to the step count, **including blocked actions**.
- E3. After a sequence of steps, the environment's state equals the result of applying `applyAction` to the same sequence by hand.
- E4. *(v1.1)* An environment created with an invalid state returns `InvalidState` from `step()`, leaves its state unchanged, and still counts the step.

### 6.5 Expected test output shape

The test program prints one heading per group, one `PASS:` line per check, and a final summary in the form:

```
N checks, 0 failures
```

where N is counted automatically. The exact N is whatever the implemented checks add up to; it is recorded in Section 12 ("What actually happened").

---

## 7. Implementation order

Stage 1 is larger than Stage 0, so it is split into **three sub-stages** (Brief Section 10 allows this). Each sub-stage ends with something the owner can build, run, and understand, and its own commit. **Copilot implements one sub-stage at a time and never commits.**

### Step 1 — Preserve the approved plan and permit the handoff notes (owner, before Copilot writes anything)
- Save this plan, once approved, as `docs/stages/STAGE_01.md`.
- *(v1.2)* Add rule **2f** (Section 13a) to the "Existing files and Git history" section of `.github/copilot-instructions.md`, directly after rule 2e, **by hand**, copying it word for word. Change nothing else in that file.
- Stage both files **by name** and commit: `Stage 1: add approved Stage 1 plan and permit Copilot handoff notes`. Push.

### Step 2 — Sub-stage 1a: positions, actions, world state, and text format
- Create `position.h`, `action.h/.cpp`, `world_state.h/.cpp` (including `isInsideGrid`, bounds-safe `isWall`, and `isValidWorld`), `world_text.h/.cpp`.
- Tests in this sub-stage: P1–P2, T1–T11, V1–V6.
- Create `test_support.h/.cpp`, `test_suites.h`, `test_project_info.cpp`, `test_world_text.cpp`; rewrite `test_main.cpp` to run the groups.
- Update `CMakeLists.txt`: add the new source files to `ai_hologram_core` and the new test files to `ai_hologram_tests`; rename the CTest test to `correctness_tests`.
- Leave `currentStage()` returning `0` for now, and P2 checking `0`. (It changes in sub-stage 1c, when the program's visible behaviour changes.)
- **Checks:** Build → Build Project with no warnings; `ai_hologram_tests` shows only PASS lines; CTest passes; `ai_hologram` output unchanged from Stage 0.
- **Commit:** `Stage 1a: add positions, actions, world state, and text format`

### Step 3 — Sub-stage 1b: rules and environment
- Create `rules.h/.cpp`, `environment.h/.cpp`, `test_rules.cpp`, `test_environment.cpp`; register them in `CMakeLists.txt` and `test_suites.h`/`test_main.cpp`.
- Tests in this sub-stage: R1–R11, E1–E4.
- **Checks:** as in Step 2.
- **Deliberate failure check** (Brief Lesson L7) — this time on the **rules code**, not a test expectation, to prove the tests catch a real rule bug:
  1. The owner temporarily changes `applyAction` so that pushing the object into a wall still moves the agent (the exact one-line edit will be given by Claude after reviewing the code).
  2. Build → Build Project; run CTest. **Check:** R4 reports `FAIL`, CTest reports the failure.
  3. Undo the change, rebuild, and confirm all checks pass again. Do **not** commit the temporary change.
- **Commit:** `Stage 1b: add movement rules and environment`

### Step 4 — Sub-stage 1c: scripted demonstration and stage number
- Create `scripted_demo.h/.cpp`; update `main.cpp`; change `currentStage()` to `1` and P2 to check `1`.
- Update `README.md` status to the wording in Section 9.4.
- Add decisions D-007 to D-011 (Section 8) to `docs/DECISIONS.md`.
- **Checks:** Build → Build Project with no warnings; `ai_hologram` prints **exactly** the output in Section 5.3; all tests pass; CTest passes.
- **Commit:** `Stage 1c: add scripted demonstration and update stage number`

### Step 5 — Fresh-build check
Tools → CMake → Reset Cache and Reload Project, then Build → Rebuild All. No warnings; program output and tests still correct.

### Step 6 — Owner verification gate
The owner personally confirms all items in Section 10 before completion. Copilot's report does not count as verification.

### Step 7 — Record, push, and tag (only after Step 6 passes)
- Fill in Section 12, "What actually happened".
- Commit `Stage 1: record stage completion`; push.
- Create and push the annotated tag `stage-1`.
- The completion commit and tag are created **only by the owner, or on the owner's explicit instruction**, and only after the gate passes.

### Running CTest from PowerShell (lesson from Stage 0)

In a plain PowerShell terminal, run these two lines **in the same terminal tab**, after **Build → Build Project** (CTest does not rebuild anything):

```
$env:Path = "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\mingw\bin;" + $env:Path
& "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\cmake\win\x64\bin\ctest.exe" --test-dir cmake-build-debug --output-on-failure
```

Alternatively, run **All CTest** from CLion's run-configuration drop-down.

---

## 8. Decisions to record in `docs/DECISIONS.md` (sub-stage 1c)

| ID | Decision | Reason | Alternatives considered |
|---|---|---|---|
| D-007 | Coordinates are (x, y) with x = column left→right and y = row top→bottom; walls stored row-first as `walls[y][x]`. Worlds are read and printed using the 7-character text format in plan 3.5. | Matches how the text layout looks on screen; one format for input, output, and tests. | (row, column) pairs; a separate file format. |
| D-008 | Movement rules are a pure function `applyAction(const WorldState&, Action)` returning a new state; the `Environment` class is the only holder of the true state. | The same kind of function can later be used for predictions without changing the real world; keeps "what I think" separate from "what is" (Brief Stage 2). | Rules as methods that change the environment directly. |
| D-009 | Tests are split into several files sharing a `TestCounter`; checks are counted automatically; the CTest test is renamed `correctness_tests`. Every attempted action, including a blocked one, counts as a step. | The test suite now covers several components; a hand-typed count could become wrong. Counting blocked steps keeps step counts honest for later comparisons. | Keep one test file; count only successful moves. |
| D-010 | *(v1.1)* One validity definition, `isValidWorld` (plan 3.6), used by `parseWorld` and checked at the top of `applyAction`. Invalid states produce the outcome `InvalidState` with the state unchanged. Positions outside the grid are treated as walls by `isWall`, which never reads outside the array. | Directly constructed states (tests now, beliefs later) must not cause crashes or out-of-bounds reads; a faulty belief that describes an impossible world should be reported clearly. Kept to one check, not a validation framework. | Trust callers without checking; exceptions; assertions that stop the program. |
| D-011 | *(v1.2)* Copilot appends a handoff note after each piece of work to `results/notes_from_github_copilot/Response_from_Github_copilot.txt` (rule 2f, an exception to 2b). Copilot may not rename, move, or delete files unless a plan lists the change. | Makes Copilot's reports easy to pass on for review. On 2026-10-19, while this file was being set up, `docs/chatgpt_reply.txt` was renamed and the rename silently rewrote mentions of it in three approved files, including the tagged Stage 0 plan; all of it was reverted before any commit. | Copying Copilot's chat replies by hand; allowing renames with review afterwards. |

---

## 9. Documentation changes in Stage 1

### 9.1 Code comments
Every new file starts with the three-part header block (Purpose / "Introduced in Stage 1" / Architecture). Beginner comments explain each C++ feature **the first time it appears in the project**, including at least:
- `struct` vs `class`, and `private`;
- constructors;
- `enum class` and `switch`;
- `std::array` (and a 2-D array of arrays), `std::vector`, `std::optional`;
- operator overloading (`==`);
- `const` member functions;
- passing by `const` reference vs returning a new value;
- `std::ostream&` as a parameter.

### 9.2 `docs/EXPERIMENT_LOG.md`
Unchanged. Stage 1 runs no experiments.

### 9.3 `.github/copilot-instructions.md`
*(Corrected in v1.3.)* The only change is the addition of **rule 2f** (Section 13a), which the owner adds by hand in Step 1, word for word, directly after rule 2e. **No other change to this file is authorized** in Stage 1.

### 9.4 `README.md` status
Replace the status paragraph with:

> **Current status: Stage 1 — grid world.** The project contains a 10×10 grid world with movement and pushing rules, a text display, a hand-written scripted demonstration, and correctness tests for every rule. **No reasoning components exist yet. No experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

No other README section changes in Stage 1. The honesty rule (Stage 0 plan, 5.7) continues to apply.

---

## 10. Stage 1 completion checklist (owner verification gate)

**Approval and history**
- [ ] This plan is approved and was committed before any Stage 1 code (Step 1).
- [ ] Git history is intact: no force-pushes, rebases, or amended pushed commits.
- [ ] The gate below was passed **before** the completion commit and `stage-1` tag.

**Scope**
- [ ] Only the files listed in Section 4.1, plus `CMakeLists.txt`, `README.md`, `docs/DECISIONS.md`, this plan, `.github/copilot-instructions.md` (rule 2f only), and the Copilot handoff-notes file, were added or changed.
- [ ] *(v1.2)* No file was renamed, moved, or deleted except as listed in this plan.
- [ ] *(v1.2)* The handoff-notes file contains one appended note per piece of work, and no earlier note was deleted or rewritten.
- [ ] Nothing from Section 2 was added.

**Build, run, test**
- [ ] Build → Build Project gives no errors and **no warnings**, including after a cache reset.
- [ ] `ai_hologram` prints **exactly** the output in Section 5.3.
- [ ] `ai_hologram_tests` shows every check as PASS and ends with `N checks, 0 failures`.
- [ ] Every test item in Section 6 (P1–P2, T1–T11, V1–V6, R1–R11, E1–E4) is covered by at least one check.
- [ ] CTest reports `correctness_tests` passed (1 of 1).
- [ ] The deliberate failure check on the rules code (Step 3) was done, R4 failed, and all checks passed again after undoing it.

**Code and documentation**
- [ ] Every new file has the three-part header block and beginner comments (9.1).
- [ ] No file over ~200 lines; no function over ~40 lines.
- [ ] `applyAction` takes `const WorldState&` and returns a new state.
- [ ] *(v1.1)* There is exactly one validity definition (`isValidWorld`), used by `parseWorld` and checked at the top of `applyAction`; no other copy of the rules exists.
- [ ] *(v1.1)* `isWall` and `parseWorld` never read outside an array or string: sizes and positions are checked before reading (3.6).
- [ ] *(v1.3)* `renderWorld` has a comment stating that it requires a valid state, and it indexes `walls` only with its own loop counters (0–9), never with agent, object or destination positions (3.6).
- [ ] README status matches 9.4; DECISIONS.md contains D-007 to D-011.
- [ ] The owner can explain the items in Section 11.

---

## 11. Things the owner should understand by the end

1. How (x, y) maps to the text grid, and why walls are stored as `walls[y][x]`.
2. The difference between a `struct` and a `class`, and what `private` protects.
3. What a constructor does.
4. What `enum class` is and how `switch` uses it.
5. Why `applyAction` returns a new state instead of changing the old one.
6. Why only `Environment` holds the true state.
7. What `std::optional` means in `ParseResult`.
8. What a round-trip test proves.
9. Why blocked actions still count as steps.
10. Why the deliberate failure check was done on the rules code this time.
11. *(v1.1)* What a "precondition" is, what makes a `WorldState` valid, and why `applyAction` checks validity before doing anything.
12. *(v1.1)* Why reading outside an array is dangerous in C++, and how `isWall` and `parseWorld` avoid it.

---

## 12. What actually happened (filled in by the owner at the end)

```
Date completed:
Commit tagged stage-1:
Total number of checks in the final test run (N):

Did everything go as planned?  yes / no
Differences from the plan, and why:

Problems encountered and how they were solved:

Anything learned that should change future stage plans:
```

---

## 13. Instructions to GitHub Copilot for this stage

1. Read `.github/copilot-instructions.md`, this plan, and the brief before writing anything.
2. Implement **one sub-stage at a time** (Section 7, Steps 2, 3, 4), only when the owner asks for that sub-stage by name. Never start the next sub-stage on your own.
3. Before creating or changing any file, check whether it exists and read it first.
4. **Every new or changed source file must include the three-part header block and beginner comments as described in 9.1.** Do not deliver minimal comments and wait to be asked.
5. Implement the rules in Section 3 **exactly**. If a rule seems unclear or wrong, stop and ask; do not invent behaviour.
6. Keep files under ~200 lines and functions under ~40 lines; split helpers rather than writing long functions.
7. Do not add anything from Section 2. Do not add files not listed in Section 4.1.
8. Never change a test merely to make it pass. Never weaken a rule to make a test pass.
9. Do not commit, tag, or push. The owner does all Git operations.
10. When a sub-stage is finished, tell the owner: which files were created or changed, what each does in one sentence, and exactly what output to expect from `ai_hologram` and `ai_hologram_tests`. Remind the owner to use **Build → Build Project** before running CTest.
11. *(v1.2)* After each piece of work, **append** a handoff note to `results/notes_from_github_copilot/Response_from_Github_copilot.txt`, as described in 13a.
12. *(v1.2)* **Never rename, move, or delete any file, and never use the IDE's rename or refactoring tools on files**, unless this plan explicitly lists that change. Renaming a file can silently rewrite every mention of it across approved documents. If a rename or any change outside this plan's file list seems necessary, stop and ask the owner.

### 13a. Copilot handoff notes *(added in v1.2)*

The owner has created `results/notes_from_github_copilot/Response_from_Github_copilot.txt` so Copilot's report on each piece of work can be passed easily to Claude and ChatGPT. It is organizational only; it is **not** research evidence.

**Rule 2f**, to be added by the owner to `.github/copilot-instructions.md` (Step 1), word for word:

> - **2f.** Exception to rule 2b: after finishing each piece of work, append a handoff note to `results/notes_from_github_copilot/Response_from_Github_copilot.txt`. Add to the end of the file only. Never delete, rewrite, or reorder earlier notes. This is the only owner file you may write to.

**Each note contains**, in plain language:
- a heading line with the date and time, and the stage or sub-stage (for example, `=== 2026-10-20 10:15 — Sub-stage 1a ===`);
- the files created or changed, with one sentence each on what they do;
- what was **actually** built or run, and its result. Copilot must not claim anything was built, run, or tested unless it was. If nothing was run, the note says so;
- anything Copilot was unsure about, or any conflict noticed between documents;
- a final line confirming that nothing was committed.

The owner commits the notes file together with each sub-stage's commit.

---

## 14. Related planning outside this stage: Experiment 1 Protocol timing (informational)

*No implementation is required in Stage 1. This section records the agreed timing so the plan and the repository history show it.*

The brief's Definition Gate (Section 10) requires `docs/EXPERIMENT_1_PROTOCOL.md` to be written, independently reviewed, and approved **before Stage 4**. The protocol fixes the experiment's rules in concrete numbers and definitions before any results exist, so results cannot reshape the rules (Brief 11.7, Lesson L8).

It will be written in **three phases**, each when it can be written accurately:

| Phase | When | Content |
|---|---|---|
| **A — Framework** | After Stage 1 is complete (tag `stage-1`) | Purpose; hypotheses H1–H5; conditions C0–C4; fault types F1–F5; measurements; reporting rules; definition of a "meaningful" difference; the pilot-run rule. |
| **B — Definitions and baseline behaviour** | After Stage 2 is complete, and **before the Stage 3 plan is approved** *(revised in v1.1)* | How states, predictions, and mismatches are represented; how each fault type is applied to a belief; how the budget is counted. **The behaviour of the competent single-agent baseline (C1):** its predict → act → observe → compare → correct → replan loop, its look-ahead, its use of safe stops, mismatch-source labels, and two-part correction verification (Brief 4.2–4.5), and how it spends its budget. Written to match the real Stage 2 code. |
| **C — Numbers and freeze** | During Stage 3, before Stage 4 | Behaviour definitions for the two-agent conditions (C2–C4), built on the shared definitions from Phase B; budget B, stop limit, fault rates, number of layouts and seeds. Then full review, approval, and freeze. |

**Protecting the baseline** *(added in v1.1)*: the competent single agent is the comparison that every hypothesis depends on (Brief 11.4, Lesson L2). To stop it being redesigned later in a way that favours the two-agent system:
- The baseline behaviour definitions (Phase B) are reviewed by ChatGPT and approved by the owner **before** `STAGE_03.md` is approved, and `STAGE_03.md` implements them as written.
- If Stage 3 reveals that a baseline definition is impossible or unclear, the definition is revised **openly**, reviewed again, and recorded in `DECISIONS.md` **before** the Stage 3 implementation is finalized and tagged.
- Once Stage 3 is tagged, the baseline may change only to fix a genuine bug. Each fix is recorded in `DECISIONS.md`, and every condition is re-run afterwards (Brief 11.5.4).
- No two-agent code (Stage 6) is written until the baseline is complete and tagged. The baseline is never adjusted in response to how the two-agent system performs.

**Process for each phase:** Claude drafts → ChatGPT reviews independently → Claude revises → owner approves → owner commits it marked as a draft (e.g. `Experiment 1 protocol: draft v0.1 (Phase A framework)`).

**Freeze:** after Phase C, ChatGPT reviews the complete protocol; the owner approves it as **APPROVED — FROZEN**, commits it, adds a decision entry to `DECISIONS.md`, and creates the tag `protocol-1-approved`. Stage 4 begins only after this.

**After the freeze:** any change needs a new protocol version, a written reason in `DECISIONS.md`, and must happen before any real experiment runs. After real results have been seen, the protocol cannot be changed; errors found then are documented, and the experiment is re-run under a new, separately approved protocol.

**Pilot-run rules (to be confirmed in Phase A)** *(expanded in v1.1)*:
1. **Separate material.** Pilot runs use layouts and seeds that are **never** used in the final (confirmatory) experiment. The two sets are listed separately in the protocol.
2. **Allowed purposes only.** Pilots may be used to find and fix:
   - crashes or hangs;
   - broken instrumentation (for example, counters or logs that record the wrong thing);
   - impossible budgets (so small that no condition can act at all);
   - experiments that cannot finish (runs that never end, or limits that are never reached).
3. **No tuning toward a result.** Comparative pilot performance (how one condition does relative to another) must **not** be used to choose any parameter. In particular, no parameter may be chosen because it makes C3 look better. Where possible, pilot reports show only whether runs worked, not condition-by-condition scores.
4. **Every change is documented.** Each change made because of a pilot is recorded in `EXPERIMENT_LOG.md` (marked *PILOT — not evidence*) and in `DECISIONS.md`. The record says what was observed, what was changed, and why the change is about whether the experiment works rather than which condition wins.
5. **Freeze before confirmatory results.** The final protocol is frozen (tag `protocol-1-approved`) **before** any confirmatory experimental result is run or examined. Pilots happen only before the freeze.

---

## Revision history of this plan

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-10-19 | First draft by Claude, after Stage 0 was completed and tagged. Incorporates Stage 0 lessons: full PowerShell CTest commands, "Build → Build Project before CTest", explicit comment requirements for Copilot, automatic check counting. Includes the Experiment 1 Protocol timing (Section 14). For independent review by ChatGPT and approval by the owner. |
| 1.1 | 2026-10-19 | Revised after ChatGPT's review, which supported the architecture, the pure `applyAction` design, no Wait action, the test restructuring, the three sub-stages, and confirmed the demo output. Three targeted changes; **all other content unchanged**. **(1) WorldState validity and safety:** new 3.6 defines a valid state (positions inside grid, border walls, things on non-wall cells, agent ≠ object) in one function `isValidWorld`, used by `parseWorld` and checked at the top of `applyAction`; new outcome `InvalidState` (state unchanged); `isWall` treats outside-grid positions as walls and never reads outside the array; `parseWorld` checks sizes before reading. Matching updates to 2 (no validation framework), 4.2 (`isInsideGrid`, `isValidWorld`, `InvalidState`), 6 (new T11, V1–V6, R11, E4), 7 (tests per sub-stage), 8 (new D-010), 10 (checklist), 11 (learning items 11–12). **(2) Protocol timing (14):** the competent single-agent baseline's behaviour definitions move into Phase B and must be reviewed and approved before `STAGE_03.md` is approved; new "Protecting the baseline" rules. **(3) Pilot safeguards (14):** expanded into five rules covering separate material, allowed diagnostic purposes, no comparative tuning toward C3, documentation of every pilot-driven change, and freezing before confirmatory results. No C++ code written. For final review by ChatGPT and owner approval. |
| 1.2 | 2026-10-19 | Added the owner's Copilot handoff-notes file, `results/notes_from_github_copilot/Response_from_Github_copilot.txt`, which the owner created after v1.1. New 13a defines rule 2f (append-only notes, the single exception to rule 2b) and the content of each note. Step 1 now has the owner add rule 2f to `.github/copilot-instructions.md` by hand. New Copilot rules 11–12 (append notes; never rename, move, or delete files or use refactoring tools on files unless the plan lists it). New D-011 records this and the rename incident of 2026-10-19, which was reverted before any commit. Checklist (10) updated. **No change to the architecture, the world rules, the tests, or the v1.1 changes requested by ChatGPT.** For final review by ChatGPT and owner approval. |
| 1.3 | 2026-10-19 | ChatGPT reviewed v1.2 in full, recommended approval, and approved the handoff-notes system. Two documentation clarifications requested: **(1)** 9.3 corrected: `.github/copilot-instructions.md` changes only by the addition of rule 2f in Step 1; no other change is authorized. **(2)** `renderWorld` requires a valid `WorldState`: documented in 3.6 and 4.2, explaining that it indexes `walls` only with its own 0–9 loop counters and only compares positions, so it cannot read outside the array; no extra checking code. Matching checklist item added (10). **No change to architecture or scope.** **Approved by the owner on 2026-10-19**, following ChatGPT's recommendation. |
| 1.4 | 2026-10-19 | **Correction found during implementation.** While implementing sub-stage 1a, GitHub Copilot stopped and reported that test T10 asked for one valid layout containing both `*` (object on destination) and `+` (agent on destination). That is impossible, because a valid world has exactly one destination (3.5, 3.6). Copilot did not change the parser or rules to make the test pass. **T10 now uses three separate valid layouts** (demo, `*`, `+`). No other change: the rules, validity definition, architecture and scope are unchanged. Correction approved by the owner on 2026-10-19; ChatGPT to be informed at the next review. |
