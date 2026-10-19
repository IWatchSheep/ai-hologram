# Experiment 1 Protocol

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Document** | `docs/EXPERIMENT_1_PROTOCOL.md` |
| **Version** | **v0.3 — Phase A (framework)** |
| **Date** | October 19, 2026 |
| **Status** | **PHASE A APPROVED — FRAMEWORK ONLY. COMPLETE PROTOCOL NOT FROZEN.** Approved on **October 19, 2026** by ChatGPT (independent review) and by the owner. Sections 1–9 are approved; Phases B and C (Sections 10–15) are still to be written, reviewed and approved. The approved Phase A definitions and thresholds **must not be changed** because of pilot performance or observed experimental results. The `protocol-1-approved` tag is reserved for the complete, frozen protocol. |
| **Governing specification** | `AI_Hologram_Project_Brief_v1.1.txt`, especially Sections 3, 4, 7, 11 and 12 |
| **Required by** | Brief Section 10, "Definition gate": this protocol must be approved and frozen **before Stage 4** begins |
| **Project state when written** | Stage 1 complete (tag `stage-1`, commit `72a11fe`). No reasoning code exists. **No experiment has been run.** |

---

## How this document is built

The protocol is written in three phases, each when it can be written accurately (agreed in `STAGE_01.md` Section 14):

| Phase | When | Content | Status |
|---|---|---|---|
| **A — Framework** | After Stage 1 | Sections 1–9 below | **APPROVED 2026-10-19** (not frozen) |
| **B — Definitions and baseline behaviour** | After Stage 2; **before** `STAGE_03.md` is approved | Sections 10–12 (placeholders below) | Not started |
| **C — Numbers and freeze** | During Stage 3, before Stage 4 | Sections 13–15 (placeholders below) | Not started |

Labels follow the brief: **[FACT]**, **[PROPOSAL]**, **[HYPOTHESIS]**, **[OPEN]**.

**Rule for this whole document:** nothing in it may be changed after any confirmatory result has been run or seen (Section 9.5).

**Aim of the design** *(v0.2)*: the goal is not to make the two-agent architecture look successful. It is to build an experiment that can tell whether it gives an **advantage**, **no advantage**, or a **disadvantage**, and to report whichever is found.

---

## 1. Purpose

[FACT] Experiment 1 is the first test of the project's central idea (Brief Section 1): that two reasoning agents with **overlapping** abilities and **different methods**, which check each other's predictions against observation, detect and correct errors in reasoning better than a **competent single agent** given the same information and the same computational budget.

[PROPOSAL] This protocol fixes, **before any experiment runs**:
- what is being tested (Section 2);
- which systems are compared (Section 3);
- which errors are introduced, and how exposure to them is counted fairly (Section 4);
- what is measured (Section 5);
- how comparisons are made and how a result is judged (Sections 6–7);
- how results are reported, including failures and identical results (Section 8);
- what trial runs may and may not be used for (Section 9).

[FACT] A result showing **no benefit**, or **harm**, from the two-agent design is a valid and useful outcome, and will be reported as clearly as a positive one (Brief 1.3).

---

## 2. Hypotheses (pre-registered)

These are copied from Brief Section 11.7 and made precise. **They are written before any code that could test them exists.**

| ID | Hypothesis | Role | Comparison | Measure (Section 5) |
|---|---|---|---|---|
| **H1** | The two-agent overlapping system (C3) detects a higher proportion of faults than the competent single agent (C1), under equal budget. | **Co-primary** | C3 − C1 | M2 strict fault detection rate |
| **H2** | C3 does **not** have a meaningfully higher false-alarm rate than C1. | **Co-primary (safety)** | C3 − C1 | M3 false alarms per 100 decision steps |
| **H3** | C3 detects more faults than two identical copies of the competent agent (C2): **method diversity** matters, not mere duplication. | Secondary | C3 − C2 | M2 strict fault detection rate |
| **H4** | C3 achieves a higher verified correction rate than the same two agents with exclusive roles (C4): **overlap** matters. | Secondary | C3 − C4 | M4 verified correction rate |
| **H5** | C1 completes the task more often than the naive agent (C0): checking predictions helps at all. | **Diagnostic** (sanity check; not part of the confirmatory conclusion) | C1 − C0 | M1 task success rate |

