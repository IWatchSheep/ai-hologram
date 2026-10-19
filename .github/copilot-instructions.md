# AI Hologram — Instructions for GitHub Copilot

The governing document is `docs/AI_Hologram_Project_Brief_v1.1.txt`. If these instructions and the brief ever disagree, the brief wins and the owner must be told.

You are assisting with the AI Hologram research project, a C++17 / CMake
project developed in JetBrains CLion on Windows. The project owner is
learning C++. Readability and honesty matter more than speed or cleverness.

BEFORE WRITING CODE
  1. Read the current approved stage plan in docs/stages/. Implement ONLY
     what that plan describes.
  2. If the plan is unclear, incomplete, or seems wrong, STOP and ask the
     owner. Do not guess and do not redesign.
  3. Do not implement future stages, even if they seem easy or helpful.

WHILE WRITING CODE
  4. Use C++17 and the standard library only. Do not add libraries,
     frameworks, networking, threads, or GUI code.
  5. Keep main.cpp short. Put each component in its own .h/.cpp pair.
  6. Keep files under ~200 lines and functions under ~40 lines.
  7. Start each file with a comment: its purpose, the stage that introduced
     it, and how it fits the architecture.
  8. Write beginner-friendly comments explaining WHAT and WHY.
  9. Prefer simple constructs: struct, class, enum class, std::vector,
     std::array, std::string, std::optional, const references.
     Avoid raw new/delete, macros, deep inheritance, and advanced templates.
  10. Use std::mt19937 with an explicit seed for any randomness.
  11. Code must compile without warnings with the project's warning flags.
  12. Agents communicate only through explicit message structs. One agent
      must never read or modify the other's internal data directly.
  13. Never give one agent automatic authority over the other. Never treat
      agreement between agents as proof of correctness.
  13a. (v1.1) "Take no action" (a safe stop) must always be a possible
      outcome of a reasoning step. Do not write code that forces an action
      every step.
  13b. (v1.1) Do not assume a mismatch means a reasoning error. Record the
      suspected source (reasoning, belief, model, observation, unknown).
  13c. (v1.1) A correction is not accepted just because it fits past
      observations. Track its status (proposed, consistent, verified,
      rejected) as described in the brief.
  13d. (v1.1) Every state prediction must go through the shared budget
      counter. Never predict a state "for free."

TESTING
  14. Add correctness tests for new behaviour in tests/.
  15. Never weaken a baseline (e.g. the competent single agent) or adjust
      experiment code to make a result look better.
  16. Never change a test merely to make it pass. If a test seems wrong,
      explain why and ask the owner.

AFTER WRITING CODE
  17. Summarise for the owner: files added or changed, what each does, how
      to build and run, what output to expect.
  18. Point out anything you were unsure about.
  19. Do not claim something works unless it has been built and run.
  20. Do not describe untested hypotheses as achieved capabilities, in code
      comments, README text, or commit messages.

CHANGE SIZE
  21. Prefer several small changes over one large one. If a requested change
      would touch more than a few files, propose splitting it.

## Existing files and Git history

- **2a.** Before creating or changing any file, inspect what already exists. If the file exists, read it first and tell the owner what is there.
- **2b.** Never overwrite, delete, rename, reformat, or "clean up" the owner's files: `LICENSE`, `.gitignore`, everything in `docs/` (including the brief, this plan, and `chatgpt_reply.txt`), and everything in `results/`. Propose changes instead and wait for approval.
- **2c.** Never select or change licensing terms, and never add licence headers or other licence files.
- **2d.** Never rewrite Git history: no force-push, rebase, amending pushed commits, `reset --hard`, or deleting branches or tags. Never use `git add .` or `git add -A`; stage files by name.
- **2e.** Treat `docs/chatgpt_reply.txt` as reference notes. If it conflicts with the brief or this plan, follow the brief and plan and point out the conflict to the owner.
- **2f.** Exception to rule 2b: after finishing each piece of work, append a handoff note to `results/notes_from_github_copilot/Response_from_Github_copilot.txt`. Add to the end of the file only. Never delete, rewrite, or reorder earlier notes. This is the only owner file you may write to.
- **2g.** Running programs and tests: this project is built with CLion's bundled MinGW. Before running any built program or CTest in a terminal, first run `$env:Path = "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\mingw\bin;" + $env:Path` in the same terminal. Otherwise Windows reports a missing `libgcc_s_seh-1.dll` (exit code `0xc0000135`), which is an environment issue, not a code problem. Never change `CMakeLists.txt` link settings, add static-linking flags, or copy DLL files into the project to work around it.