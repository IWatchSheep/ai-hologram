# Experiment 1 Protocol

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Document** | `docs/EXPERIMENT_1_PROTOCOL.md` |
| **Version** | **v0.6 — Phase A and Phase B approved** |
| **Date** | October 19, 2026 |
| **Status** | **PHASE A APPROVED. PHASE B APPROVED. COMPLETE PROTOCOL NOT FROZEN.** Phase A (Sections 1–9) was approved on **October 19, 2026**, and Phase B (Sections 10–12) on **October 19, 2026**, each by ChatGPT (independent review) and by the owner. Phase C (Sections 13–15) is still to be written, reviewed and approved. The approved Phase A definitions and thresholds **must not be changed** because of pilot performance or observed experimental results. The `protocol-1-approved` tag is reserved for the complete, frozen protocol. |
| **Governing specification** | `AI_Hologram_Project_Brief_v1.1.txt`, especially Sections 3, 4, 7, 11 and 12 |
| **Required by** | Brief Section 10, "Definition gate": this protocol must be approved and frozen **before Stage 4** begins |
| **Project state when written** | Phase A: Stage 1 complete (tag `stage-1`, commit `72a11fe`). Phase B: Stage 2 complete (tag `stage-2`, commit `e112145`). No reasoning code exists. **No experiment has been run.** |

---

## How this document is built

The protocol is written in three phases, each when it can be written accurately (agreed in `STAGE_01.md` Section 14):

| Phase | When | Content | Status |
|---|---|---|---|
| **A — Framework** | After Stage 1 | Sections 1–9 below | **APPROVED 2026-10-19** (not frozen) |
| **B — Definitions and baseline behaviour** | After Stage 2; **before** `STAGE_03.md` is approved | Sections 10–12 below | **APPROVED 2026-10-19** (not frozen) |
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

## Phase B sections (APPROVED 2026-10-19; not frozen)

Phase B is written after Stage 2 (tag `stage-2`, commit `e112145`), so that its definitions match the reviewed code. It was approved on **October 19, 2026**, before `STAGE_03.md`. The approved Phase B definitions **must not be changed** because of pilot performance or observed results (as for Phase A). **[PROPOSAL]** labels inside Sections 10–12 mark the design as approved for Experiment 1. They do not mean the design has been shown to work. **Nothing in Sections 1–9 (Phase A) is changed.** Where Phase A says "exact definition: Phase B", the definition is given below.

### 10. Representations

#### 10.1 What already exists (Stage 2 code) — [FACT]
| Concept | Code (tag `stage-2`) |
|---|---|
| True state of the world | `WorldState` (agent, object, destination, 10×10 walls), held only by `Environment` |
| Validity | `isValidWorld` (one definition, `STAGE_01.md` 3.6) |
| True rules | `applyAction` → `ActionResult { state, outcome }`, outcome ∈ {Moved, Pushed, Blocked, InvalidState} |
| A reasoner's belief | `Belief { WorldState state; }` — a separate type from the true state |
| A prediction | `Prediction { action, outcome, state }` |
| Prediction machinery | `WorldModel::predict(belief, action, budget)` with a replaceable `ActionModel` (`std::function`) |
| Budget | `BudgetCounter`: one model evaluation per prediction; refuses when exhausted |
| Comparison | `compareStates` (every difference, fixed order), `comparePrediction`, `matches`, `describeDifferences` |

#### 10.2 Observation — [PROPOSAL]
- An **observation** is the exact `WorldState` after the most recent action, together with that action and its outcome. The first observation of a run is the initial state, with no action and no outcome.
- Every reasoner in every condition receives the **same, complete observation** at every decision step (Brief 11.5.2). Nothing is hidden or delayed.
- **History:** every reasoner keeps every observation it has received, exactly as received. Faults never change the history (F5 changes a belief, not the record).

#### 10.3 Belief and how it is kept up to date — [PROPOSAL] *(revised in v0.5)*
- A reasoner's initial belief is the **whole** first observation.
- After that, a belief changes **only** in these four ways. Each change is logged with its cause.
  1. **Footprint update after a matching check** (10.5). The check matched, so the predicted footprint contents equal the observed ones, and the update cannot change any belief value that was in the comparison.
  2. **Correction**, through a correction record (11.4, 11.5). This is the only **deliberate** repair.
  3. **Unexplained update.** After a mismatch, if no candidate passes V1, the reasoner must still bring its footprint into line with the observation to continue. Such an update is recorded as **unexplained**, never as a correction, and the mismatch is flagged as an anomaly.
  4. **Injected fault** (Stage 4; Section 12).
- A belief is **never silently overwritten** by an observation (Brief 11.5.1).
- The scorer separates deliberate repair (2) from any other change that happens to restore a true value (12.3).

#### 10.4 Belief elements (what a fault can affect and what a claim can name) — [PROPOSAL]
| Element | Meaning | Fault types |
|---|---|---|
| `wall(x, y)` | Whether the reasoner believes cell (x, y) is blocked | F1 |
| `object` | Where the reasoner believes the object is | F2, F5 |
| `model(a)` | The reasoner's action model for action *a* | F3 |
| `trap(p)` | Whether the reasoner's trap analysis classifies cell *p* as a **dead square**, as defined exactly in 11.2 | F4 |

The agent's own position and the destination are not fault targets in Experiment 1 (Brief 11.3 lists no such faults).

#### 10.5 The footprint of an action — [PROPOSAL] *(made fully deterministic in v0.5)*
**Definition.** For action *a* taken at decision step *t*:
- **c0** is the agent's cell in the reasoner's belief **before** acting.
  - The agent's position is never a fault target, and it is always in the footprint, so without a bug this equals the agent's cell in the recorded observation *o_t−1*.
  - If they ever differ, the step is flagged as an anomaly (a possible bug, Phase A 8.4), and the footprint is still built from the belief.
