# Stage 4 reference implementation (Python; not project code)

Written by Claude to produce every exact expected value in `docs/stages/STAGE_04.md` (v1.0 to v1.2.2). It mirrors the C++ data flow of sub-stages 4a-4f. The C++ code matched it on the first run in every sub-stage (618 checks, commit `89ea9b6`).

| File | Purpose |
|---|---|
| `ref_c1.py` | The Stage 3 C1/C0 reference (copied from `docs/protocol_validation/ref_c1.py`). |
| `ref_stage4.py` | SplitMix64, the true-state search, layouts, fault-event lists, fault application (A3, A4), the run with faults. |
| `ref_score.py` | The scorer: phases, claims, event life, committed-prediction shadow, observers and effective use. |
| `ref_csv.py` | CSV rows and the Stage 4 program section. |
| `gen_*.py` | Scripts that print the plan's values. |
| `reference_logs.txt` | Their output (v1.2). |
| `expected_output_stage4.txt` | The program's exact expected output (170 lines). |

Run from this folder with Python 3, for example `python3 gen_score2.py`. Only demonstration seeds 500000-500009 are run with agents, with one disclosed exception below.

**`gen_c.py`: a one-off crash check, preserved for completeness.** On October 20, 2026, while preparing plan v1.0, Claude ran the **Python reference** C0 and C1 with faults once on the 80 pilot runs (layouts 900000-900039 x run seeds 1-2). Its only purpose was to check that the reference did not crash, and that activation classes occurred as expected (protocol 9.2 "does it work"). Only the three first runs, pooled outcomes over both conditions, and activation counts were printed; no per-condition score was computed or examined (protocol 9.3). This was disclosed to ChatGPT in the v1.0 review note. The exact console output is preserved in `gen_c_output_2026-10-20.txt`.

It was run with the reference as it was **before** plan v1.1.1: before the activation point was added to the early safe-stop path, and before the dead-square map followed the current belief. Re-running it with the files here may therefore give slightly different numbers. The pilot material must not be run again during development (rebuild rule R21).
