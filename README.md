# ai-hologram

## About

**AI Hologram** is a research project that will investigate an alternative approach to artificial reasoning that does not use a large language model as its central reasoning mechanism. The project **proposes** a design in which two cooperating reasoning agents — one emphasising incremental, step-by-step reasoning and one emphasising temporal-spatial reasoning — have overlapping abilities and check each other's predictions against what is actually observed. Whether this design detects and corrects errors better than a competent single agent is an **open research question**. It has not yet been implemented or tested.

## Current status

**Current status: Stage 0 — repository foundation.** The project currently contains only a minimal C++ program and one basic test that check the build setup. **No reasoning components exist yet. No experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

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