- **c1** is c0 moved one cell in direction *a*.
- **c2** is c1 moved one cell in direction *a*. It is included **if and only if** at least one of these holds:
  - **(i)** the believed pre-action state has the object at c1;
  - **(ii)** the observed outcome is Pushed;
  - **(iii)** the observed post-action state has the object at c1. (This happens only when a push was blocked.)
- Cells outside the grid count as walls, as in `isWall`.

The **footprint** is these cells **plus the outcome**.

**Footprint comparison.** For each footprint cell, compare:
- whether it is a wall;
- whether the agent is there;
- whether the object is there.

Compare these in the predicted post-action state and the observed post-action state, and compare the predicted and observed outcomes. The cells are chosen **before** comparing, from the belief and the observation only. They never depend on which architecture is running.

**Comparing two reasoners' predictions** (Phase C) uses the same rule:
- c0 and *a* are shared.
- c2 is included if **either** reasoner's believed pre-action state has the object at c1, or **either** predicts Pushed.

**Disagreement cases, worked out:**

| Case | Footprint | What the comparison reports |
|---|---|---|
| Predicted Moved, observed Moved, beliefs right | c0, c1 | match |
| Predicted Moved; observed Blocked by a true wall at c1 (believed free) | c0, c1 | outcome; wall at c1; agent at c0 vs c1 |
| Predicted Blocked (believed wall at c1); observed Moved | c0, c1 | outcome; wall at c1; agent position |
| Predicted Blocked (believed object at c1, c2 believed blocked); observed Pushed | c0, c1, c2 (i, ii) | outcome; agent position; object at c2; wall at c2 if believed |
| Predicted Moved (object believed elsewhere); observed Pushed (true object at c1) | c0, c1, c2 (ii) | outcome; agent position; object at c2 |
| Predicted Moved (object believed elsewhere); observed Blocked (true object at c1, c2 blocked) | c0, c1, c2 (iii) | outcome; object at c1; agent position |
| Predicted Pushed (object believed at c1); observed Moved (no object there) | c0, c1, c2 (i) | outcome; object at c2 vs none |
| Predicted and observed agent positions differ | follows from c0 and *a* as above | the agent-presence differences in c0 and c1 |

[FACT, from the Stage 1 rules R1–R8, all tested] An action can change **only** the contents of c0, c1 and c2.
- Walls and the destination never change.
- The agent moves only from c0 to c1.
- The object moves only from c1 to c2.

[OBSERVATION, checked by simulation for this draft] Over 2,000 random valid worlds × 60 random actions, every change fell inside the footprint, and a fault-free reasoner that updated only its footprint kept an exactly correct belief.

**Consequence:** a reasoner that updates its belief from the footprint after every step keeps an **exactly correct** belief whenever it has no fault. Stage 3 must test this (12.9).

Stage 3 adds a footprint comparison built on the Stage 2 types. The full `compareStates` is kept for logging and for scoring.

#### 10.6 Records — [PROPOSAL]
Stage 3 introduces these records. Field names are fixed in `STAGE_03.md`.
- **Mismatch record:**
  - decision step and action;
  - the prediction and the footprint differences;
  - the candidate elements considered;
  - the suspected source (S1–S5) and the evidence for it.
- **Correction record:**
  - identifier, element, old value and new value;
  - status (PROPOSED, CONSISTENT, VERIFIED or REJECTED) and the step of each change;
  - the evidence for V1 and V2.
- **Claim record:** see 12.5.
- **Step log:**
  - the observation received and the check result;
  - the plan or fallback choice, the action or safe stop, and the reason;
  - model evaluations used (total, and for verification);
  - counts of free computations (10.7).

#### 10.7 Budget and free computations — [PROPOSAL] *(strengthened in v0.5)*
**Model evaluations (budgeted).**
- One model evaluation is one call of `WorldModel::predict` (Brief 11.5.3).
- **Any operation that generates or evaluates a successor state** is a model evaluation and must go through the counter, whatever code performs it. A successor state is a state reached from a (state, action) pair, including a partial state such as agent and object positions.
- **Every state the planner or any check examines must have been produced by a budgeted model evaluation.** The only exceptions are:
  - the reasoner's current belief;
  - recorded observations;
  - the hypothetical **input** states built for the V2 test of a trap correction (11.5). Their predicted results are budgeted like any others.
- **Budget per step:**
  - Each reasoner has a budget per decision step: B in a one-reasoner condition, split by a fixed rule in a two-reasoner condition (Phase C).
  - Unused budget is lost at the end of the step and is not carried over.
- **Reuse.** A model evaluation already made in the same decision step, from the same belief, may be reused at no cost. C0's stored plan states are covered in 11.8.

**Free computations (not budgeted, but counted and reported).** These are the only free computations allowed. Each has a fixed, bounded scope, and none may create a successor state.

| Computation | Allowed scope | Bound | Counted as |
|---|---|---|---|
| **Comparisons** | Footprint comparison, `compareStates`, comparison of records | One comparison per pair of states compared | comparisons |
| **Evidence lookups** | Reading cells or positions in the current observation or history, **only** during an investigation (11.4) | One lookup per element per recorded observation | lookups |
| **Trap analysis** (11.2) | The relaxed reverse-push analysis over **object cells only** (100 cells): it never builds agent–object states. It gives each cell's dead-square status and relaxed push distance. | Run only when the believed walls change. At most 100 cells × 4 directions per pass, repeated until nothing changes (at most 100 passes). | analysis runs and cell operations |
| **Dead-square local check** (11.4 B) | Re-checks the analysis rule at **one** cell, from the believed walls and the stored statuses of its four neighbours. | Only while investigating a consequence surprise; one cell, 4 directions | local checks |
| **Heuristic** | A fixed formula combining **table lookups** from the latest trap analysis with straight-line (Manhattan) distances. No search and no loops over actions. | Called only on states already produced by a budgeted evaluation, so calls ≤ model evaluations | heuristic calls |

