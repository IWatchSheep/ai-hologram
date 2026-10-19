# Stage 0 — Repository Foundation

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Stage** | 0 of the roadmap in *AI_Hologram_Project_Brief_v1.1.txt*, Section 10 |
| **Plan version** | 1.2 |
| **Date** | October 19, 2026 |
| **Status** | **v1.2 APPROVED by the owner on October 19, 2026** (after final review and approval by ChatGPT). Ready for implementation, beginning with Step 1a. |
| **Governing specification** | AI_Hologram_Project_Brief_v1.1.txt (approved October 19, 2026; **unchanged** by this revision) |

---

## 1. Purpose of Stage 0

Stage 0 proves that the basic development cycle works before any research code exists:

> **write → build → run → test → commit**

When Stage 0 is complete, the owner should be able to open the project in CLion, build it, run it, run the tests, and understand what every file does and why it exists.

Stage 0 also sets up the **patterns** every later stage will follow:
- where source files, tests, documents, and selected results live;
- how CMake builds a program and a test program from shared code;
- how decisions and results are recorded;
- how Copilot receives its instructions.

## 2. What Stage 0 must NOT contain

To keep this stage small (Brief, Lesson L9), Stage 0 contains **none** of the following:

- No grid world, walls, objects, or movement.
- No agents, beliefs, predictions, or comparisons.
- No messages, fault injection, budgets, or experiment runner.
- No random numbers.
- No external libraries or test frameworks (e.g. no GoogleTest, Catch2).
- No networking, threads, or GUI.

If anything in this list seems necessary, **stop and ask the owner** rather than adding it.

---

## 3. Folder layout after Stage 0

*Updated in v1.2 to match the folders and files the owner has already created.*

Legend:  **[OWNER]** already created by the owner — preserve, do not overwrite.
**[STAGE 0]** created during Stage 0 according to this plan.

```
ai-hologram/
│
├── CMakeLists.txt                  [STAGE 0] Build instructions for CMake
├── README.md                       [STAGE 0] Front page of the GitHub repository
│                                             (revise, don't replace, if one already exists — see 5.7)
├── LICENSE                         [OWNER]   Licence terms — preserve unchanged (see 5.13)
├── .gitignore                      [OWNER]   Files Git must never track (see 5.6)
│
├── .github/
│   └── copilot-instructions.md     [STAGE 0] Rules GitHub Copilot reads automatically
│
├── docs/
│   ├── AI_Hologram_Project_Brief_v1.txt      [OWNER] Original v1.0 (kept for history)
│   ├── AI_Hologram_Project_Brief_v1.1.txt    [OWNER] Approved specification
│   ├── chatgpt_reply.txt           [OWNER]   Preserved ChatGPT review notes (see 5.14)
│   ├── DECISIONS.md                [STAGE 0] Log of design decisions
│   ├── EXPERIMENT_LOG.md           [STAGE 0] Expected vs observed results (no experiments yet)
│   └── stages/
│       └── STAGE_00.md             [OWNER]   This plan
│
├── results/
│   ├── console/
│   │   └── program_console_results.txt   [OWNER] Selected console output (see 5.15)
│   └── debugging/
│       └── debugging_output.txt          [OWNER] Selected debugging output (see 5.15)
│
├── src/
│   ├── main.cpp                    [STAGE 0] The program's starting point
│   └── core/
│       ├── project_info.h          [STAGE 0] Declares project name and stage functions
│       └── project_info.cpp        [STAGE 0] Defines those functions
│
└── tests/
    └── test_main.cpp               [STAGE 0] The test program
```

**Owner-approved organizational additions** (*v1.2*): `LICENSE`, `docs/chatgpt_reply.txt`, `results/console/program_console_results.txt`, `results/debugging/debugging_output.txt`, and the owner's `.gitignore`.
These organize the repository. **They do not represent completed research or experimental results.** No experiment has been run, and nothing in `results/` is evidence for or against any hypothesis in the brief.

*(v1.1 said `results/` would not be created in Stage 0. The owner has created it; that note is superseded. This is consistent with Brief Section 8.3, which already lists a `results/` folder.)*

---

## 4. The one design idea in Stage 0: a shared core library

A beginner's first C++ project usually has only `main.cpp`. This project needs one extra idea from the start, because **tests must be able to use the same code the program uses**.