### 2.1 How H1 and H2 together give the primary conclusion *(v0.2)*

[PROPOSAL] The **primary conclusion** of Experiment 1 is positive **only if both** H1 shows a meaningful improvement **and** H2 shows that false alarms are not meaningfully higher. A detection gain that comes with many more false alarms is not a clear improvement. Section 7.4 gives the full table, and H1 and H2 are **always** reported side by side.

Because the positive conclusion requires **both** to succeed, testing two co-primary hypotheses does not increase the chance of a false positive conclusion. Each is therefore judged with a 95% interval (Section 6.4).

### 2.2 H5 is a diagnostic check *(revised in v0.2)*

[PROPOSAL] H5 checks that checking predictions helps at all. **If H5 does not show C1 clearly ahead of C0, this is a diagnostic warning that must be investigated, not automatic proof that the experiment is invalid.** Possible innocent reasons include layouts that are easy enough that even a naive agent usually succeeds, or faults that rarely affect a naive plan. Possible serious reasons include broken checking code or broken fault injection.

- H1–H4 are **always reported**, whatever the H5 result.
- If H5 does not reach "Meaningful improvement" (Section 7.2), every H1–H4 result is reported with the qualification **"H5 diagnostic warning: see investigation"**, and the investigation and its findings are written up alongside the results.
- An H5 warning does not, by itself, change any H1–H4 verdict.

---

## 3. Conditions

From Brief 11.4. All conditions receive the **same layouts, seeds, fault-event list, observations, per-step budget, maximum world steps, and safe-stop limit** (Brief 11.5).

| ID | Name | Description | Role |
|---|---|---|---|
| **C0** | Naive single agent | Plans once, never checks predictions. | Reference floor; used only by H5. |
| **C1** | Competent single agent | Full predict → act → observe → compare → correct → replan loop, with look-ahead, safe stops, mismatch-source labels, and two-part correction verification (Brief 4.2–4.5). | **Main baseline.** |
| **C2** | Two identical competent agents | Two copies of C1's method, cross-checking each other through messages. | Tests duplication alone (Lesson L4). |
| **C3** | Two-agent overlapping architecture | Agent 1 (incremental) + Agent 2 (temporal-spatial), different methods, overlapping abilities, mutual checking, verified correction (Brief 3–4). | **The hypothesis.** |
| **C4** | Two-agent exclusive architecture | The same agents as C3, but each checks only its own area and trusts the other. | Tests overlap (Lesson L3). |

[PROPOSAL] **Protecting the baseline** (from `STAGE_01.md` Section 14):
- C1's behaviour is defined in **Phase B** and approved **before** `STAGE_03.md` is approved.
- C1 is built in Stage 3 and tagged **before** any two-agent code exists (Stage 6).
- After that, C1 changes only to fix a genuine bug. Each fix is recorded in `DECISIONS.md`, and **every** condition is re-run.
- C1 is never adjusted in response to how C3 performs.

[OPEN] Exact behaviour of each condition: **Phase B** (C0, C1, and shared definitions) and **Phase C** (C2–C4).

---

## 4. Faults and exposure

### 4.1 Fault types

From Brief 11.3. Faults are injected into **reasoning** (beliefs or reasoning steps), never into the world. The world in Experiment 1 is deterministic and fully observed (Brief 11.2).

| ID | Fault type | Plain description |
|---|---|---|
| **F1** | Wrong wall belief | The reasoner believes a free cell is blocked, or a blocked cell is free. |
| **F2** | Wrong object position | The reasoner believes the object is one cell away from where it is. |
| **F3** | Faulty action model | The reasoner's model of what an action does is wrong (e.g. believes "push east" moves the object north). |
| **F4** | Faulty consequence reasoning | The reasoner fails to recognise that a move leads to a trap. |
| **F5** | Stale memory | The reasoner uses an old state after the world has changed. |