**Equality across conditions:**
- Every reasoner in every condition that uses a trap analysis or heuristic uses **exactly these definitions** (fixed in `STAGE_03.md`), with the same bounds.
- Phase C may **not** add any other kind of free computation. So a two-reasoner condition gets no extra uncharged reasoning capacity: each of its reasoners has the same bounded free tools as C1, and both spend from the same total budget.
- The counts in the table are reported per reasoner and per condition, so any difference in free computation is visible (Brief 11.5.3).

---

### 11. Baseline behaviour (C0 and C1)

#### 11.1 Shared rule for every condition: checks are triggered by use — [PROPOSAL]
This rule applies to every reasoner in C0–C4. Phase C must apply it unchanged to C2–C4.
1. **What is checked.**
   - A reasoner checks its beliefs **through its predictions**: after acting, it compares its prediction with the observation **on the footprint** (10.5).
   - In two-reasoner conditions, predictions are also compared with each other on the same footprint (Phase C).
   - No reasoner continuously audits its whole belief against the whole observation.
2. **Evidence during an investigation.**
   - When a mismatch, or in two-reasoner conditions a disagreement, is being investigated, the reasoner **may consult any part** of the current observation and of its history as evidence (Brief 4.2, step 4a: "Is there evidence (history, current observation) that settles it?").
3. **No information is withheld.**
   - Every observation is delivered in full to every reasoner (10.2).
   - This rule governs **when** a reasoner looks at the parts of an observation that no action could have changed. It does not limit **what** it receives.

Section 12.1 explains why this rule is needed and why it is fair.

#### 11.2 C1's components — [PROPOSAL]
- **Action model:** `WorldModel` with the true rules (`applyAction`) unless faulted.
- **Trap analysis (consequence check)** *(made precise in v0.6)*:
  - **Definition.** Let *Free* be the set of cells the reasoner believes are not walls, and let *dest* be the destination.
    - A **relaxed push** of the object from cell *q* in direction *d* is possible when both the cell behind it, *q* − *d*, and the cell in front, *q* + *d*, are in *Free*.
    - "Relaxed" means the agent is assumed able to stand at *q* − *d* whenever needed. Whether the agent can actually walk there is ignored.
    - Let *R* be the smallest set that contains *dest* and also contains any cell *q* that has a possible relaxed push into a cell already in *R*.
    - A **dead square** is a cell in *Free* that is not in *R*.
  - **What it guarantees.** If *p* is a dead square, **every** full state with the object at *p* is unsolvable. The relaxation only adds possibilities, so if the object cannot reach the destination even when the agent may stand anywhere, it cannot reach it at all.
  - **What it does not guarantee.** A full state can be unsolvable even though the object's cell is **not** a dead square, for example because the agent can never reach the side it would need to push from. Such **full-state dead ends** are not detected by the trap analysis. They are avoided only through budgeted search, and they are **not** F4 faults.
  - **How C1 uses it.** In the rest of this protocol, a "trap" means a dead square, and `trap(p)` means "*p* is a dead square".
  - It is computed from the believed walls by a fixed reverse-push analysis, defined exactly in `STAGE_03.md`.
  - It is recomputed whenever the believed walls change.
  - The experimenter computes the **true** dead squares of each layout with the same analysis on the true walls and destination.
- **Heuristic:** a fixed estimate of the remaining effort, computed from the belief without model evaluations. It counts as unreachable when the object is on a believed dead square. Its formula is fixed in `STAGE_03.md`.
- **Planner:** a deterministic best-first search over believed states.
  - It expands successors only through `WorldModel::predict`, which charges the budget.
  - It never expands a state whose object is on a believed dead square.
  - Ties are broken in the fixed action order North, East, South, West.

#### 11.3 C1's decision step — [PROPOSAL]
Each decision step *t* runs in this order:
1. **Receive** observation *o_t* and add it to the history.
2. **Check** the prediction committed at step *t*−1, if any, by a footprint comparison with *o_t*.
   - If it **matches**, apply the footprint update (10.3).
   - If it **does not match**, create a mismatch record and go to step 3. The footprint is then updated only as described in 11.4 A.5.
   - In both cases, record the result of any pending V2 test (11.5).
3. **Investigate** any new mismatch (11.4). If the budget does not allow the investigation to finish, it continues at the next step, and C1 is in **cautious mode** until it finishes.
4. **Fault activation point.** This is where Stage 4 applies the faults scheduled for step *t* (12.3). Nothing happens here in Stage 3.
5. **Plan.**
   - Keep the current plan if its next action's predicted result, from the current belief, equals the result the plan expects.
   - Otherwise, search anew with the remaining budget, keeping back one evaluation for step 7.
   - If the search reaches the goal, that path becomes the plan.
   - If not, use the **fallback**: the first action of the best frontier state found by the heuristic.
6. **Safety.** Do not choose an action whose predicted result puts the object on a believed dead square. If no acceptable action remains, take a **safe stop** (11.6).
7. **Commit** the prediction for the chosen action. If a correction is awaiting V2 and this prediction differs under the old and the corrected value, mark it as that correction's V2 test.
8. **Act** (one world action) or **safe stop**, and write the step log.

The run ends with:
- **Success**, when the observed object is on the destination;
- **Stopped — unresolved**, when the safe-stop limit is reached;
- **Step limit**, when the maximum number of world steps is reached.

The limits are set in Phase C.

#### 11.4 Investigating a mismatch, and source labels — [PROPOSAL] *(revised in v0.5)*
**Source labels in Experiment 1** *(v0.5)*:
- **S1 (reasoning error)** means the belief about the world was right, but what the reasoner derived from it was wrong. This **explicitly includes a corrupted internal action model or consequence analysis.**
- **S3** is reserved for a mismatch between the true environment rules or execution and the rules every reasoner was given. It should not occur in Experiment 1; if it does, it signals a bug (Brief 4.4).