```
            project_info.cpp
                   │
                   ▼
        ┌─────────────────────┐
        │  ai_hologram_core   │   ← a "library": compiled code with
        │   (static library)  │     no main() of its own
        └─────────────────────┘
             │            │
             ▼            ▼
      ai_hologram     ai_hologram_tests
      (the program)   (the test program)
       main.cpp        test_main.cpp
```

- **`ai_hologram_core`** holds the real code. In Stage 0 it contains only `project_info`. In later stages the grid world, agents, etc. are added here.
- **`ai_hologram`** is the program you run. Its `main.cpp` stays short and only calls core code (Brief, Section 8.2).
- **`ai_hologram_tests`** is a separate program that checks the core code and reports pass/fail.

Why this matters: if tests had to copy code from `main.cpp`, they would be testing a copy, not the real thing. With a shared library, both programs use exactly the same compiled code.

`project_info` is deliberately tiny. Its job is to give the program something real to print and the test something real to check, so the library/program/test pattern can be learned with almost no code.

---

## 5. File-by-file specification

### 5.1 `CMakeLists.txt` (repository root)

**What it is:** CMake is a tool that reads this file and generates the instructions a compiler needs. CLion reads this file automatically to understand the project.

**Requirements** (Copilot writes the file from these):

1. Require a minimum CMake version of **3.20** (needed for `ctest --test-dir`; the CMake bundled with current CLion is newer).
2. Declare the project name `ai_hologram`, language C++, version `0.0.0`.
3. Require **C++17** exactly:
   - standard set to 17;
   - standard marked as *required* (fail rather than silently use an older standard);
   - compiler extensions turned **off** (portable standard C++ only).
4. Turn on warnings for all targets:
   - for MSVC: `/W4`;
   - for all other compilers (MinGW, GCC, Clang): `-Wall -Wextra -Wpedantic`.
   - Do **not** turn warnings into errors in Stage 0 (the owner may decide this later; record it in DECISIONS.md if so).
5. Create the static library target **`ai_hologram_core`** from `src/core/project_info.cpp`, and make the `src` folder its public include directory, so other code can write `#include "core/project_info.h"`.
6. Create the executable target **`ai_hologram`** from `src/main.cpp`, linked to `ai_hologram_core`.
7. Create the executable target **`ai_hologram_tests`** from `tests/test_main.cpp`, linked to `ai_hologram_core`.
8. Enable testing and register `ai_hologram_tests` with CTest under the test name **`stage0_tests`**.
9. Comments in the file explain each section in plain language (what a target is, what linking means, what CTest does).

Keep the file short — roughly 40–60 lines including comments.

### 5.2 `src/core/project_info.h`

**What it is:** a *header file*. It **declares** functions — it says what exists and how to call it, without containing the working code.

**Requirements:**
- Starts with the standard file comment block (Brief, Section 8.2): purpose, "Introduced in Stage 0", how it fits the architecture.
- Uses `#pragma once` so the file is never included twice.
- Places everything inside a namespace named `ai_hologram`.
- Declares two functions:
  - `projectName()` — returns the text `AI Hologram` as a `std::string`.
  - `currentStage()` — returns the whole number `0` as an `int`.
- Each declaration has a one-line comment saying what it returns.

### 5.3 `src/core/project_info.cpp`

**What it is:** the *source file* that **defines** (implements) the functions declared in the header.

**Requirements:**
- Standard file comment block.
- Includes its own header first.
- Implements the two functions exactly as described above.
- A short comment explains why declarations and definitions are split into two files (so many files can use the functions while the working code is compiled only once).

### 5.4 `src/main.cpp`

**What it is:** where the program starts. The operating system calls the `main` function when the program runs.

**Requirements:**
- Standard file comment block, explaining that `main.cpp` must stay short and only connect components together.
- Uses `projectName()` and `currentStage()` — the stage number must **not** be typed directly into `main.cpp`.
- Prints exactly these three lines and returns 0 (meaning "success"):

```
AI Hologram -- Stage 0
Repository foundation: build, run, and test cycle.
No reasoning components are implemented yet.
```

- The third line is intentional: the program must never imply capabilities it does not have (Brief, "How to read this document").

### 5.5 `tests/test_main.cpp`

**What it is:** a separate small program whose only job is to check that core code behaves as expected. It reports results and returns 0 if everything passed, or 1 if anything failed. CTest uses that return value to decide pass/fail.

**Requirements:**
- Standard file comment block, explaining the difference between *correctness tests* and *research measurements* (Brief, Section 12.1). These are correctness tests.
- No test framework and no macros (Brief, Section 8.2). Instead, a small helper function, for example `check(condition, description)`, that:
  - prints `PASS: <description>` or `FAIL: <description>`;
  - keeps a count of failures.
