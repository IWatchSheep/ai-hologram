# Design Decisions

This file records project design decisions, with the newest entries at the bottom.
Add one short entry for each decision using the format below.

| ID | Date | Decision | Reason | Alternatives considered | Who decided |
|---|---|---|---|---|---|
| D-001 | 2026-10-19 | Adopt AI_Hologram_Project_Brief_v1.1 as the approved starting specification. | Revised after independent review; four corrections applied. | Keep v1.0; further revision first. | Owner (after review by ChatGPT, drafted by Claude) |
| D-002 | 2026-10-19 | Use hand-written tests registered with CTest; no test framework. | No external dependencies in early stages (Brief 8.2); easy for a beginner to read. | GoogleTest, Catch2. Can be revisited later. | Owner |
| D-003 | 2026-10-19 | Put shared code in a static library (`ai_hologram_core`) used by both the program and the tests. | Tests must check the real code, not a copy. | Compile source files into both executables separately. | Owner |
| D-004 | 2026-10-19 | C++17 required, compiler extensions off, warnings on but not treated as errors. | Portability and readable diagnostics while learning. | Warnings as errors from the start. | Owner |
| D-005 | 2026-10-19 | Add organizational files: `docs/chatgpt_reply.txt`, `results/console/`, `results/debugging/`, and the owner's `.gitignore`. These are organizational only and are not research results. | Preserve review notes and selected program output so development can be followed and reproduced. | Keep review notes outside the repository; create `results/` only when experiments begin. | Owner |
| D-006 | 2026-10-19 | `LICENSE` file added by the owner. Its terms are chosen and changed only by the owner. | Licensing is a personal decision with legal effect. | — | Owner |