**A. Mismatch between prediction and observation:**
1. **Which beliefs produced the prediction?** Compare C1's believed **pre-action** footprint contents with the recorded observation *o_t−1*. This is an evidence lookup.
2. **If they differ,** each differing element (`wall(c)` for a footprint cell, `object`) is a **belief candidate**.
   - Its corrected value is the value in *o_t−1*, the observation of the same moment.
   - Suspected source: **S2** (belief error).
3. **If they agree,** the belief was right and the prediction from it was wrong. The candidate is `model(a)`, corrected to the behaviour shown by the transition *o_t−1* → *o_t*.
   - Suspected source: **S1**.
4. Each candidate goes through **V1** (11.5).
   - **Exactly one passes:** it becomes **CONSISTENT** and is applied, through a correction record.
   - **None passes:** the suspected source is **S5**, and the mismatch is flagged as an anomaly to investigate as a possible bug.
   - **More than one passes:** the suspected source is S5. No correction is applied, and C1 is in cautious mode until V2 separates them.
5. **Updating the footprint afterwards:**
   - Footprint elements covered by an applied correction are updated by that correction.
   - Any remaining footprint difference is brought into line with *o_t* by an **unexplained update** (10.3), which is logged and flagged and never counted as a correction.

**B. Consequence surprise and consequence mismatch (how F4 can be noticed)** *(made precise in v0.6)*:
1. **Consequence surprise (the trigger).**
   - **When it occurs.** A search from a believed state ends with **no states left to explore** and the goal has not been reached, while the object's cell *p* in that state is classified **not** dead.
   - **What completes the search.** Only budgeted model evaluations, possibly over several steps while C1 is in cautious mode or a safe stop.
   - **What it does not show.** A surprise alone is **not** evidence of a fault, because the state may be a full-state dead end (11.2).
2. **Investigation: the dead-square local check** (free, 10.7). By the definition in 11.2, "*p* is not dead" holds exactly when either:
   - *p* is the destination; or
   - some direction *d* has *p* − *d* and *p* + *d* in *Free*, and *p* + *d* is classified not dead.
3. **Consequence mismatch (the contradiction).**
   - **If no such direction exists**, the stored status of *p* contradicts the analysis rule. C1 records a **consequence mismatch**, with the candidate `trap(p)` corrected to "dead" and suspected source **S1**.
   - **If such a direction exists**, the classification is consistent. The state is logged as a **full-state dead end**, and **no claim** is made.

**C. Labels C1 never uses.** C1 never labels a mismatch S3 or S4 in Experiment 1.

#### 11.5 Correction verification — [PROPOSAL] (Brief 4.5) *(V1 made time-aware in v0.5)*
**V1: the corrected belief must explain the recorded observations, each at its own time.** Values that change over time are never required to stay constant across history.

| Element | V1 check | Cost |
|---|---|---|
| `wall(c)` | Walls never change (rule R8), so the corrected value must equal *c*'s wall status in **every** recorded observation. Re-predicting the mismatched step with the corrected belief must also reproduce *o_t*. | Free lookups, plus 1 evaluation |
| `object` | The corrected value refers to step *t*−1. It must equal the object's position in *o_t−1*. Re-predicting the mismatched step from the corrected belief must reproduce *o_t*. Earlier positions are not compared with it, because the object moves over time. | Free lookups, plus 1 evaluation |
| `model(a)` | The corrected model, applied to **each** recorded pre-action observation *o_k−1* in which action *a* was taken, must reproduce the recorded *o_k* for that same transition. | One evaluation per such transition, possibly spread over several steps |
| `trap(p)` (the claim "*p* is a dead square") | The corrected status must satisfy the analysis rule at *p*: *p* is not the destination, and no direction *d* has *p* − *d* and *p* + *d* in *Free* with *p* + *d* classified not dead. No recorded observation may show the object moving from *p* to the destination. | Free local check and lookups |

**V2: an independent test.**
- **Walls, object position and action models** use Brief 4.5 option (a), a **future** observation. This is a later committed prediction whose result differs under the old and the corrected value. If it matches the observation, the correction becomes **VERIFIED**; if not, **REJECTED**.
- **Trap classifications** *(revised in v0.6)* use a **model-based local test** of the same claim, "*p* is a dead square". The test uses the action model, not the trap analysis's own wall reasoning.
  - For each direction *d* whose cell *p* − *d* is believed free, C1 builds the hypothetical state "agent at *p* − *d*, object at *p*" and predicts the push in direction *d*. Each prediction costs 1 evaluation, so at most 4 in total.
  - The correction becomes **VERIFIED** if no predicted push moves the object, or every push that moves it ends on a cell classified dead.
  - It is **REJECTED** if some predicted push moves the object to a cell classified not dead.
  - This test is about the **relaxed** claim only, so the agent's real reachability is deliberately not tested. It is not a claim about full-state solvability.
- **Active test.** If a correction is still CONSISTENT after *T_v* decision steps (Phase C) with no discriminating prediction, C1 may choose a **safe discriminating action** as a test.
  - An action is a safe discriminating action if it is predicted safe under both values and the two values predict different results.
  - If there are several, C1 chooses the one with the shortest walk, with ties broken in the action order.
- **Options not available.** Option (b), held-out observations, is not used. Option (c), a cross-check by the other agent, does not exist for a single reasoner.

**Status rules.**
- A **CONSISTENT** correction is used, marked provisional.
- A correction that later fails a prediction is **REJECTED**: the old value is restored, or marked unknown if the old value is also known to be wrong.
- Every status change is logged.
- CONSISTENT and VERIFIED corrections are reported separately (Brief 4.5).