- Two checks:
  1. `projectName()` returns `AI Hologram`.
  2. `currentStage()` returns `0`.
- At the end, prints a summary line, e.g. `2 checks, 0 failures`, and returns 0 if there were no failures, otherwise 1.

**Expected output when all checks pass:**

```
PASS: projectName() returns "AI Hologram"
PASS: currentStage() returns 0
2 checks, 0 failures
```

**Important:** this test is a check that the *testing machinery* works. It says nothing about the research hypotheses and must never be described as evidence for them.

### 5.6 `.gitignore` — **owner's file; inspect, do not replace** (*revised in v1.2*)

**What it is:** a list of files and folders Git should ignore — typically things the computer generates that should not be in the shared history.

The owner has already written `.gitignore` to exclude CLion build folders, IDE settings, temporary files, CTest temporary output, and generated object and executable files. **Copilot must not replace or rewrite it.**

Instead, during Step 2, the owner (or Copilot, read-only) checks that it covers at least:
- `cmake-build-*/` — CLion's build folders;
- `build/` and `out/` — common build folders;
- `.idea/` — CLion's personal settings;
- `Testing/` — CTest output;
- generated object and executable files.

And — just as important — that it does **not** accidentally ignore files that must be tracked. These must **not** be ignored:
- `docs/` and everything in it;
- `results/console/program_console_results.txt` and `results/debugging/debugging_output.txt`;
- `LICENSE`, `README.md`, `.github/copilot-instructions.md`;
- `src/`, `tests/`, `CMakeLists.txt`.

A simple way to check a file: `git check-ignore -v <path>` prints a line if the file **is** ignored (and which rule caused it), and prints nothing if it is not.

If something is missing or wrongly ignored, Copilot **proposes** the exact line to add or change and waits for the owner's approval.

### 5.7 `README.md` (*revised in v1.2*)

**What it is:** the front page visitors see on GitHub. It must always describe the **current** state honestly (Brief, Section 13.4).

**If a README.md already exists** (GitHub often creates one, and the owner may have written one): Copilot first reads it, then proposes changes section by section. Owner-written text is not deleted without the owner's agreement.

**Honesty requirement for the architecture description:**
The two-agent architecture must be described as a **proposed research design under investigation**, never as a working or demonstrated AI system. This applies to every sentence in the README.

- ✅ Acceptable: "proposes", "will investigate", "is designed to test", "hypothesis", "planned", "not yet implemented".
- ❌ Not acceptable: "the AI reasons…", "the agents detect and correct errors", "the system understands…", "a new kind of AI", "achieves", "outperforms" — or any present-tense description of capabilities that do not yet exist.

**Required summary wording** (Copilot may adjust phrasing slightly, but not the meaning):

> **AI Hologram** is a research project that will investigate an alternative approach to artificial reasoning that does not use a large language model as its central reasoning mechanism. The project **proposes** a design in which two cooperating reasoning agents — one emphasising incremental, step-by-step reasoning and one emphasising temporal-spatial reasoning — have overlapping abilities and check each other's predictions against what is actually observed. Whether this design detects and corrects errors better than a competent single agent is an **open research question**. It has not yet been implemented or tested.

**Required status wording:**

> **Current status: Stage 0 — repository foundation.** The project currently contains only a minimal C++ program and one basic test that check the build setup. **No reasoning components exist yet. No experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