### 4.2 Assignment rules (from Brief 11.5.1, restated)

- Fault events are defined **per system**: each run has one fault-event list (step, type, details), generated from the run's seed and **identical for every condition**.
- **C0 / C1:** every event is applied to the single reasoner.
- **C2 / C3 / C4:** each event is applied to **one** reasoner, chosen by a separate seeded choice, **balanced** so each reasoner receives half of each fault type.
- **Common-mode events:** a fixed share of events is applied to **both** reasoners at once (share fixed in Phase C). In C0 and C1, a common-mode event is simply applied to the single reasoner.
- A fault **persists** until it is corrected or the run ends.
- If a fault type has no meaning for some reasoner, this is written down in Phase B, and results are always reported **per fault type** so the gap stays visible.

### 4.3 Scheduled, activated, and detectable faults *(added in v0.2)*

Runs can end at different steps in different conditions (by success, safe-stop limit, or step limit). A fault scheduled for step 30 never happens in a run that ended at step 20. So every fault event is classified as:

| Class | Meaning |
|---|---|
| **Scheduled** | On the run's fault-event list. |
| **Activated** | Actually injected, because the run reached the event's scheduled step. |
| **Not activated** | Scheduled, but the run ended before its step. |
| **Detection opportunity** | Activated, **and** the faulty belief or reasoning step was actually used in at least one prediction or decision before the fault was corrected or the run ended. *(Exact definition: Phase B.)* |

[PROPOSAL] **Requirements:**
- **Non-activated faults are never counted as detected or as missed.** They are excluded from detection and correction denominators, and the **number** of non-activated faults is reported for every condition and fault type.
- Activated faults that never had a detection opportunity are reported separately, as a count per condition and fault type, so that a condition is not credited or blamed for faults that could not have mattered.
- **A condition cannot improve its detection score by stopping early or acting less.** Activated faults that were never detected count as **missed**, whatever the run's outcome, including "Stopped — unresolved".

### 4.4 Fair exposure in comparisons *(added in v0.2)*

Two conditions may activate different sets of faults in the same (layout, seed) run, because their runs end at different steps. To compare them fairly:

[PROPOSAL]
- **Primary comparison set (common exposure):** for each comparison (for example C3 vs C1), detection and correction rates are computed over the fault events that were **activated in both** conditions for that (layout, seed) pair. Both conditions are then judged on exactly the same faults.
- **Secondary (all activated):** rates over all faults each condition activated are also reported, clearly labelled, so any effect of different exposure is visible.
- If the common-exposure set excludes a large share of activated faults (threshold fixed in Phase C), this is reported prominently as a limitation of the comparison.

**Safeguards for matched exposure** *(added in v0.3)*:
- **Retained fraction.** For every comparison, the report states the fraction of **scheduled** fault events retained in the common-exposure set, overall and per fault type. It also states the fraction of each condition's **activated** events that were retained.
- **Zero common exposure.** A layout can have **no** fault events activated in both conditions of a comparison. Its rate for that comparison is then **undefined**: it is **never** computed by dividing by zero, and is **never** treated as 0% or 100%.
  - Such a layout is left out of that comparison's common-exposure average **only**, and it remains in every other measure (task success, safe stops, false alarms, exposure counts, and the all-activated results).
  - The number and identity of these layouts are reported for every comparison. They are never excluded silently.
  - The bootstrap (6.3) resamples only the layouts with a defined difference for that measure, and the report states how many there were.
  - If **no** layout has a defined common-exposure difference for a comparison, that hypothesis's verdict is **Inconclusive (no common exposure)**.