#### 11.6 Safe stops and cautious mode — [PROPOSAL]
- **Cautious mode.** C1 enters cautious mode while an investigation is unfinished or several candidates remain. It then chooses only actions whose predicted results agree under **every** remaining candidate and are safe under every one.
- **Safe stop.** C1 takes a **safe stop** when:
  - there is no such action in cautious mode; or
  - no safe action exists at all (11.3, step 6).

  The safe stop is logged with the unresolved question and the risk avoided. It consumes that step's budget (Brief 11.5.3), which may be used to continue investigating.
- **Limit.** A safe-stop limit S_max (Phase C) bounds the number of safe stops. When it is reached, the run ends "Stopped — unresolved".

#### 11.7 Order of budget use within a step — [PROPOSAL]
1. Reserve one evaluation for the committed prediction (step 7).
2. Then spend the rest in this order:
   1. unfinished V1 checks;
   2. checks needed to choose a cautious action;
   3. planning;
   4. extra search in fallback mode.

Phase C must set B large enough that every condition can always afford the reserve. That is a pilot check (Section 9.2), not tuning.

#### 11.8 C0, the naive single agent — [PROPOSAL] *(planning clarified in v0.5)*
C0 uses C1's planner, heuristic, trap analysis and budget. It **never reads an observation after the first**. It makes no checks, investigations, corrections, claims or safe stops.

**Initial planning.**
- At decision step 1, C0 runs one budget-limited search from its initial belief.
- If the search reaches the goal, the path is C0's plan. If not, C0 takes the fallback action (11.3, step 5).

**Executing a plan.**
- While its plan has actions left, C0 takes the next one **without any new search**.
- It sets its belief to the state the plan predicted for that action, which was stored when the plan was made. This reuse costs nothing, because C0's belief is, by definition, that stored state.

**Continuation, after the plan is used up.**
- When C0 has no remaining plan actions and its belief has not reached the goal, it runs a new budget-limited search from its **own** belief. This happens after a fallback action, or when a plan has run out.
- Continuation is **never** triggered by anything observed. C0 never revises a plan in the light of observations, which is what "plans once, never checks predictions" means here.

**Effect.**
- Without faults, C0's predictions are exact, so it behaves like C1.
- With faults, its belief drifts from the world.
- Its detection measures are zero **by definition**. C0 is used only for H5 (task success).

---

### 12. Fault application and event definitions

#### 12.1 The STAGE_02 Section 5.6 problem and how this design resolves it — [PROPOSAL]
**The problem.** The world is fully observed. Suppose every reasoner compared its whole belief, or whole predicted states, with the whole observation at every step. Then any wrong belief about something visible, such as a wall (F1) or the object's position (F2, F5), would be revealed at once, by every condition. That includes C1, and it would happen whether or not the belief ever mattered. The result would be a ceiling effect that says nothing about catching errors in reasoning.

**The resolution** (rule 11.1, footprint 10.5).
- Every reasoner reads exactly what the world can change: the footprint.
- It does not continuously re-verify the parts of its belief that no action could have changed.
- A wrong belief about those parts is, by construction, an error that came from the reasoner and not from the world, which is exactly what Experiment 1 studies (Brief 11.1).
- Such an error becomes visible when it is **used**: when it affects a prediction's footprint, and that prediction is checked.

**Why this is fair to C1.**
1. **Full information.** Every observation is delivered in full to every reasoner, and the whole of it is available as evidence once a question arises (11.1, 11.4).
2. **No loss without faults.** Without faults, footprint updating keeps the belief exactly correct (10.5). The rule costs a reasoner nothing in a world where it holds no false beliefs.
3. **Equal rules.** The same rule applies to every reasoner in every condition, including the agents of C2–C4, whose predictions are compared with each other on the same footprint (12.8).
4. **Matches the brief.** It follows the brief's own reasoning cycle (Brief 4.2): checking is done by comparing predictions with observations, and investigation then consults the evidence.

**What it changes, and what it does not.**
- It does not change any Phase A definition.
- It makes F1, F2 and F5 faults **detectable through use**, like F3 and F4.
- It narrows what Experiment 1's results can claim. They apply to reasoners that check their beliefs through predictions. A reasoner that audited every belief against every observation would find F1, F2 and F5 faults immediately. This is stated as a limitation (12.7).

#### 12.2 How each fault type is applied — [PROPOSAL]

**Details fixed when the fault-event list is generated** (the same for every condition):

| Type | Element | Details |
|---|---|---|
| **F1** wrong wall | `wall(c)` | An interior cell *c*, other than the destination. The split between free→blocked and blocked→free events is set in Phase C. |
| **F2** wrong object position | `object` | A seeded order of the four directions. |
| **F3** faulty action model | `model(a)` | An action *a* and a rotation (left or right). |
| **F4** faulty consequence reasoning | `trap(p)` | A **true dead square** *p* of the layout (11.2, computed from the true walls and destination). |
| **F5** stale memory | `object` | None. |

**When an event is not applicable:**
- **F1:** a free→blocked flip would put a wall under the agent or object the reasoner believes is there.
- **F2:** no direction passes the test in the effect below.
- **F3:** always applicable.
- **F4:** the layout has no dead squares.
- **F5:** the object has not yet moved in this run.

**Effect when activated:**
- **F1:** the reasoner's belief about *c* is flipped.
- **F2:** the believed object moves one cell in the first direction (in the seeded order) whose cell is inside the grid, believed free of walls, and not the believed agent cell.
- **F3:** in this reasoner's action model, when *a* would push, the object is predicted to move in the rotated direction. If that cell is believed blocked, the prediction is Blocked. The fault affects **all** of this reasoner's model evaluations, including planning.
- **F4:** in **every** run of this reasoner's trap analysis, **after the analysis has finished**, the stored status of *p* is overwritten with "not dead". Every other cell's status is computed normally from the true rule, before the overwrite, so it does not depend on the faulty status of *p*. The walls and the local check of 11.4 B are unaffected. F4 always goes in this one direction: it fails to recognise a dead square. **True value for scoring:** "dead".
- **F5:** the believed object position goes back to where it was before its most recent move.

