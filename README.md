# ai-hologram

## About

**AI Hologram** is a research project that will investigate an alternative approach to artificial reasoning that does not use a large language model as its central reasoning mechanism. The project **proposes** a design in which two cooperating reasoning agents — one emphasising incremental, step-by-step reasoning and one emphasising temporal-spatial reasoning — have overlapping abilities and check each other's predictions against what is actually observed. Whether this design detects and corrects errors better than a competent single agent is an **open research question**. It has not yet been implemented or tested.

## Current status

**Current status: Stage 4 — fault injection and measurement.** The project contains a 10×10 grid world (Stage 1), a world model and comparison (Stage 2), and C1, the competent single agent, with C0, a naive reference (Stage 3). Stage 4 adds the experiment's measuring instrument: seeded generation of layouts and fault-event lists, fault injection into a reasoner's beliefs or reasoning exactly as the frozen Experiment 1 protocol defines, scoring of detection and correction from the agent's own records, and CSV output. Demonstrations use demonstration seeds only. **No two-agent reasoning exists, and no experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

**Archived on October 20, 2026.** Stage 4 code is complete (618 checks, 0 failures; commit `89ea9b6`), but the Stage 4 completion gate and the `stage-4` tag were **not** done. The owner decided to rebuild the C++ code from the beginning in a new repository, under improved rules agreed with ChatGPT; the frozen Experiment 1 protocol (unchanged), its validation, the design inputs and the reference implementations are to be carried there with recorded provenance. Using a re-implemented C1 as the experiment's baseline requires a separately approved migration addendum (M1); this notice does not record that approval. This repository is the permanent record: it is never changed, rewritten or deleted.

## Where to start reading

- [Project brief](docs/AI_Hologram_Project_Brief_v1.1.txt)
- [Decisions](docs/DECISIONS.md)
- [Experiment log](docs/EXPERIMENT_LOG.md)
- [Stage plans](docs/stages/)

## How to build and run in CLion

Open the project folder in CLion, let it load the CMake project, select the `ai_hologram` configuration, then build and run it.

## How to run the tests

In CLion, run the `ai_hologram_tests` configuration or use a CTest configuration. From a plain PowerShell terminal on Windows, first add CLion's MinGW folder to `PATH`, then run CTest by its full path:

```powershell
$env:Path = "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\mingw\bin;" + $env:Path
& "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\cmake\win\x64\bin\ctest.exe" --test-dir cmake-build-debug --output-on-failure
```

The CLion version in the path may differ on other computers. Running **All CTest** inside CLion avoids this.

## Requirements

CMake 3.20 or newer and a C++17 compiler. CLion's bundled MinGW works on Windows.

## How the project is developed

The project proceeds in small approved stages, with each stage built, run, and reviewed before the next. The owner leads and approves each stage; Claude develops the architecture and plans, ChatGPT independently reviews, and GitHub Copilot implements only the currently approved plan. The owner builds, runs, and verifies each stage before proceeding, as described in [Brief Section 9](docs/AI_Hologram_Project_Brief_v1.1.txt).

## Licence

See the `LICENSE` file.
