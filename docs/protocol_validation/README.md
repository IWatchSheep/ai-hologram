# Protocol validation record (Experiment 1, Phase C)

This folder preserves the evidence used to validate protocol v1.0, Section 13, before the freeze (Section 15). It is **not project code**, and **not experimental results**.

| File | What it is |
|---|---|
| `ref_c1.py` | Claude's Python reference of C1 and C0, validated in Stage 3 (exact demo output: 19 and 414 evaluations) |
| `ref_two.py` | The reference of C2, C3 and C4 per Section 13 (v1.0), including 13.4a–c, 13.6a, 13.7, M_max per step, and A2 behind the flag `TRAP_CHECK_ON_OBJECTION` |
| `pilot.py` | Feasibility pilots (9.2): pilot layouts and seeds, fault events per 12.2 and 14, all five conditions. Run with `--wait --trapcheck` (A4 and A2). |
| `t_*.py` | The validation cases of 13.11, the A1 and A2 checks, the M_max deliberate-failure check, and fault-free solvability |
| `validation_logs.txt` | The outputs of all of the above, as reviewed by ChatGPT |

**How to rerun:** Python 3, no dependencies. From this folder, for example: `python3 -W ignore t_cases3.py`.

The Stage 6 plan will take its exact expected values from these cases, as Stage 3 did from `ref_c1.py`.