**Ground-truth source label:**
- **S2** for F1, F2 and F5.
- **S1** for F3 and F4.

**Why F3 counts as S1** *(settled in v0.5, following review)*: in Experiment 1, S1 explicitly includes a corrupted internal action model or consequence analysis, and S3 is reserved for a mismatch in the actual environment rules or execution (11.4). The brief says S3 should not occur in Experiment 1 (4.4).

**Other rules:**
- **Common-mode events** apply the same details to both reasoners. F2 and F5 are applied to each reasoner's own belief.
- **Reasoners without an element.** If a fault type has no meaning for a reasoner in Phase C (for example, a reasoner without a trap analysis), this must be stated there, and results are reported per type (Brief 11.5.1).

#### 12.3 Activation, persistence, and end of a fault (the fault lifecycle) — [PROPOSAL] *(rewritten in v0.5)*
**Activation.**
- An event is **activated** in a reasoner when both are true:
  - the run reaches its scheduled decision step;
  - the event is applicable there.
- It is applied at the fault activation point (11.3, step 4): after the previous prediction has been checked and any investigation of it has run, and before planning.
- A common-mode event counts as **one** event. It is activated if it is applied to at least one reasoner.
- An event whose step is not reached, or that is not applicable, is **not activated** (Phase A 4.3). The two reasons are counted separately.

**Two kinds of fault.**

| Kind | Types | What activation does | When the fault is active |
|---|---|---|---|
| **Value fault** | F1, F2, F5 | Corrupts one stored belief value once. No ongoing cause remains. | While the element's believed value differs from its true value at that moment. For `object`, the true value is the object's current position. |
| **Process fault** | F3, F4 | Installs an ongoing cause in the action model `model(a)` or the trap analysis at *p*. The cause is applied on **every** use, including every recomputation of the trap analysis. | Until a correction **replaces** `model(a)` with the true behaviour, or reclassifies `trap(p)` truly. No belief update, footprint update, or recomputation can end it. |

**End states recorded by the scorer** (exactly one at the end of the run, with its history):
1. **Corrected.** A **correction record** for this reasoner and element set it to the **true** value or behaviour. Its final status is recorded: CONSISTENT or VERIFIED.
2. **Overwritten without correction.** The element became true by any other route, such as an unexplained update. This applies to value faults only. The fault ends, but this event is **not** counted as detected, corrected or verified.
3. **Active at end of run.** The fault was still active when the run ended.

**Further rules:**
- If a correction is later **REJECTED** and the faulty value or behaviour is restored, the **same** event becomes active again. It is not a new activation.
- A wrong-valued correction leaves the fault active. It counts under M6.
- **Credit only through records.** Detection is credited **only** through claims (12.5), and correction and verification **only** through correction records. A restored true value never earns detection, correction or verification credit.

#### 12.4 Shadow evaluation (scoring only) — [PROPOSAL]
- **What it is.** For every model evaluation, heuristic computation and trap analysis performed by a reasoner while it holds an active fault, the scoring code repeats the computation with the faulted element at its true value.
- **Separation.**
  - These shadow computations never use any reasoner's budget.
  - They are never visible to any reasoner.
  - They live in the scoring code, which is separate from the agents (Brief 11.5.2).

#### 12.5 Event definitions — [PROPOSAL]
1. **Decision step.** One pass through the cycle that ends in exactly one world action or one safe stop. A **world step** is a decision step that ends in a world action. Safe-stop steps are decision steps (Phase A 5.4).
2. **Effective use** of an active fault. A computation in which the fault changes the result, compared with its shadow:
   - for a model evaluation, the predicted outcome or footprint contents differ;
   - for the heuristic or trap analysis, the value differs.
3. **Detection opportunity** (Phase A 4.3). At least one effective use before the fault ends or the run ends.
   - **Revealing opportunity:** an effective use in a **committed prediction that was then checked**.
   - Both counts are reported.
4. **Mismatch.** A recorded footprint difference between a committed prediction and the observation, or, in Phase C, between two reasoners' predictions. A mismatch is **caused by** every active fault that had an effective use in the compared prediction(s).
5. **Mismatch detection** of event *e* (Phase A 5.2). A mismatch caused by *e* is recorded while *e* is active.
6. **Claim that an error exists.** One of:
   - a correction recorded as CONSISTENT or VERIFIED;
   - in Phase C, an OBJECTION or SAFE_STOP that names a specific reasoner and element.

   Candidates rejected inside V1 are **not** claims.
7. **Correctly identified.** The claim names the same reasoner and the same element as event *e*. The corrected value is judged separately, by M4 and M6.
8. **Reasoner attribution and strict detection** (Phase A 5.2). The first claim made while *e* is active that names *e*'s reasoner, and *e*'s reasoner and element, respectively. For common-mode events, the rules in Phase A 5.2 apply. *(v0.5)* An element that became true without a claim ("overwritten without correction", 12.3) is **never** counted as detected.
9. **False alarm** (M3). A claim whose reasoner and element held **no** active fault at the time of the claim.
10. **Verified correction** (M4). A correction for *e*'s reasoner and element, with the **true** value, that reached VERIFIED while *e* was active.
11. **Wrong accepted correction** (M6). A correction that reached CONSISTENT or VERIFIED with a value that is not the true value.
12. **Justified safe stop** (M7). A safe stop at a decision step when at least one activated fault was active in any reasoner of the system.
13. **Source-label accuracy** (M8). A mismatch record's suspected source is compared with the ground-truth sources of the faults that caused it (12.2).
    - A mismatch caused by **no** fault cannot happen in an exact world unless there is a bug, so each one is investigated as a possible bug (Phase A 8.4).
    - S5 is scored as "not identified", never as correct.