**Sections:**
1. **Title and summary** — the required summary wording above.
2. **Current status** — the required status wording above.
3. **Where to start reading** — links to `docs/AI_Hologram_Project_Brief_v1.1.txt`, `docs/DECISIONS.md`, `docs/EXPERIMENT_LOG.md`, `docs/stages/`.
4. **How to build and run in CLion** — open the folder, let CLion load the CMake project, choose the `ai_hologram` configuration, build, run.
5. **How to run the tests** — in CLion: run the `ai_hologram_tests` configuration (or a CTest configuration). From the terminal: `ctest --test-dir cmake-build-debug --output-on-failure`.
6. **Requirements** — CMake 3.20 or newer, a C++17 compiler (CLion's bundled MinGW is fine on Windows).
7. **How the project is developed** — one short paragraph: small approved stages, each built, run, and reviewed before the next; roles of owner, Claude, ChatGPT, Copilot (Brief, Section 9).
8. **Licence** — one line: "See the `LICENSE` file." Do not describe or summarise the licence terms.

Keep it under about one screen of text. Details belong in the brief, not the README.

### 5.8 `.github/copilot-instructions.md`

**What it is:** GitHub Copilot automatically reads this file and applies it as instructions for this repository.

**Content:** a copy of **Section 15 of the brief v1.1**, including rules 13a–13d, word for word, with:
- a title line: `# AI Hologram — Instructions for GitHub Copilot`;
- one line at the top saying the governing document is `docs/AI_Hologram_Project_Brief_v1.1.txt`, and that if the two ever disagree, the brief wins and the owner must be told.
- (*v1.2*) a short section titled **"Existing files and Git history"** containing rules 2a–2e from Section 10 of this plan, word for word. This is an addition from the approved stage plan, not a change to the brief's rules.

Do not rewrite or "improve" the brief's rules.

### 5.9 `docs/AI_Hologram_Project_Brief_v1.txt` and `docs/AI_Hologram_Project_Brief_v1.1.txt`

Already placed by the owner. They stay **unchanged**. v1.1 is the approved specification; v1.0 is kept so the history of the review is visible (Brief, Section 13.4). This plan revision (v1.2) does **not** modify the brief.

### 5.10 `docs/DECISIONS.md`

**What it is:** a running log of design decisions, newest at the bottom. Each entry uses the format from Brief, Section 13.4.

**Initial entries:**

| ID | Date | Decision | Reason | Alternatives considered | Who decided |
|---|---|---|---|---|---|
| D-001 | 2026-10-19 | Adopt AI_Hologram_Project_Brief_v1.1 as the approved starting specification. | Revised after independent review; four corrections applied. | Keep v1.0; further revision first. | Owner (after review by ChatGPT, drafted by Claude) |
| D-002 | 2026-10-19 | Use hand-written tests registered with CTest; no test framework. | No external dependencies in early stages (Brief 8.2); easy for a beginner to read. | GoogleTest, Catch2. Can be revisited later. | Owner |
| D-003 | 2026-10-19 | Put shared code in a static library (`ai_hologram_core`) used by both the program and the tests. | Tests must check the real code, not a copy. | Compile source files into both executables separately. | Owner |
| D-004 | 2026-10-19 | C++17 required, compiler extensions off, warnings on but not treated as errors. | Portability and readable diagnostics while learning. | Warnings as errors from the start. | Owner |
| D-005 | 2026-10-19 | Add organizational files: `docs/chatgpt_reply.txt`, `results/console/`, `results/debugging/`, and the owner's `.gitignore`. These are organizational only and are not research results. | Preserve review notes and selected program output so development can be followed and reproduced. | Keep review notes outside the repository; create `results/` only when experiments begin. | Owner |
| D-006 | 2026-10-19 | `LICENSE` file added by the owner. Its terms are chosen and changed only by the owner. | Licensing is a personal decision with legal effect. | — | Owner |

The file starts with two sentences explaining what it is for and how to add an entry. Plain bullet-style entries are acceptable instead of a table if easier to read.

### 5.11 `docs/EXPERIMENT_LOG.md`

**What it is:** where every experiment is recorded as *expected vs observed* (Brief, Section 12.3).

**Content for Stage 0:**
- One paragraph explaining its purpose.
- The entry template from Brief 12.3 (ID/date/commit, hypothesis, settings, EXPECTED, OBSERVED, difference, interpretation, follow-up).
- The line: **"No experiments have been run yet."**

### 5.12 `docs/stages/STAGE_00.md`

This plan, already placed by the owner. At the end of the stage, the owner adds a short section **"What actually happened"** (Section 9 below).

### 5.13 `LICENSE` — **owner's file; preserve** (*added in v1.2*)

`LICENSE` already exists and must be **preserved exactly as it is**.
- Copilot, Claude, and ChatGPT must not select, change, replace, reformat, or summarise the licensing terms without the owner's explicit permission.
- No other licence files, licence headers in source files, or licence badges are added unless the owner asks.
- The README refers to it only with "See the `LICENSE` file."

### 5.14 `docs/chatgpt_reply.txt` — **owner's file; preserve** (*added in v1.2*)

**Purpose:** preserves useful ChatGPT review notes or instructions, so the reasoning behind reviews is part of the project's visible history.

**How it relates to other documents** (order of authority):
1. `docs/AI_Hologram_Project_Brief_v1.1.txt` — the approved specification.
2. The current approved stage plan (this file, for Stage 0).
3. `docs/chatgpt_reply.txt` — review notes; useful context, **not** a governing document.

If a note in `chatgpt_reply.txt` conflicts with the brief or the approved stage plan, the brief and plan take priority, and Copilot must point out the conflict to the owner rather than act on the note. Copilot does not edit this file; the owner adds to it.

### 5.15 `results/console/` and `results/debugging/` — **owner's files; preserve** (*added in v1.2*)

**Purpose:** to hold **selected** output files that help someone reproduce and understand the program's behaviour and, later, experiments.
- `results/console/` — selected console output from program runs.
- `results/debugging/` — selected debugging output that explains how a problem was found or fixed.

**Rules:**
- Commit only output that is useful for reproducing or understanding something. Temporary logs, repeated runs, and excessively large files do **not** need to be committed. As a rough guide, keep each committed output file small (well under 1 MB); for anything larger, commit a short excerpt or summary instead and ask the owner.
- Each committed output file should say, at the top or in an accompanying note, what produced it: date, Git commit hash, and what was run. (This applies to new files; the two existing owner files are preserved as they are.)
- Output files are never edited by hand after they are produced (Brief, Section 12.4).
- **Nothing in `results/` is a research finding unless it is recorded in `docs/EXPERIMENT_LOG.md` as part of an approved experiment.** The two existing files are organizational, as recorded in D-005.
- Later experiments may also add per-experiment folders (`results/<experiment_id>/`) as described in Brief Section 12.4. `console/` and `debugging/` continue to exist alongside them.

---

## 6. Step-by-step implementation order

Each step ends with something the owner can **see** working. Do not move to the next step until the current one works.

**General rule for every step** (*v1.2*): this is **not** an empty repository. It already contains owner files and Git history. Before creating or changing any file, check whether it already exists. Existing owner files are preserved; Git history is never rewritten.

### Step 1 — Look before changing anything (*revised in v1.2*)
The owner inspects the current state. Useful commands in the CLion terminal:
- `git status` — what is changed, new, or untracked.
- `git log --oneline` — the existing commit history (this must be preserved).
- `git ls-files` — which files Git already tracks.

Then:
- Confirm the owner files listed in Section 3 are present.
- Note which of them are **already committed** and which are **not yet committed**.
- CLion's "New Project" wizard may have created a top-level `main.cpp` and `CMakeLists.txt`. The template `main.cpp` will be **replaced** by `src/main.cpp`, and the template `CMakeLists.txt` by the one in 5.1 — but only in Step 3, and only these two template files.
- If a `README.md` already exists, note it (it will be revised in Step 5, not replaced).
- If anything else unexpected exists, stop and ask before changing or deleting it.

### Step 1a — Preservation commit: approved documents and owner files (done by the owner, before Copilot writes anything) (*revised in v1.2*)
Purpose: make sure GitHub records the approved specification, this plan, and the owner's organizational files **before** any Stage 0 code is written — on top of the existing history, without altering it.

1. Stage only the following files **by name**, and only those not already committed in their current form:
   - `docs/AI_Hologram_Project_Brief_v1.1.txt`
   - `docs/AI_Hologram_Project_Brief_v1.txt`
   - `docs/stages/STAGE_00.md` (this plan, v1.2)
   - `docs/chatgpt_reply.txt`
   - `LICENSE`
   - `.gitignore`
   - `results/console/program_console_results.txt`
   - `results/debugging/debugging_output.txt`

   Example: `git add docs/AI_Hologram_Project_Brief_v1.1.txt docs/AI_Hologram_Project_Brief_v1.txt docs/stages/STAGE_00.md docs/chatgpt_reply.txt LICENSE .gitignore results/console/program_console_results.txt results/debugging/debugging_output.txt`

   Do **not** use `git add .` — CLion template files and IDE folders must not enter this commit.
2. **Check:** `git status` shows only those files staged (any already committed and unchanged simply won't appear).
3. **Commit:** `Stage 0: preserve approved brief v1.1, Stage 0 plan, and owner organizational files`
   Body: the brief was approved after independent review; the added files are organizational and contain no research results; no Stage 0 code exists yet.
4. If **all** of these files were already committed unchanged, no commit is needed — record that in Section 9 instead.
5. Push to GitHub with a normal `git push`. **Check:** the files are visible on GitHub and the earlier commits are still listed in the history.
6. Only after this is pushed does Copilot begin Step 2.

**Never** use history-rewriting commands on this repository (for example `git push --force`, `git rebase`, `git commit --amend` on a pushed commit, `git reset --hard`, or deleting existing branches or tags).

### Step 2 — Remaining documentation and `.gitignore` check (*revised in v1.2*)
- Create `docs/DECISIONS.md` (5.10), `docs/EXPERIMENT_LOG.md` (5.11), and `.github/copilot-instructions.md` (5.8). If any of these already exists, stop and ask the owner instead of overwriting it.
- Check the owner's `.gitignore` as described in 5.6 (read-only). Propose any changes; do not make them without approval.
- **Check:** `git status` does not list `.idea/` or `cmake-build-*` folders; `git check-ignore -v` shows that the files in 5.6's "must not be ignored" list are **not** ignored.
- **Commit:** `Stage 0: add decision log, experiment log, and Copilot instructions`

### Step 3 — Core library and program
Create `src/core/project_info.h`, `src/core/project_info.cpp`, `src/main.cpp`, and `CMakeLists.txt` **without** the test target yet (items 1–6 of 5.1). Remove the CLion template top-level `main.cpp` only after confirming with the owner that it is the unmodified template.
- In CLion: *File → Reload CMake Project*. The configuration `ai_hologram` should appear.
- Build. **Check:** no errors, **no warnings**.
- Run. **Check:** the three lines in 5.4 appear exactly.
- **Commit:** `Stage 0: add core library and minimal program`

### Step 4 — Test program and CTest
Create `tests/test_main.cpp` and add items 7–8 of 5.1 to `CMakeLists.txt`.
- Reload CMake. Build. **Check:** no warnings.
- Run `ai_hologram_tests`. **Check:** the output in 5.5 appears.
- Run CTest (CLion test configuration, or `ctest --test-dir cmake-build-debug --output-on-failure` in the CLion terminal). **Check:** `100% tests passed, 0 tests failed out of 1`.
- **Deliberate failure check** (important — proves the test can actually fail, Brief Lesson L7):
  1. Temporarily change the expected stage number in the test from 0 to 1.
  2. Build and run CTest. **Check:** CTest reports the test **failed**, and the output shows a `FAIL:` line.
  3. Change it back. Confirm the test passes again.
  4. Do **not** commit the temporary change.
- **Commit:** `Stage 0: add test program registered with CTest`

### Step 5 — README (*revised in v1.2*)
- If `README.md` exists: Copilot reads it and proposes changes to match 5.7, section by section, keeping owner-written text unless the owner agrees to change it. The owner reviews the proposed wording against the honesty requirement in 5.7.
- If it does not exist: create it according to 5.7.
- Read it on GitHub after pushing to make sure links work.
- **Commit:** `Stage 0: add README` (or `Stage 0: revise README` if it already existed)

### Step 6 — Fresh-build check
Prove the project builds from scratch, as a new contributor would experience it:
- In CLion: *Tools → CMake → Reset Cache and Reload Project*, then *Build → Rebuild Project*.
- **Check:** builds with no warnings; program output and tests still correct.

### Step 7 — Owner verification gate (required before completion)
The owner personally confirms each of the following on their own computer. Copilot's report that something works does **not** count as verification.

- [ ] The project builds in CLion with no errors and no warnings.
- [ ] `ai_hologram` prints exactly the three lines in 5.4.
- [ ] CTest reports 1 of 1 tests passed.
- [ ] The deliberate failure check (Step 4) was performed, CTest reported the failure, and the test passed again after the change was undone.
- [ ] The fresh-build check (Step 6) succeeded.

If any item fails, the stage is **not** complete. Fix the problem, record it in Section 9, and repeat the gate.

Optional: if the owner wants to keep the verified console output, save it as a new, clearly labelled file in `results/console/` following the rules in 5.15. This is not required.

### Step 8 — Record, push, and tag (only after Step 7 passes)
- Fill in "What actually happened" in `docs/stages/STAGE_00.md` (Section 9).
- Go through the completion checklist (Section 8).
- **Commit:** `Stage 0: record stage completion`
- Push to GitHub. Create the tag `stage-0` and push the tag.
- The completion commit and the `stage-0` tag are created **by the owner, or on the owner's explicit instruction, and only after Step 7 has passed.** Copilot must never create them on its own initiative.

Each commit message also includes a short "why" and "what was verified" (Brief, Section 13.1).

---

## 7. Things the owner should understand by the end

Not a test — a learning guide. The owner should be able to explain, in plain words:

1. The difference between a header (`.h`) and a source file (`.cpp`).
2. What `#pragma once` prevents.
3. What a namespace is for.
4. Why `main()` returns 0, and what a non-zero return means.
5. What a CMake *target* is, and the difference between a library and an executable.
6. What "linking" the program to the library means.
7. How CTest decides whether a test passed (the test program's return value).
8. Why the deliberate failure check matters.
9. Why build folders and `.idea/` are not committed, and how to check whether a file is ignored.
10. What each file in `docs/` and `results/` is for.
11. (*v1.2*) Why Git history must never be rewritten in a public research repository.

If any of these is unclear, ask Claude, ChatGPT, or Copilot to explain it **before** marking the stage complete (Brief, Section 14).

---

## 8. Stage 0 completion checklist (*updated in v1.2*)

General criteria (Brief, Section 14) plus Stage 0 specifics:

**Approval and history**
- [ ] This plan (v1.2) is approved by the owner and stored in `docs/stages/STAGE_00.md`.
- [ ] The approved brief v1.1 (with v1.0), this plan, and the owner's organizational files were committed **before** any Stage 0 code (Step 1a).
- [ ] The existing Git history is intact: no force-pushes, rebases, or amended pushed commits.
- [ ] The owner verification gate (Step 7) was passed **before** the completion commit and tag were created.

**Files**
- [ ] Every file in Section 3 exists.
- [ ] The only C++ source and test files are `src/main.cpp`, `src/core/project_info.h`, `src/core/project_info.cpp`, and `tests/test_main.cpp`. No CLion template `main.cpp` remains at the top level.
- [ ] Nothing from Section 2 ("must NOT contain") was added.
- [ ] `LICENSE` is unchanged from the owner's version.
- [ ] `docs/chatgpt_reply.txt` and both files in `results/` are unchanged from the owner's versions.
- [ ] `docs/AI_Hologram_Project_Brief_v1.1.txt` is unchanged.

**Build, run, test**
- [ ] Project builds in CLion with **no errors and no warnings**, including after a cache reset (Step 6).
- [ ] `ai_hologram` prints exactly the three lines in 5.4.
- [ ] `ai_hologram_tests` prints the output in 5.5.
- [ ] CTest reports 1 of 1 tests passed.
- [ ] The deliberate failure check was done and CTest reported a failure.

**Git and ignore rules**
- [ ] `git status` shows no build folders, `.idea/`, object files, or executables tracked.
- [ ] `git check-ignore -v` confirms `docs/`, `results/` files, `LICENSE`, `README.md`, `src/`, `tests/`, and `CMakeLists.txt` are **not** ignored.

**Documentation**
- [ ] Every source file starts with the standard comment block.
- [ ] `.github/copilot-instructions.md` matches Brief v1.1 Section 15, plus the "Existing files and Git history" section from 5.8.
- [ ] README describes the two-agent architecture only as a **proposed research design**, states that no reasoning components exist and no experiments have been run, and claims no capabilities (5.7).
- [ ] DECISIONS.md contains D-001 to D-006.
- [ ] EXPERIMENT_LOG.md states that no experiments have been run.

**Completion**
- [ ] The owner can explain the items in Section 7.
- [ ] "What actually happened" is filled in.
- [ ] Commits are pushed and tag `stage-0` exists on GitHub.

---

## 9. What actually happened (filled in by the owner at the end)

```
Date completed:
Commit hash of tag stage-0:
Compiler and version used (shown in CLion's CMake output):
CMake version:

Step 1a: preservation commit hash (or "all files already committed"):

Did everything go as planned?  yes / no
Differences from the plan, and why:

Problems encountered and how they were solved:

Anything learned that should change future stage plans:
```

---

## 10. Instructions to GitHub Copilot for this stage

1. Read `.github/copilot-instructions.md` and this plan before writing anything.
2. Implement only the files marked **[STAGE 0]** in Section 3, following Section 5. Follow the step order in Section 6 — one step at a time, and wait for the owner to confirm each step works before starting the next. Begin with Step 2, and only after the owner confirms the Step 1a commit has been pushed (or that no commit was needed).

   **Existing files and Git history** (*added in v1.2*):

   - **2a.** Before creating or changing any file, inspect what already exists. If the file exists, read it first and tell the owner what is there.
   - **2b.** Never overwrite, delete, rename, reformat, or "clean up" the owner's files: `LICENSE`, `.gitignore`, everything in `docs/` (including the brief, this plan, and `chatgpt_reply.txt`), and everything in `results/`. Propose changes instead and wait for approval.
   - **2c.** Never select or change licensing terms, and never add licence headers or other licence files.
   - **2d.** Never rewrite Git history: no force-push, rebase, amending pushed commits, `reset --hard`, or deleting branches or tags. Never use `git add .` or `git add -A`; stage files by name.
   - **2e.** Treat `docs/chatgpt_reply.txt` as reference notes. If it conflicts with the brief or this plan, follow the brief and plan and point out the conflict to the owner.
3. Do not add anything from Section 2.
4. Do not add extra helper files, scripts, CI workflows, licence files, or configuration not listed here. Suggest them to the owner instead.
5. Do not rewrite the brief or the copilot-instructions rules.
6. After each step, tell the owner: which files changed, what each does in one sentence, how to build/run it, and what output to expect.
7. If anything in this plan is unclear or seems wrong, stop and ask.
8. Never create the Stage 0 completion commit or the `stage-0` tag on your own. These happen only after the owner has personally passed the verification gate in Step 7, and only by the owner or on the owner's explicit instruction.
9. In any text you write (README, comments, commit messages), describe the two-agent architecture only as a proposed research design. Never describe it as working or demonstrated.

---

## 11. Design choices recorded for reviewers

These choices were reviewed with v1.0/v1.1 and are kept unchanged:

1. **`project_info` in Stage 0.** A two-function core library so the program/library/test pattern exists from the start.
2. **Warnings are not errors.** A beginner is not blocked by a warning; the checklist still requires zero warnings.
3. **`.idea/` fully ignored.** Simplest choice.
4. **CMake 3.20 minimum.** Required for `ctest --test-dir`.

Updated in v1.2:

5. **`results/` exists from Stage 0** (owner decision, D-005), holding selected output only — not research results.
6. **`LICENSE` exists** (owner decision, D-006). Its terms are the owner's alone to choose or change.

---

## Revision history of this plan

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-10-19 | First draft by Claude, for independent review by ChatGPT and approval by the owner. |
| 1.1 | 2026-10-19 | Approved by the owner after ChatGPT recommended approval. Two clarifications requested by the owner: (1) new Step 1a — an initial commit preserving the approved brief v1.1, the original v1.0, and this plan before any code is written; Step 2 adjusted so those files are not re-added; (2) new Step 7 — an owner verification gate (build, program output, CTest, deliberate failure check, fresh build) that must pass before the completion commit and `stage-0` tag (now Step 8). Matching items added to the Section 8 checklist and the Section 10 Copilot instructions. No change to the architecture or the scope of Stage 0. |
| 1.2 | 2026-10-19 | Organizational revision requested by the owner after ChatGPT reviewed the repository. **No change to the AI architecture, the brief v1.1, or the C++ scope of Stage 0.** Changes: **(a)** Section 3 layout updated to the owner's actual structure, with [OWNER]/[STAGE 0] labels; owner additions (`LICENSE`, `docs/chatgpt_reply.txt`, `results/console/program_console_results.txt`, `results/debugging/debugging_output.txt`, owner's `.gitignore`) recorded as organizational, not research results; v1.1 note "results/ not created in Stage 0" superseded. **(b)** New 5.13 (`LICENSE` preserved; terms only changed with owner's permission), 5.14 (`chatgpt_reply.txt` purpose and order of authority), 5.15 (`results/console/` and `results/debugging/` hold selected reproducibility output; temporary or large logs need not be committed). **(c)** 5.6 changed from "create `.gitignore`" to "inspect the owner's `.gitignore`", including a check that required files are not accidentally ignored. **(d)** 5.7 README: added honesty requirement and required wording describing the two-agent architecture as a proposed research design; revise rather than replace an existing README; licence line added. **(e)** 5.8: copilot-instructions gains an "Existing files and Git history" section. **(f)** 5.10: decisions D-005 (organizational files) and D-006 (LICENSE) added. **(g)** Section 6: general rule that the repository is not empty; Step 1 inspects existing history and files; Step 1a rewritten as a preservation commit of approved documents and owner files on top of existing history, with a ban on history-rewriting commands; Steps 2, 3, 5 adjusted to inspect before changing. **(h)** Section 8 checklist reorganized and updated for the new structure, owner files, ignore checks, and README wording. **(i)** Section 10: Copilot rules 2a–2e (inspect first, preserve owner files, licence, Git history, chatgpt_reply.txt) and rule 9 (README honesty). **(j)** Section 7: two learning items updated/added. **(k)** Section 11 retitled and updated. Reviewed and approved by ChatGPT; **approved by the owner on 2026-10-19**. The only change made after approval was the Status line, updated to record the approval. |