- **What matching cannot fix.** Restricting to faults activated in both conditions makes the comparison fairer, but it does **not** remove every effect of different stopping times. The retained faults are, by construction, those occurring earlier in runs, and may be easier or harder than average. One condition's runs may also have had more time to detect a fault after it was activated. Matched exposure is therefore treated as a **fairer** comparison, not a perfect one.
- **Always reported together.** Common-exposure results are **always** reported alongside the all-activated results, task success (M1), safe stops (M7), and exposure counts (M12). No common-exposure result may be presented on its own.

---

## 5. Measurements

Each measure is listed with its **concept**. Exact operational definitions are written in **Phase B**, to match the real code.

### 5.1 Run outcomes (each run ends in exactly one)

| Outcome | Meaning |
|---|---|
| **Success** | The object reaches the destination. |
| **Stopped — unresolved** | The safe-stop limit was reached (Brief 4.2, step 4b). Reported separately, never counted as success. |
| **Step limit** | The maximum number of world steps was reached without success. |
| **Software failure** | A crash or invalid internal state. This is a **bug**, not a result (Section 8.4). |

### 5.2 Three levels of detection *(added in v0.2)*

Noticing that something is wrong is different from finding out **what** is wrong. These are measured separately:

| Level | Concept |
|---|---|
| **Mismatch detection** | While an activated fault was in effect, the system recorded a mismatch (between prediction and observation, or between the two agents' predictions) that the fault caused. The system noticed that something was wrong. |
| **Reasoner attribution** | Mismatch detection, **and** the system correctly identified **which reasoner** held the fault. *(Two-reasoner conditions only; in C0/C1 there is only one reasoner.)* |
| **Strict fault detection** | The system correctly identified **both** the faulty reasoner **and** the faulty belief element or reasoning step. |

**Strict fault detection is the primary detection measure** (M2), because it is what the architecture claims to improve: finding the cause, not only noticing a problem.

**Common-mode events** (both reasoners faulted, Section 4.2) are counted **once per event**, so the denominator is the same for one- and two-reasoner conditions:
- A common-mode event is **strictly detected** if the faulty belief element is correctly identified in **at least one** affected reasoner.
- For two-reasoner conditions, **full common-mode detection** (correctly identified in **both** affected reasoners) is reported separately.
- All detection results are reported separately for single-reasoner and common-mode events.

### 5.3 Measures

| # | Measure | Concept | Used by |
|---|---|---|---|
| M1 | **Task success rate** | Runs ending in Success ÷ all runs. | H5 |
| M2 | **Strict fault detection rate** | Strictly detected fault events ÷ fault events in the comparison set (common exposure, Section 4.4). | H1, H3 |
| M2a | Mismatch detection rate | As M2, at the mismatch-detection level (5.2). | Reported |
| M2b | Reasoner attribution rate | As M2, at the reasoner-attribution level (two-reasoner conditions). | Reported |
| M3 | **False alarms per 100 decision steps** | Claims that an error exists, made when **no** activated fault was present in the accused reasoner and belief element, per 100 decision steps. | H2 |
| M3a | **Total false alarms** | The raw count of false alarms per condition. Always reported alongside M3. *(v0.2)* | Reported |
| M4 | **Verified correction rate** | Fault events corrected **and** verified (Brief 4.5, status VERIFIED) with a corrected belief that matches the ground truth ÷ fault events in the comparison set. | H4 |
| M5 | Detection delay | Decision steps from activation to strict detection, for detected events. | Reported |
| M6 | Wrong accepted corrections | Corrections accepted (CONSISTENT or VERIFIED) whose corrected belief was **wrong**. | Reported |
| M7 | Safe stops | Number of safe stops; how many happened while a fault was active ("justified") vs not. | Reported |
| M8 | Source labelling accuracy | How often a mismatch's suspected source (S1–S5, Brief 4.4) matched the ground truth. | Reported |
| M9 | Disagreement outcomes | When agents disagree, how often the objecting agent was right (Brief Q6). Two-agent conditions only. | Reported |
| M10 | Steps to completion, and wasted steps | For successful runs. | Reported |
| M11 | Budget used | Model evaluations used, total and per reasoner (Brief 11.5.3). | Reported; confirms budgets were equal |
| M12 | Exposure counts | Scheduled, activated, not-activated, and no-opportunity fault events, and decision steps, per condition. *(v0.2)* | Reported |

### 5.4 Fairness requirements for rates and run lengths *(added in v0.2)*

[PROPOSAL]
- **Decision steps** include every step at which the system made a decision, **including safe-stop steps**, so a system cannot lower its false-alarm rate by stopping often. *(Exact definition: Phase B.)*
- False alarms are reported **both** as a rate (M3) and as a total (M3a), with decision-step totals (M12), so differences in run length are visible.
- Safe stops are reported per condition (M7). A run that ends "Stopped — unresolved" keeps all its activated faults in the denominators (Section 4.3).
- All conditions share the same maximum world steps and safe-stop limit (Section 3).

[OPEN] Precise event definitions for "claim that an error exists", "correctly identified", "used in a prediction", and "decision step": **Phase B**.

---

## 6. Unit of analysis and comparison method

[PROPOSAL]

### 6.1 Paired design
Every condition runs on exactly the same (layout, seed) pairs with the same fault-event list, so conditions are compared **pair by pair**, not as unrelated groups. This removes most of the noise caused by some layouts being harder than others.

### 6.2 Unit of analysis: the layout
Runs that share a layout are not independent. For each comparison, each layout and each condition, a measure is computed by pooling over that layout's seeds (using the common-exposure set for M2 and M4). The difference between the two conditions is then computed **per layout**.

### 6.3 Uncertainty: bootstrap over layouts
The difference between conditions is averaged over layouts, and a **confidence interval** is computed by resampling layouts with replacement (a "bootstrap", using the percentile method; 10,000 resamples, with a fixed, recorded random seed). This is a standard, assumption-light method, and simple enough to implement and explain.

### 6.4 Interval widths and multiple comparisons *(corrected in v0.2)*

| Hypotheses | Interval | Reason |
|---|---|---|
| **H1, H2** (co-primary) | **95%** each | The positive primary conclusion requires **both** to succeed (2.1), so no adjustment is needed. |
| **H3, H4** (secondary) | **97.5%** each | Bonferroni adjustment for two secondary comparisons sharing a 5% error allowance (0.05 ÷ 2 = 0.025 each). *(v0.1 wrongly stated 98.3%.)* |
| **H5** (diagnostic) | **95%** | Diagnostic only (2.2); not part of the confirmatory error control. |

### 6.5 Sample size
Layout and seed counts (N layouts × M seeds) are fixed in **Phase C**, before any confirmatory run. Pilots may only check that runs complete in reasonable time (Section 9). They may **not** be used to estimate effect sizes or to pick sample sizes that favour any result.

---

## 7. Decision rules

Every hypothesis gets exactly **one** verdict, decided mechanically by the rules below. The rules are applied **in the order listed**, and the **first rule that matches decides the verdict**.

### 7.1 Smallest meaningful differences (Δ)

| Measure | Δ |
|---|---|
| M2 strict fault detection rate (H1, H3) | **10 percentage points** |
| M4 verified correction rate (H4) | **10 percentage points** |
| M1 task success rate (H5) | **10 percentage points** |
| M3 false alarms per 100 decision steps (H2) | **1.0** (non-inferiority margin) |

[PROPOSAL] *(v0.2)* These thresholds are **research design judgments** made by the project team for this experiment. They are **not** objectively established or universal values. They express the smallest difference this project would consider worth the added complexity of a second agent. They are fixed once Phase A is approved, and may **not** be changed by pilots or by any later phase.

### 7.2 Verdicts for improvement hypotheses (H1, H3, H4, H5) *(revised in v0.2)*

Let *D* be the average per-layout difference (for example C3 − C1), and [*L*, *U*] its confidence interval (Section 6.4).

| Order | Verdict | Rule | Plain meaning |
|---|---|---|---|
| 1 | **Worse** | *U* < 0 | The tested system is reliably worse. |
| 2 | **Meaningful improvement** | *L* ≥ Δ | The whole interval is at or above the meaningful size: the true improvement is very likely at least Δ. |
| 3 | **Meaningful improvement ruled out** | *U* < Δ | The whole interval is below the meaningful size: any true improvement is very likely smaller than Δ. **This does not necessarily mean there was no improvement** (see note below). If also *L* > 0, add the note **"small positive improvement"**. |
| 4 | **Some improvement, size uncertain** | *L* > 0 | Very likely better, but whether the gain reaches Δ is uncertain. |
| 5 | **Inconclusive** | none of the above | The interval is too wide to decide. |

**Boundary equality:** the comparisons are applied exactly as written (`<`, `≥`, `>`). For example, *L* exactly equal to Δ satisfies rule 2, and *L* exactly equal to 0 does **not** satisfy rule 4. Interval limits are computed and compared at full precision, and reported rounded to 4 decimal places.

**Only "Meaningful improvement" counts as support** for H1, H3, H4 or H5. "Some improvement, size uncertain" is reported honestly as weaker evidence, never as confirmation.

**Small positive improvements** *(added in v0.3)*: "Meaningful improvement ruled out" says only that the improvement is very likely **smaller than Δ**. It does **not** say the improvement is zero.
- If the interval lies **entirely above zero but entirely below Δ** (0 < *L* and *U* < Δ), there is evidence of a **small positive improvement**: the tested system is very likely better, but by less than this protocol's chosen meaningful size. It is **not** a meaningful improvement under this protocol.
- If the interval includes zero (*L* ≤ 0 and *U* < Δ), there may be no improvement at all, and any improvement is very likely smaller than Δ.
- **Reporting instruction:** whenever the verdict is "Meaningful improvement ruled out", the report states *D*, *L* and *U*, and says in plain words which of these two cases applies. A small positive improvement must never be described as "no effect", and must never be described as support for the hypothesis.

### 7.3 Verdicts for H2 (not meaningfully higher false alarms) *(revised in v0.2)*

Let *D* = (C3 false alarms per 100 decision steps) − (C1 false alarms per 100 decision steps), with 95% interval [*L*, *U*] and margin Δ = 1.0.

| Order | Verdict | Rule | Plain meaning |
|---|---|---|---|
| 1 | **Meaningfully higher** | *L* ≥ Δ | C3 very likely raises at least Δ more false alarms per 100 steps. H2 **not supported**. |
| 2 | **Not meaningfully higher** | *U* < Δ | Any increase is very likely smaller than Δ. H2 **supported**. If also *U* < 0, add the note **"fewer false alarms"**. |
| 3 | **Inconclusive** | otherwise | |

The same boundary-equality rule as 7.2 applies.

### 7.4 Primary conclusion from H1 and H2 together *(revised in v0.2)*

| H1 verdict | H2 verdict | Primary conclusion |
|---|---|---|
| Meaningful improvement | Not meaningfully higher | **Positive:** meaningful detection benefit without a meaningful increase in false alarms. |
| Meaningful improvement | Meaningfully higher | **Mixed:** detection benefit bought with meaningfully more false alarms. |
| Meaningful improvement | Inconclusive | **Mixed / uncertain:** detection benefit; the false-alarm cost is unknown. |
| Some improvement, size uncertain | any | **Not demonstrated:** some evidence of benefit, but a meaningful benefit was not shown. H2 reported alongside. |
| Meaningful improvement ruled out | any | **No meaningful benefit** under these conditions. |
| Worse | any | **Negative:** the two-agent design detected fewer faults than the competent single agent. |
| Inconclusive | any | **Inconclusive:** the experiment could not decide. Interval widths reported. |

[PROPOSAL] **No other interpretation is allowed in the summary.** H3, H4 and H5 are reported with their own verdicts beneath the primary conclusion. Additional analyses may be reported, but only clearly labelled as **exploratory**, never as tests of H1–H5.

---

## 8. Reporting rules

### 8.1 What is always reported
- Every hypothesis's verdict, *D*, and confidence interval, including those that fail or are inconclusive.
- Every measure in Section 5 for **every** condition, per fault type, split by single-reasoner vs common-mode events, and for both the common-exposure and all-activated sets.
- Exposure counts (M12), total false alarms (M3a), and safe stops (M7).
- The exact protocol version, the Git commit and tag of the code, and all settings used.
- Results are recorded in `docs/EXPERIMENT_LOG.md` as **expected vs observed** (Brief 12.3), with raw output in `results/` (Brief 12.4).

### 8.2 Expected results are written down first
Before the confirmatory run, the owner records in `EXPERIMENT_LOG.md` what he **expects** for each hypothesis. Observed results are added afterwards, without editing the expectations.

### 8.3 No averaging away differences
If results differ by fault type, they are reported by fault type. Mixed results are reported as mixed (Brief 11.7).

### 8.4 Software failures and exclusions
- A crash or invalid internal state is a **bug**. The run is **not** quietly excluded. The bug is fixed, recorded in `DECISIONS.md`, and **all conditions are re-run** (Brief 11.5.4).
- No run may be excluded for any reason not written in this protocol before the confirmatory run.

### 8.5 Language
Results are described with the brief's labels: an outcome is an **[OBSERVATION]** about this experiment, never a general claim about AI. No README or commit message may describe the two-agent design as "working" on the basis of Experiment 1 alone.

---

## 9. Pilot runs

From `STAGE_01.md` Section 14, confirmed by ChatGPT's review.

### 9.1 Separate material
Pilot runs use layouts and seeds that are **never** used in the confirmatory experiment. The two sets are listed separately in Phase C.

### 9.2 Allowed purposes only
Pilots may be used **only** to find and fix:
- crashes or hangs;
- broken instrumentation (counters or logs that record the wrong thing);
- impossible budgets (so small that no condition can act at all);
- experiments that cannot finish (runs that never end, or limits that are never reached).

### 9.3 No tuning toward a result
- Comparative pilot performance (how one condition does relative to another) must **not** be used to choose **any** parameter. In particular, no parameter may be chosen because it makes C3 look better.
- Pilot reports show whether runs **worked**, not condition-by-condition scores.
- The decision thresholds (Section 7) and hypotheses (Section 2) are fixed in Phase A and are **never** affected by pilots.

### 9.4 Every change is documented
Each change made because of a pilot is recorded in `EXPERIMENT_LOG.md` (marked **PILOT — not evidence**) and in `DECISIONS.md`. The record says what was observed, what was changed, and why the change concerns whether the experiment works, not which condition wins.

### 9.5 Freeze before confirmatory results
The final protocol is frozen and tagged **`protocol-1-approved`** **before** any confirmatory run is started or any confirmatory result is examined. Pilots happen only before the freeze. After the freeze, nothing in this document changes. Errors found later are documented, and the experiment is re-run under a new, separately approved protocol.

---

## Phase B sections (placeholders; to be written after Stage 2)

### 10. Representations
How states, beliefs, predictions and mismatches are represented, matching the reviewed Stage 2 code.

### 11. Baseline behaviour (C0 and C1)
The full behaviour of C0 and C1:
- C1's predict → act → observe → compare → correct → replan loop;
- its look-ahead;
- safe stops and the stop limit's role;
- mismatch-source labels (S1–S5);
- two-part correction verification;
- how it spends its per-step budget.

**Must be approved before `STAGE_03.md` is approved.**

### 12. Fault application and event definitions
- How each fault type F1–F5 is applied to a belief or reasoning step.
- The exact event definitions for: activation; detection opportunity; the three detection levels (5.2); "claim that an error exists"; "correctly identified"; "decision step"; and verified correction.

## Phase C sections (placeholders; to be written during Stage 3)

### 13. Two-agent behaviour (C2–C4)
Built on the shared definitions in Phase B.

### 14. Numbers
- Budget per step B and how it is split;
- stop limit;
- maximum world steps;
- fault rate and common-mode share;
- the common-exposure warning threshold (4.4);
- N layouts × M seeds (confirmatory);
- the separate pilot layouts and seeds;
- the bootstrap seed.

### 15. Freeze record
Approval, `DECISIONS.md` entry, and the `protocol-1-approved` tag.

---

## Revision history

| Version | Date | Change |
|---|---|---|
| Draft v0.1 | 2026-10-19 | Phase A (framework) drafted by Claude after Stage 1 was completed and tagged (`72a11fe`). Sections 1–9 written; Phase B and C sections are placeholders. For independent review by ChatGPT and approval by the owner. Not approved; not frozen. |
| Draft v0.2 | 2026-10-19 | Revised after ChatGPT's review of v0.1, which supported the overall framework. **(1) Decision rules (7.2–7.4):** v0.1 called a result "Supported" when *L* > 0 and *D* ≥ Δ, which does not show the true improvement reaches Δ. Replaced by five ordered verdicts (Worse; Meaningful improvement, *L* ≥ Δ; Meaningful improvement ruled out, *U* < Δ; Some improvement, size uncertain, *L* > 0; Inconclusive), applied in order with defined boundary equality; H2 given three ordered verdicts; new primary-conclusion table. Thresholds explicitly described as research design judgments (7.1). **(2) Intervals (6.4):** corrected the Bonferroni interval for the two secondary hypotheses from 98.3% (an arithmetic error) to **97.5%**; explained why co-primary H1 and H2 use 95% each; H5 treated as diagnostic. New 2.1 on how H1 and H2 jointly decide the primary conclusion. **(3) Detection (5.2):** three detection levels (mismatch, reasoner attribution, strict), with strict detection kept as primary; common-mode events counted once per event, with full common-mode detection reported separately; new M2a and M2b. **(4) Exposure and fairness (4.3, 4.4, 5.4):** scheduled / activated / not-activated / detection-opportunity classes; non-activated faults never counted as detected or missed; primary comparisons on the common-exposure set, with all-activated reported as secondary; decision steps include safe-stop steps; total false alarms (M3a) and exposure counts (M12) always reported. **(5) H5 (2.2):** a failed sanity check is a diagnostic warning requiring investigation, not automatic invalidation; H1–H4 always reported, with a qualification. The brief is unchanged; no code written; no Phase B or C details added beyond the requirements they must meet. Not approved; not frozen. |
| Draft v0.3 | 2026-10-19 | ChatGPT reviewed v0.2, accepted all its revisions, and recommended approval of the Phase A framework after two final clarifications. **(1) 7.2:** the five verdicts and their rules are unchanged; added an explanation and reporting instruction that "Meaningful improvement ruled out" does not necessarily mean no improvement. An interval entirely above 0 but below Δ is evidence of a **small positive improvement**, not a meaningful one, and must never be called "no effect" or support. **(2) 4.4:** added matched-exposure safeguards: report the retained fraction of scheduled (and activated) fault events; layouts with zero common exposure give an **undefined** rate (never divide by zero, never treated as 0% or 100%), are left out of that average only, and are counted and named; a comparison with no defined layouts is "Inconclusive (no common exposure)"; matching does not remove every effect of different stopping times; common-exposure results are always reported with all-activated results, task success, safe stops and exposure counts. No other change to architecture, hypotheses, conditions, thresholds or design. Not frozen. |
| v0.3 — Phase A approved | 2026-10-19 | **Phase A (Sections 1–9) APPROVED** by ChatGPT's independent review and by the owner. ChatGPT confirmed both v0.3 clarifications and approved the hypotheses H1–H5, conditions C0–C4, fault types F1–F5, measurement categories, statistical comparison approach, and pilot-run safeguards. Only the status lines changed; the content is identical to Draft v0.3. **Framework only. The complete protocol is not frozen**; Phases B and C remain. The approved Phase A definitions and thresholds must not be changed because of pilot performance or observed results. The `protocol-1-approved` tag is reserved for the complete protocol. |