#### 12.6 Denominators — [PROPOSAL] (clarifying Phase A, not changing it)
- **Primary denominators.** Detection opportunity depends on each condition's own behaviour. So it is **not** used to remove events from the primary denominators. The primary M2 and M4 denominators remain the common-exposure set of **activated** events (Phase A 4.4).
- **Why.** A condition must not be able to shrink its denominator by using its beliefs less (Phase A 4.3: "cannot improve its detection score by stopping early or acting less").
- **Secondary analysis.** A pre-specified secondary analysis reports M2 and M4 over events with a **revealing opportunity in both** conditions of a comparison. It is clearly labelled, and it is never presented alone.

#### 12.7 Known limitations of this design — [PROPOSAL]
1. **The checks-through-prediction model.** Results apply to reasoners that check beliefs through predictions (12.1), **not to every possible single-agent architecture**. A reasoner that continuously audited every belief against the whole observation would find F1, F2 and F5 faults at once. Every report of Experiment 1 must state this limitation next to the primary conclusion.
2. **Noticing is the hard part.** Once a mismatch is noticed, the exact history makes it easy to find the faulty belief element. Differences between conditions are therefore expected mainly in **noticing** (when, and whether, a fault is used and checked), in timing (M5), and in false alarms. Strict identification itself is expected to be easy.
3. **Faults a single reasoner cannot find.** A single reasoner that wrongly believes a free cell is blocked (F1, free→blocked) will usually plan around it, and never test it. Such faults are often undetectable without a second reasoner. This is reported per fault type and direction, and through the secondary analysis (12.6).
4. **F4 is hard for every condition.** It can be noticed only after an exhaustive search ends without a solution (11.4 B), which a per-step budget rarely allows unless the object is already stuck. Expected detection rates for F4 are low for every condition. F4 concerns only **dead squares** in the relaxed sense of 11.2. Full-state dead ends that are not dead squares are outside F4: they are never counted as F4 faults, detections or misses.

#### 12.8 Requirements Phase C must meet — [PROPOSAL]
1. **Shared rules for C2–C4.** C2–C4 use the rules of 11.1, the footprint, the belief maintenance of 10.3, the verification of 11.5, the safe stops of 11.6, the fault lifecycle of 12.3, and the event definitions of 12.5 without change. They use only the free computations in 10.7, with the same bounds.
2. **Comparing predictions between reasoners.** Two reasoners' predictions are compared **on the footprint only**, so that a partner's prediction does not act as a whole-belief audit. Beliefs are revealed to the other reasoner only through messages during an investigation (Brief 4.2, step 4a).
3. **Numbers.** Phase C sets the remaining numbers, with stated reasons, before any pilot:
   - B and how it is split;
   - T_max (the maximum number of world steps);
   - S_max (the safe-stop limit);
   - T_v (the steps before an active test);
   - the fault rate and the mix of fault types, including the F1 direction split;
   - the common-mode share;
   - layout requirements: some layouts must contain dead squares (Brief 11.2).

#### 12.9 Tests the Stage 3 plan must include — [PROPOSAL] *(added in v0.5)*
`STAGE_03.md` must include correctness tests for at least the following, using **valid** test worlds (Stage 1 lesson T10). Beliefs may be made wrong by hand in tests; Stage 3 has no fault injection.

1. **Footprint invariant.** Without faults, after any sequence of actions, the footprint-updated belief equals the true state.
2. **Footprint selection.** Each row of the disagreement table in 10.5 produces exactly the listed cells and reported differences.
3. **Matching update.** A footprint update after a matching check never changes a belief value that was in the comparison.
4. **Investigation:**
   - a wrong wall at c1;
   - a wrong object position (believed at c1 when it is not, and not believed at c1 when it is);
   - a wrong action model.

   Each case gives the correct candidate, the correct corrected value and the correct suspected source (S2 or S1).
5. **Time-aware V1:**
   - A true object correction passes V1 even though the object was elsewhere in earlier observations.
   - A wrong object correction fails V1.
   - A wall correction is checked against every recorded observation.
   - An action-model correction is checked against each recorded transition with its own pre-state.
6. **V2 and active tests.** A discriminating future prediction makes a correction VERIFIED or REJECTED, and an active test is chosen only when safe.
7. **Unexplained update.** When no candidate passes V1, the update is logged as unexplained and no correction record is created.
8. **Budget:**
   - No state is examined without a budgeted evaluation.
   - Unused budget is not carried over.
   - The reserve for the committed prediction is always kept.
   - Heuristic calls never exceed model evaluations.
   - The trap analysis runs only when the believed walls change.
11. **Dead squares versus full-state dead ends** *(v0.6)*:
    - In a small layout, the analysis marks a relaxed dead square, such as a corner that is not the destination.
    - In a layout with a full-state dead end whose object cell is **not** a dead square (for example, the agent cannot reach the pushing side), the analysis marks the cell "not dead", an exhaustive search finds no solution, the local check holds, and the result is logged as a full-state dead end with **no claim**.
12. **F4 correction claims** *(v0.6)*:
    - **Valid claim.** The status of a true dead square is overwritten by hand to "not dead", with the object on it. The search runs out of states, the local check fails, and the claim `trap(p)` = dead is made (S1). It passes V1, and the model-based V2 test makes it VERIFIED.
    - **Invalid claim.** A proposed "dead" status for a cell that is not a dead square fails V1, or is REJECTED by V2.
9. **Safe stops.** A safe stop happens when no safe or cautious action exists. It counts as a decision step, and it consumes that step's budget.
10. **C0.** C0 never reads observations after the first, and plans again only when its plan is used up.

## Phase C sections (placeholders; to be written during Stage 3)

### 13. Two-agent behaviour (C2–C4)
Built on the shared definitions in Phase B, and meeting the requirements in 12.8.

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
| Draft v0.4 | 2026-10-19 | **Phase B (Sections 10–12) drafted** by Claude after Stage 2 was completed and tagged (`e112145`), for independent review by ChatGPT and approval by the owner. **Section 10** defines representations matching the Stage 2 code: observations (complete, identical for every reasoner, exact history), how a belief may change (never silently overwritten), belief elements for faults and claims, the action **footprint**, records, and budget/free-computation rules. **Section 11** defines C1's full decision step (check, investigate, plan, safety, commit, act or safe stop), investigation and source labels, V1/V2 correction verification with an active-test option, cautious mode and safe stops, budget order, and C0. **Section 12** resolves `STAGE_02.md` 5.6: every reasoner checks its beliefs through predictions compared **on the footprint** (all that an action can change) and may consult the whole observation and history once a mismatch is investigated; no information is withheld, and without faults beliefs stay exactly correct. It also defines how F1–F5 are applied, activation and persistence, scoring-only shadow evaluation, all event definitions required by Phase A, a clarification of denominators, limitations, and requirements for Phase C. Header and phase-status lines updated. **Sections 1–9 (Phase A) unchanged.** Not approved; not frozen. |
| Draft v0.5 | 2026-10-19 | ChatGPT reviewed v0.4, supported the overall Phase B design (footprint-based, prediction-triggered checking applied equally to all conditions, with full observations and history available during investigations), and requested targeted corrections. Only the affected Phase B sections were revised. **(1) Time-aware V1 (11.5):** each element is verified against observations at its own time: walls against every recorded observation (walls never change); a corrected object position against the observation of the same moment and the mismatched transition, never against earlier positions; action models against each recorded transition from its own pre-state; trap classifications by the exhaustive-search evidence, with V2 by an independent second search. **(2) Budget (10.7):** any operation that generates or evaluates a successor state (including partial states) is budgeted; every examined state must come from a budgeted evaluation; the only free computations are listed with fixed scopes and bounds (trap analysis over object cells only, run only when believed walls change; heuristic by table lookups, called only on budgeted states); all are counted and reported; Phase C may add no other free computation, so two-reasoner conditions gain no uncharged capacity. **(3) Fault lifecycle (10.3, 12.3, 12.5):** belief changes are matching footprint updates, corrections, logged *unexplained updates*, or injected faults; value faults (F1, F2, F5) and process faults (F3, F4) defined separately; the scorer records each event as corrected (with status), overwritten without correction, or active at end; detection, correction and verification are credited only through claims and correction records, never through a restored value. **(4) Deterministic footprint (10.5):** c2 inclusion rule completed (believed object at c1, observed Pushed, or observed object at c1); cells chosen before comparison from belief and observation only; the same rule for comparing two reasoners' predictions; a table of disagreement cases. **(5) C0 (11.8):** initial planning, plan execution without search, and continuation only when the plan is used up, never triggered by observation. **(6)** Limitation 12.7.1 strengthened (results do not apply to every possible single-agent architecture; stated beside the primary conclusion). S1 explicitly includes a corrupted internal action model or consequence analysis; S3 reserved for actual environment rules or execution (11.4, 12.2). Consequence mismatches (how F4 can be noticed) defined (11.4 B). New 12.9 lists the tests `STAGE_03.md` must include. **Sections 1–9 (Phase A) unchanged.** Not approved; not frozen. |
| Draft v0.6 | 2026-10-19 | ChatGPT reviewed v0.5, accepted all six v0.4 corrections, and requested one final conceptual correction about the trap analysis and F4. **(1) 11.2:** a trap is now defined precisely as a **dead square**, a cell outside the smallest set *R* that contains the destination and is closed under *relaxed* pushes (both the cell behind and the cell in front believed free, agent reachability ignored). It is stated that a dead square makes every full state with the object there unsolvable, but that full-state dead ends with the object on a non-dead square exist, are not detected by the analysis, and are not F4 faults. **(2) 11.4 B:** an exhaustive search ending without a solution is now only a **consequence surprise** (a trigger, not evidence). A **consequence mismatch** requires a contradiction of the relaxed claim itself: the free one-cell **dead-square local check** finds that the stored status "*p* not dead" violates the analysis rule. Otherwise the state is logged as a full-state dead end, and no claim is made. **(3) 11.5:** V1 and V2 for `trap(p)` now test the same claim, "*p* is a dead square": V1 is the local analysis-rule check; V2 is a model-based local test (at most 4 budgeted push predictions from hypothetical states "agent at *p* − *d*, object at *p*"), replacing the second full-state search. **(4) 12.2:** F4 overwrites the stored status of a true dead square *p* with "not dead" after each analysis run, leaving every other cell's status unaffected; true value for scoring: "dead". **(5) 12.9:** new tests distinguishing dead squares from full-state dead ends, and testing valid and invalid F4 claims. 10.7 adds the bounded local check to the free computations and permits budgeted predictions from the V2 hypothetical states; 12.7.4 restricts F4 to dead squares. Checked by brute force for this draft on 1,500 random 7×7 layouts: 0 of 366,770 full states with the object on a dead square were solvable; the local check rejected every corrupted dead-square status (19,306 cases); and 28,372 full-state dead ends on non-dead squares were found, confirming that the two notions differ. **Sections 1–9 (Phase A) unchanged.** Not approved; not frozen. |
| v0.6 — Phase B approved | 2026-10-19 | **Phase B (Sections 10–12) APPROVED** by ChatGPT's independent review and by the owner. ChatGPT confirmed that v0.6 correctly distinguishes dead squares (relaxed reverse-push analysis) from full-state dead ends; a consequence surprise (trigger only) from a consequence mismatch (a contradiction of the dead-square classification); and accepted the revised F4 definition, the local V1 check and the budgeted model-based V2 test. The earlier Phase B safeguards remain accepted: equal information, computational budgets, fault persistence, time-aware verification, and protection of the competent single-agent baseline. Only the status lines changed; the content of Sections 10–12 is identical to Draft v0.6. **Sections 1–9 (Phase A) unchanged.** **The complete protocol is not frozen**; Phase C remains. The `protocol-1-approved` tag is reserved for the complete protocol. |
