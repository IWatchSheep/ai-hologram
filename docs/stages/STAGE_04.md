# Stage 4 — Fault Injection and Measurement

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Stage** | 4 of the roadmap in *AI_Hologram_Project_Brief_v1.1.txt*, Section 10 |
| **Plan version** | 1.1 |
| **Date** | October 20, 2026 |
| **Status** | **ChatGPT's review of v1.0 (October 20, 2026): 4a and 4b APPROVED; 4c CONDITIONALLY APPROVED.** v1.1 adds what ChatGPT asked for: the instrumentation exception IE-1 (3.12), the status of the M4 clarification D-038, and the protection of primary scoring (3.9, 6.6). **APPROVED by the owner on October 20, 2026** ("yes I approve"), including IE-1 and D-038. Ready for implementation: Step 1 (the owner commits this plan), then **4a only**. 4c may begin after 4b (IE-1 is approved). **4d may not begin until plan v1.2 (exact scoring values) is reviewed and approved.** Approval of the plan does not mean any Stage 4 code is implemented or verified. |
| **Governing specification** | AI_Hologram_Project_Brief_v1.1.txt (approved) |
| **Governing protocol** | `docs/EXPERIMENT_1_PROTOCOL.md` **v1.0, frozen** (tag `protocol-1-approved`, commit `e7b4462`). This plan implements Phase A 4.2–4.4 and 5, Phase B 12.2–12.6, Section 13.8 (fault targets), Section 13.12 (A3, A4 and the reporting table) and the numbers in Section 14, **for C0 and C1**. **The protocol cannot change. Where this plan and the protocol disagree, the protocol wins and the owner must be told.** Where the protocol leaves an implementation detail open, this plan fixes it as a **reading** (marked [READING]), lists it for review (Section 14), and records it in `DECISIONS.md`. A reading may fill a gap; it may never change a protocol rule. |
| **Builds on** | Stage 3, complete and tagged `stage-3` (commit `7587f38`). C1 is frozen there. |

---

## 1. Purpose of Stage 4

From the brief (Section 10, Stage 4):
> Goal: the ability to introduce controlled, logged errors into an agent's beliefs or reasoning, and to measure detection and recovery. Contents: fault types (Section 11.3), a fault schedule controlled by seed, metrics collection, CSV output. Learning focus: reproducible experiments.

Stage 4 builds the experiment's **measuring instrument**. In plain terms:
1. **Material.** It generates the layouts and the fault-event lists from fixed seeds, so that anyone can regenerate exactly the same experiment.
2. **Faults.** It applies each scheduled fault to a reasoner at the fault activation point, exactly as Phase B 12.2–12.3 and amendments A3 and A4 say, and records what happened to every event.
3. **Measurement.** It scores each run from the agent's own records, using the event definitions of Phase B 12.5, without the agent knowing. The scoring code is separate from the agents (Brief 11.5.2).
4. **Output.** It writes one row per run and one row per fault event in CSV format.

Stage 4 does all of this for **C0 and C1**, the single-reasoner conditions. The two-reasoner conditions (C2–C4) are built in Stage 6. Stage 4 already generates the **targets** (R1, R2 or both) that Stage 6 will need, because the fault-event list is defined per system and is identical for every condition (Phase A 4.2, Section 13.8).

**What Stage 4 does not do:**
- define messages (Stage 5) or build Agent 1 and Agent 2 (Stage 6);
- run the pilot or the confirmatory experiment, or build the experiment runner (Stages 7–8);
- compute any comparison between conditions: common exposure, bootstrap intervals and verdicts belong to the analysis (Stage 8);
- run any agent on a pilot or confirmatory layout (Section 2).

---

## 2. What Stage 4 must NOT contain

**Excluded features:**
- No message protocol, second reasoner, or two-agent code.
- No experiment runner, no batch runs over the pilot or confirmatory sets, and no comparison statistics.
- No change to C1's or C0's decisions. The only changes to Stage 3 code are the four **observation-only** additions in 3.12, each tested to change nothing when no fault is scheduled.

**Material rules:**
- **No agent is run on a pilot layout (seeds 900000–900039) or a confirmatory layout (seeds 100000–100399) in Stage 4.** Tests and demonstrations use hand-built worlds and the **demonstration seeds 500000–500009**, a range reserved here for that purpose. Generating the pilot and confirmatory *material* (layouts and event lists) is allowed, so its balance can be tested; nothing is run on it.

**Code rules:**
- No exceptions, no assertions that stop the program, no inheritance (D-010).
- No changes to Stage 1 rules or Stage 2 types, or to any existing test's expected values.
- No external libraries, threads, networking or GUI. Randomness comes only from the project's own generator (3.1), never from `<random>` or `rand()`.

If anything here seems necessary, **stop and ask the owner**.

---

## 3. Concepts and exact behaviour

### 3.1 Seeded random numbers [Brief Stage 4: "a fault schedule controlled by seed"]

**Why the project has its own generator.** The C++ standard library fixes the Mersenne Twister engine exactly, but **not** its distributions (`std::uniform_int_distribution` may give different numbers with different compilers). An experiment must give the same material on any machine, so the project uses one small, fully specified generator. [READING] The protocol fixes the seed ranges (Section 14) but not the algorithm; this plan fixes it (D-030).

**`SeededRandom`** is **SplitMix64** (a well-known, published 64-bit generator). Its state is one `std::uint64_t`, set from the seed. All arithmetic wraps modulo 2⁶⁴.

```
next():
    state = state + 0x9E3779B97F4A7C15
    z = state
    z = (z XOR (z >> 30)) * 0xBF58476D1CE4E5B9
    z = (z XOR (z >> 27)) * 0x94D049BB133111EB
    return z XOR (z >> 31)

below(n), for n >= 1 (a whole number from 0 to n-1, every value equally likely):
    threshold = (2^64 - n) mod n          // in C++: (0 - n) % n with uint64_t
    repeat: r = next()  until r >= threshold
    return r mod n

shuffle(list):                             // Fisher-Yates, from the end
    for i = size-1 down to 1:  j = below(i+1); swap(list[i], list[j])

pickDistinct(list, k):                     // first k of a partial Fisher-Yates, from the front
    for i = 0 to k-1:  j = i + below(size-i); swap(list[i], list[j])
    return the first k elements, in that order
```
`below(1)` always returns 0 but still uses one `next()`.

**Exact values** (from the reference, `ref_stage4.py`):

| Call | Result |
|---|---|
| seed 0, three `next()` | 16294208416658607535, 7960286522194355700, 487617019471545679 |
| seed 1234567, three `next()` | 6457827717110365317, 3203168211198807973, 9817491932198370423 |
| seed 42, ten `below(10)` | 3, 1, 8, 4, 0, 2, 5, 8, 5, 4 |
| seed 42, `shuffle` of 1..10 | 1, 10, 6, 9, 7, 5, 8, 3, 2, 4 |
| seed 7, `pickDistinct(1..10, 3)` | 8, 1, 5 |
| seed 5, `below(1)` | 0, and the state afterwards is 11400714819323198490 |

The first row is the published SplitMix64 test value for seed 0.

### 3.2 The experimenter's search and the layout generator [Section 14, "Layouts"]

**The experimenter's search** (`shortestSolutionLength`). A breadth-first search over **true** states (agent position, object position), using the Stage 1 rules, trying actions in the order North, East, South, West. It returns the number of world steps in a shortest solution, or "none" if the destination cannot be reached. It is **experimenter code**: it lives in `src/experiment/`, and no agent ever calls it or sees its result. It is not charged to any budget, because no reasoner runs it.

**The layout generator** (`generateLayout(layoutSeed)`), with `SeededRandom random(layoutSeed)`:
```
for attempt = 1 to 10000:
    for y = 0..9, for x = 0..9 (row by row):
        border cell -> wall (no random number is used)
        interior cell -> wall if random.below(100) < 12
    free = all non-wall cells, row by row (y, then x)
    if fewer than 3 free cells: next attempt
    (agent, object, destination) = random.pickDistinct(free, 3)
    reject (next attempt) unless ALL of:
        - the world is valid (isValidWorld, Stage 1);
        - shortestSolutionLength is 12 to 40;
        - at least one interior cell other than the destination is a true dead square
          (computeDeadSquares on the true world, no overrides).
    accept: return the world, the attempt number and the shortest length
after 10000 attempts: report "no layout" (never happens for the seeds tested)
```
[READING] "About 12% interior walls (seeded)" is read as each interior cell independently a wall with probability 12/100 (D-031).

**Exact values:**

| Seed | Attempts | Shortest | Agent | Object | Destination | True dead cells | Interior walls |
|---|---|---|---|---|---|---|---|
| 500000 | 3 | 12 | (7, 2) | (6, 5) | (2, 6) | 25 | 7 |
| 500001 | 6 | 16 | (8, 1) | (4, 5) | (8, 4) | 23 | 9 |
| 500002 | 1 | 29 | (4, 1) | (4, 7) | (8, 5) | 27 | 8 |
| 500003 | 3 | 14 | (1, 1) | (4, 5) | (7, 4) | 26 | 7 |
| 900000 | 2 | 17 | (7, 1) | (7, 6) | (2, 8) | 19 | 7 |
| 900001 | 4 | 19 | (4, 3) | (3, 7) | (5, 6) | 26 | 11 |

Layouts 500004–500009: attempts 3, 1, 3, 8, 5, 2; shortest solutions 20, 16, 12, 12, 12, 16.

Layouts 500000 and 900000, rendered with `renderWorld`:
```
500000          900000
##########      ##########
#...#....#      #....#.A##
#.....#A.#      ##...#...#
##.......#      #........#
#.#......#      #.....#..#
#.....O..#      #.#......#
##D...#..#      #......O.#
#........#      #......#.#
#......#.#      #.D......#
##########      ##########
```
**Whole-set checks** (material only; no agent is run): all 40 pilot layouts are generated, at most 10 attempts each, shortest solutions 12–35; all 400 confirmatory layouts are generated, at most 20 attempts each, shortest solutions 12–38.

**Note for the record.** The validation pilots in `docs/protocol_validation/` used Python's own generator, so their layouts are **not** the layouts this generator makes from the same seeds. That is expected and harmless: those pilots validated the protocol's design (9.2), and the experiment's material is, from now on, defined only by this generator.

### 3.3 The fault-event list [Phase A 4.2, Phase B 12.2, Sections 13.8 and 14]

The list is generated **once for a whole experiment** (the pilot set, the confirmatory set, or the demonstration set), because the type mix, the common-mode share, the targets and the F1 directions are balanced over the whole experiment (Section 14, 13.8). It is **identical for every condition**.

**Input:** the ordered list of runs, each (layout seed, run seed, true layout), in the order layout seed ascending, then run seed ascending; and the **fault master seed 4242** [READING, D-032: the protocol does not fix this seed; 4242 is the value used in the validation pilots, fixed now, before any confirmatory run].

**Experiment-level draws**, with `SeededRandom experimentRandom(4242)`, in this exact order:
1. With *R* runs, there are *n* = 3*R* events, numbered *i* = 0 … *n*−1 (run *r* holds events 3*r*, 3*r*+1, 3*r*+2). The type list is F1, F2, F3, F4, F5, F1, F2, … (type of *i* = F(*i* mod 5 + 1)); it is shuffled once.
2. For each type, in the order F1, F2, F3, F4, F5:
   - `idx` = the event numbers of that type, ascending; `perm` = a copy, shuffled.
   - *k* = round-half-up(count ÷ 5) = (2·count + 5) div 10. The first *k* events of `perm` are **common-mode** (target "both").
   - The remaining events, in `perm` order, alternate **R1, R2, R1, …** (an odd count gives R1 the extra event).
   - For F1 only: `perm2` = another copy of `idx`, shuffled; its events alternate **free→blocked, blocked→free, …**.

**Per-run draws**, with `SeededRandom runRandom(runStreamSeed)`, where
`runStreamSeed = (4242 << 40) + (layoutSeed << 8) + runSeed` (unsigned 64-bit; distinct for every run in every range):
1. The three event steps: `pickDistinct(1..10, 3)`, then sorted ascending. Event *j* (1, 2, 3) of the run gets the *j*-th step.
2. Then, for *j* = 1, 2, 3 in order, the details of that event (12.2):
   - **F1:** candidates = interior cells (x and y from 1 to 8), row by row, other than the destination, whose **true** status is free (for free→blocked) or blocked (for blocked→free). Cell = `candidates[below(size)]`. If there are no candidates, the cell is "none" and no number is drawn.
   - **F2:** the direction order = `shuffle` of (North, East, South, West).
   - **F3:** action = (North, East, South, West)[`below(4)`], then rotation = (left, right)[`below(2)`].
   - **F4:** candidates = true dead squares among the interior cells, row by row, other than the destination. Cell = `candidates[below(size)]`, or "none" with no draw.
   - **F5:** nothing is drawn.

**Event identifier:** `<layoutSeed>-<runSeed>-<j>`, for example `900000-1-2`. It never changes, and every report keeps it with the original scheduled step (13.12).

**Rotation directions:** left turns counter-clockwise (North→West→South→East→North); right turns clockwise.

**Exact values: the demonstration experiment** (layouts 500000–500003, run seeds 1–2, so 8 runs and 24 events):

| Event | Step | Type | Target | Details |
|---|---|---|---|---|
| 500000-1-1 | 4 | F1 | both | (6, 8) free→blocked |
| 500000-1-2 | 6 | F3 | R1 | East, right |
| 500000-1-3 | 10 | F4 | R2 | (8, 2) |
| 500000-2-1 | 2 | F1 | R1 | (1, 6) blocked→free |
| 500000-2-2 | 9 | F2 | R2 | North South East West |
| 500000-2-3 | 10 | F4 | R2 | (7, 1) |
| 500001-1-1 | 4 | F2 | R1 | West North East South |
| 500001-1-2 | 6 | F2 | R2 | West East South North |
| 500001-1-3 | 8 | F5 | R2 | — |
| 500001-2-1 | 2 | F1 | R1 | (1, 3) free→blocked |
| 500001-2-2 | 4 | F2 | R1 | West East South North |
| 500001-2-3 | 10 | F4 | both | (7, 1) |
| 500002-1-1 | 3 | F3 | R1 | East, right |
| 500002-1-2 | 6 | F4 | R1 | (4, 1) |
| 500002-1-3 | 7 | F1 | R2 | (2, 8) blocked→free |
| 500002-2-1 | 5 | F3 | both | South, right |
| 500002-2-2 | 8 | F3 | R2 | West, left |
| 500002-2-3 | 9 | F5 | both | — |
| 500003-1-1 | 1 | F5 | R1 | — |
| 500003-1-2 | 2 | F4 | R1 | (1, 7) |
| 500003-1-3 | 8 | F1 | R2 | (6, 8) free→blocked |
| 500003-2-1 | 1 | F2 | both | East North South West |
| 500003-2-2 | 9 | F3 | R2 | North, right |
| 500003-2-3 | 10 | F5 | R1 | — |

**Exact values: the pilot list** (40 pilot layouts × run seeds 1–2; 240 events; material only):
- each type: 48 events, of which 10 common-mode, 19 R1, 19 R2;
- F1: 24 free→blocked, 24 blocked→free; no F1 or F4 event without a cell;
- run 900000-1: `900000-1-1` step 1 F4 both (4, 1); `900000-1-2` step 2 F5 both; `900000-1-3` step 8 F3 both West right.

**Exact values: the confirmatory list** (400 layouts × run seeds 1–5; 6,000 events; material only):
- each type: 1,200 events, of which 240 common-mode (20%), 480 R1, 480 R2;
- F1: 600 free→blocked, 600 blocked→free; no F1 or F4 event without a cell;
- events per step 1–10: 580, 620, 603, 578, 609, 577, 614, 597, 605, 617;
- the first run's events: `100000-1-1` step 2 F2 both (South West East North); `100000-1-2` step 7 F2 both (South North East West); `100000-1-3` step 8 F3 R2 South right.

These counts confirm Section 14 exactly: 20% of each type, 20% common-mode per type, exact halves for targets and F1 directions.

### 3.4 Applying a fault to a reasoner [Phase B 12.2, amendment A3]

A fault is applied by the **fault injector** (experimenter code) to one reasoner's state, at that reasoner's fault activation point (3.6). C0 and C1 receive every event, whatever its target (13.8). Applying a fault never touches the world, and never charges any budget.

| Type | Not applicable when | Effect when applied |
|---|---|---|
| **F1** wall(c) | free→blocked and c is the **believed** agent cell; or free→blocked and c is the **believed** object cell; or the event has no cell [READING, D-033] | The belief's wall status of c is flipped. |
| **F2** object | no direction in the event's order gives a cell that is inside the grid, believed free of walls, and not the believed agent cell | The believed object moves one cell in the first direction (in the event's order) that passes. |
| **F3** model(a) | never | In this reasoner's action model, when *a* would push, the object is predicted to move in the rotated direction; Blocked if that cell is believed blocked. Every evaluation with the reasoner's own model is affected, including planning. |
| **F4** trap(p) | the event has no cell (the layout has no dead square) | *p* is added to the reasoner's "not dead" overrides: after every run of its trap analysis, *p*'s status is overwritten with "not dead" (Stage 3 already applies these overrides after the analysis, and before the "dead" corrections). |
| **F5** object | the object has not yet moved in this run; **(A3)** the object's previous square is the square the reasoner believes the agent is on; **(A3)** the reasoner believes the previous square is a wall | The believed object goes back to the object's previous square. |

**Details:**
- **F5's previous square** [READING, D-034] is read from the **true** observation history of the run, up to and including the current observation: the object's position just before its most recent move. ("The object has moved" means two consecutive true observations show different object positions.) A3's checks use **this reasoner's own belief**, as A3 requires.
- **F3 faults combine** [READING, D-035]. Each reasoner keeps a table of faulted actions (each with its rotation). Applying an F3 event sets that action's entry, rebuilds the reasoner's **base** model from the table (standard rule for actions not in it), and removes any earlier "restored" mark on that action. A correction of `model(a)` still works as in Stage 3: it marks *a* restored, without changing the base model, so a correction of one action never removes the fault on another. *(The validation pilot replaced the whole base model at each F3 event, which would silently end an earlier F3 fault on another action without a correction, contrary to 12.3. This plan does not copy that.)*
- **After any applied fault**, the reasoner's prediction cache is emptied [READING, D-036], because its belief or its model has changed.
- **Common-mode events** need no special handling for C0 and C1: they are applied to the single reasoner, like every other event.

### 3.5 Activation records and amendment A4 [Phase B 12.3, A4, the 13.12 reporting table]

The injector keeps one **activation record** per (event, reasoner). At the activation point of decision step *t*:
- an event with status *scheduled* and step *t* is tried:
  - applied → **applied at *t***;
  - not applicable, type F1–F4 → **not applicable at *t***, with the reason (it lapses for good);
  - not applicable, type F5 → **pending**, with the reason (A4);
- an event with status *pending* (F5 only) is tried at every later step: applied → **applied at *t***; otherwise it stays pending, and the latest reason is kept.

At the end of the run, each event is reported in exactly one class of the 13.12 table:

| Final status | Reported as | Phase A 4.3 class |
|---|---|---|
| applied at its scheduled step | **Activated at its scheduled step** | Activated |
| applied later (F5) | **Activated after deferral**, with the application step and the delay | Activated |
| still scheduled | **Not activated: run ended before the scheduled step** | Not activated |
| not applicable (F1–F4) | **Not activated: not applicable at the scheduled step**, with the reason | Not activated |
| still pending (F5) | **Not activated: deferred, never applicable**, with the last reason | Not activated |

**Reason texts** (exact): `no cell of the required kind`, `the wall would be under the believed agent`, `the wall would be under the believed object`, `no direction passes the test`, `the layout has no true dead square`, `the object has not moved yet`, `the previous square is the believed agent square`, `the previous square is believed to be a wall`.

### 3.6 The fault activation point and the run with faults [Phase B 11.3 step 4; 11.8]

**C1.** Stage 3 left step 4 of `SingleAgent::decide` as a commented, empty step, placed after the check (step 2), the deferred investigation and the pending work (step 3), and before planning. Stage 4 adds one optional **activation hook** there (3.12). When the hook is empty, nothing happens, exactly as in Stage 3. *(The validation reference placed this point before the continuation of unfinished V1 checks; the C++ code places it after them, which is what 11.3 says: activation comes after step 3 is complete. The Stage 4 reference follows the C++ order.)*

**C0.** C0 never checks or investigates, so its activation point is at the start of `NaiveAgent::decide`, after the decision step counter and the budget are renewed, and before it plans or takes its next plan action.

**The run with faults** (`runWithFaults`) is the Stage 3 run loop with three additions:
1. it keeps the run's **true observation history** (the injector reads it for F5);
2. it connects the injector to the agent's activation hook before the first decision step;
3. at the end, it asks the injector to finish the activation records (3.5), and the scorer to finish the run's scores (3.7–3.10).

Everything else (outcomes, limits, the final check) is exactly the Stage 3 run loop. With an empty event list, `runWithFaults` gives exactly the same result and log lines as `runSingleAgent`.

**Exact values.**

*Injection reproduces Stage 3's second demonstration.* DEMO2 (Stage 3 5.4) with the **true** world, and one event `demo-1`: step 1, F1, (4, 3), blocked→free. C1's log lines are **identical** to Stage 3 demonstration 2 (25 lines, from `Plan: search found a plan of 4 actions` to `Step 18: North | predicted Pushed, observed Pushed | match`), followed by the same result: Success after 18 world steps, 0 safe stops, 1 mismatch, 1 correction (1 verified), **414 model evaluations**. A fault applied at step 1's activation point is the same wrong belief Stage 3 wrote by hand.

*The demonstration experiment* (events in 3.3; B = 200, S_max = 5, T_max = 100):

| Run | C1 outcome | World steps | Decision steps | Safe stops | Evaluations | C0 outcome | World steps | Evaluations |
|---|---|---|---|---|---|---|---|---|
| 500000-1 | Success | 12 | 12 | 0 | 270 | Success | 12 | 152 |
| 500000-2 | Success | 12 | 12 | 0 | 304 | Success | 12 | 152 |
| 500001-1 | Step limit | 100 | 100 | 0 | 9704 | Success | 16 | 156 |
| 500001-2 | Success | 18 | 18 | 0 | 469 | Success | 16 | 156 |
| 500002-1 | Step limit | 100 | 100 | 0 | 9406 | Step limit | 100 | 9200 |
| 500002-2 | Step limit | 100 | 100 | 0 | 9207 | Step limit | 100 | 9200 |
| 500003-1 | Success | 16 | 16 | 0 | 525 | Success | 16 | 256 |
| 500003-2 | Step limit | 100 | 100 | 0 | 9178 | Step limit | 100 | 9000 |

*Activation records in the demonstration experiment.* Every event is **activated at its scheduled step** in both C1 and C0, except:

| Event | C1 | C0 |
|---|---|---|
| 500001-1-3 (F5, step 8) | Not activated: deferred, never applicable (`the object has not moved yet`) | Activated after deferral at step 15 (delay 7) |
| 500002-2-3 (F5, step 9) | Activated after deferral at step 12 (delay 3) | Activated after deferral at step 12 (delay 3) |
| 500003-1-1 (F5, step 1) | Activated after deferral at step 9 (delay 8) | Activated after deferral at step 11 (delay 10) |

These numbers describe **how the instrument behaves on 8 demonstration runs**. They are not results, and they are never compared or interpreted (Phase A 9.3).

### 3.7 Scoring I: the life of each fault event [Phase B 12.3]

The **scorer** is experimenter code in `src/experiment/`. It reads the agent's public state (belief, records, model marks, overrides) and the true world; it never writes to the agent, and the agent never reads it (Brief 11.5.2).

**When the scorer looks** [READING, D-037]. Each decision step *t* has two **phases**:
- **phase A(*t*)**: from the start of `decide` to the activation point (receiving, checking, investigating, pending work);
- **phase B(*t*)**: from the activation point to the end of `decide` (planning, safety, commitment).

The scorer takes a **snapshot** at the activation point **before** any fault is applied (end of A(*t*)), and another when `decide` returns (end of B(*t*)). The final check after the last world action is a phase A with no phase B. For C0, phase A is empty. Everything the agent records in a phase is dated to that phase. This gives the order within a step that the definitions need (a claim made in A(*t*) comes **before** a fault activated at *t*; a claim made by an exhaustive search in B(*t*) comes after it).

**Active or not.** At every snapshot, for every activated event that has not ended:
- **Value faults** (F1, F2, F5) are active while the element's believed value differs from its true value at that moment (the true object position is the object's position in the current observation).
- **Process faults** (F3, F4) are active until the reasoner's state shows the true behaviour: F3 ends when action *a* is marked restored; F4 ends when *p* is among the reasoner's "dead" corrections.

**End states** (exactly one at the end of the run, with the step at which it was reached):
1. **Corrected.** The element became true in a phase in which the agent created a **correction record** for the same element with the **true value** (for the object: the true position at that time). The record's final status is reported: CONSISTENT or VERIFIED.
2. **Overwritten without correction** (value faults only). The element became true in a phase with no such correction record.
3. **Active at end of run.**

**Further rules** (12.3):
- If the correction that ended an event is later **REJECTED** and the fault is active again, the **same** event becomes active again (no new activation). Its end state is then decided afresh.
- A wrong-valued correction leaves the fault active (and counts under M6, 3.8).
- **Credit only through records.** "Overwritten without correction" never earns detection, correction or verification credit.

### 3.8 Scoring II: claims, detection and corrections [Phase B 12.5]

**Claims.** For C0 and C1, a **claim that an error exists** is a **correction record** created as CONSISTENT (12.5.6). Each record is one claim, at the phase in which it was created. (Candidates rejected inside V1 never become records, so they are not claims. OBJECTION and SAFE_STOP claims exist only in Stage 6.) A record later becoming VERIFIED is not a new claim.

**For each claim**, the scorer records the reasoner, the element (wall(c), object, model(a), trap(p)), the claimed value, and whether the value is true. Then:
- **Correctly identifies** event *e* (12.5.7): the claim names *e*'s reasoner and *e*'s element, and *e* was active at the start of the claim's phase.
- **False alarm** (M3, 12.5.9): no activated fault in that reasoner and element was active at the start of the claim's phase.
- **Wrong accepted correction** (M6, 12.5.11): the claimed value is not the true value. (A trap(p) claim is true if *p* is a true dead square. A model(a) claim is always "the standard rule", which is the true behaviour.)

**For each event *e*** (common-mode events count once, Phase A 5.2; for C0 and C1 there is only one reasoner, so "at least one affected reasoner" is that reasoner):
- **Strict detection** (M2, 12.5.8): the first claim that correctly identifies *e*. Its step, and the **detection delay** (M5) = detection step − activation step.
- **Verified correction** (M4, 12.5.10) [READING, D-038]: *e* ended as **Corrected** by a record with the true value, made while *e* was active, and that record reached VERIFIED (and was not later REJECTED). *(12.5.10 says the correction "reached VERIFIED while e was active". A correct correction ends a fault as soon as it is applied, at CONSISTENT, so VERIFIED always comes after the fault has ended. The reading keeps the intended meaning: the correction was made while the fault was active, and was later verified.)*
- **Mismatch detection** (M2a, 12.5.5): the first mismatch record caused by *e* (3.9) while *e* is active.
- **Reasoner attribution** (M2b): two-reasoner conditions only; not computed in Stage 4.

**Per run:**
- **Justified safe stops** (M7, 12.5.12): safe stops at which, at the end of that decision step, at least one activated fault was active.
- **Source-label accuracy** (M8, 12.5.13): for each mismatch record, its suspected source is **correct** if it equals the ground-truth source (S2 for F1, F2, F5; S1 for F3, F4) of at least one fault that caused it; S5 counts as "not identified", never correct. A mismatch caused by **no** fault is counted separately as a **possible bug** (Phase A 8.4).
- **Decision steps, world steps, safe stops, mismatches, claims, false alarms, wrong accepted corrections, model evaluations** (M3a, M10, M11, M12).

### 3.9 Scoring III: shadow evaluation and effective use [Phase B 12.4, 12.5.2–12.5.4]

**What it is.** For a computation the reasoner performs while it holds an active fault, the scorer repeats the computation with the faulted element at its true value (the **shadow**). If the result differs, the fault had an **effective use**. Shadow computations never use any reasoner's budget, are never visible to any reasoner, and live in the scoring code.

**The shadow of an input** for one active event *e*, changing only *e*'s element (other faults stay in place, so each fault is judged on its own, 12.5.4):

| Type | Shadow input |
|---|---|
| F1 wall(c) | the same state with c's wall status set to its true value |
| F2, F5 object | if the state's object is at the reasoner's current (wrong) believed object position: the same state with the object at its true current position; otherwise unchanged [READING, D-039] |
| F3 model(a) | for an evaluation of action *a* with the reasoner's own model, while *a* is not restored: the standard rule instead |
| F4 trap(p) | for the trap analysis: the analysis without *p*'s "not dead" override |

If the shadow input is not a valid world (for example, the true wall at c is under the state's agent), the use counts as effective.

**Three kinds of computation are observed** [READING, D-040]:

| Computation | Effective use when |
|---|---|
| **Model evaluation** (every charged evaluation, with the own model or a variant) | the predicted outcome, or the footprint contents (10.5, the two-prediction rule), differ from the shadow's |
| **Heuristic value** | the value differs from the heuristic of the shadow state with the shadow map |
| **Dead-square status read** (the planner's pruning, the safety rule, the local check and the trap verification each read one cell's status) | the status differs from the shadow map's status of that cell |

*Why status reads and not whole analysis runs:* Phase A 4.3 defines a detection opportunity as the faulty belief being "actually used in at least one prediction or decision". Running the trap analysis is not a use; reading *p*'s status to prune a plan, or to reject an action, is. (Comparing whole maps would make F1 and F4 "used" at every recomputation, even when no decision depended on them.)

**What effective use gives:**
- **Detection opportunity** (12.5.3): at least one effective use from activation until the fault ends or the run ends. Activated events with no opportunity are reported (Phase A 4.3, M12).
- **Revealing opportunity** (12.5.3): an effective use in a **committed prediction that was then checked**. The scorer computes the shadow of each committed prediction directly, from the belief at commitment, when `decide` returns. Every committed prediction of C1 is checked (at the next step, or by the final check). C0 commits nothing, so it has no revealing opportunities, by definition.
- **Mismatch causes** (12.5.4): a mismatch record at step *t* is caused by every event that was active when the prediction was committed at step *t*−1, had an effective use in that committed prediction, and was still active at the start of phase A(*t*).

**Known limitation of D-039.** For hypothetical states inside a search, "the object's true value" has no exact meaning once the object has moved in the hypothesis; the shadow then leaves the state unchanged, so such uses are never counted as effective. For a recorded past state whose object happened to be at the believed position, a use may be counted as effective although it is not. Both affect only the **opportunity** counts, which are reported, never the primary denominators (12.6). Committed predictions, mismatch causes and revealing opportunities are always computed from the real current belief, where the shadow is exact.

**Primary scoring does not depend on opportunity accounting** *(added in v1.1, at ChatGPT's request)*. M2 (strict detection), M3, M4, M5 and M6 are computed **only** from claims, correction records and the event life (3.7–3.8). They never read the observer, the opportunity counts or any shadow result. Their denominators are activated events (12.6), never events with an opportunity. This is tested (IE-T4, 6.6): scoring the demonstration experiment with the observer disabled gives exactly the same M2, M3, M4, M5 and M6 values, per event and per run, as with it enabled.

**Data kept for Stage 6** *(added in v1.1)*. Every event record keeps its provenance: event identifier, layout seed, run seed, original scheduled step, type, details, target and common-mode flag. Every activation record, claim and end state is stored **per reasoner** (reasoner `R` in C0 and C1), so Stage 6 can score two reasoners, common-mode events in each affected reasoner (Phase A 5.2), and the two-reasoner measures, without changing the Stage 4 records.

### 3.10 What one run produces

One **run record** and one **event record** per scheduled event, as plain structs. The CSV rows (3.11) are made from them.

### 3.11 CSV output [Brief Stage 4: "CSV output"]

**Format.** Comma-separated, one header line, one line per record, `\n` line endings, no quoting (no field ever contains a comma, quote or line break). Booleans are `1` or `0`. A value that does not exist (for example, a detection step for an undetected event) is an **empty** field. Positions are written `x:y`, for example `4:3`.

**`runs.csv`** columns, in this order:
`run_id, condition, layout_seed, run_seed, outcome, decision_steps, world_steps, safe_stops, justified_safe_stops, mismatches, mismatches_without_fault_cause, source_labels_correct, source_labels_scored, claims, false_alarms, wrong_accepted_corrections, evaluations, events_scheduled, events_activated`

**`events.csv`** columns, in this order:
`event_id, condition, reasoner, layout_seed, run_seed, type, target, common_mode, cell, direction, order, action, rotation, scheduled_step, reported_as, activation_step, activation_delay, not_activated_reason, opportunity, opportunity_step, revealing_opportunity, mismatch_detected, mismatch_step, strictly_detected, strict_step, detection_delay, end_state, end_step, correction_status, verified_correction`

- `run_id` = `<layoutSeed>-<runSeed>`; `condition` = `C0` or `C1`; `reasoner` = `R` for C0 and C1 (Stage 6 uses `R1`, `R2`).
- `outcome`: `Success`, `Step limit` or `Stopped -- unresolved` (as Stage 3).
- `reported_as`: one of the five texts of 3.5. `end_state`: `Corrected`, `Overwritten without correction`, `Active at end of run`, or empty for events not activated. `correction_status`: `CONSISTENT`, `VERIFIED` or empty.
- `order` is the four directions separated by single spaces.

The functions return the text. A separate function writes text to a file and reports success or failure (no exceptions). **Stage 4 writes no result files** into the repository: the demonstration prints its CSV lines to the screen, and the file-writing test writes and deletes one file in the build folder. Writing the experiment's files is Stage 7's job.

### 3.12 Instrumentation exception IE-1 (changes to Stage 3 code) [Phase A 3: C1 frozen] *(rewritten in v1.1)*

**The rule it is an exception to.** Phase A 3 says C1 changes after `stage-3` **only** to fix a genuine bug. H1–H4 below are **not** bug fixes, so they need their own, explicitly approved exception. Saying that they change no behaviour is not enough on its own: the exception must be approved by the owner, and its invariants must be proved.

**Why an exception is needed at all.** The protocol itself requires an activation point (11.3 step 4, left empty in Stage 3) and shadow evaluation (12.4), which must see the reasoner's computations. The only alternative is a second copy of C1 inside the scorer, and two copies could drift apart.

**IE-1, in full:**
1. **The tag `stage-3` is never moved or changed.** It continues to mark the C1 baseline as built in Stage 3. IE-1 changes later commits only.
2. **Exactly four additions are permitted**, with exactly this behaviour:

| # | File(s) | Permitted addition | Permitted behaviour | Sub-stage |
|---|---|---|---|---|
| H1 | `single_agent.h/.cpp`, `naive_agent.h/.cpp` | `std::function<void(int)> activationHook` in the agent's state | Called once per decision step at the activation point (C1: after step 3; C0: at the start of `decide`), with the decision step number. When empty, nothing is called. | 4c |
| H2 | `agent_model.h/.cpp` | `void setBase(ActionModel model)` | Replaces the base model and keeps the restored marks. Called **only** by the fault injector (F3), never by agent code. | 4c |
| H3 | `step_predictor.h/.cpp` | an optional pointer to an observer | After each **charged** evaluation, passes (state, action, own model or variant, result) to the observer. Null: nothing happens. Cache hits are not reported, and the observer is never consulted for any value. | 4e |
| H4 | `dead_squares.h/.cpp`, `heuristic.cpp`, `investigation.cpp`, `planner.cpp`, `single_agent_choose.cpp`, `verification.cpp` | an optional observer pointer carried by the map (set by `DeadSquareCache::get` from the cache's own pointer); `readDeadStatus(map, cell)`, used at the five places that read a cell's dead status | `readDeadStatus` returns exactly the stored value and reports (cell, value) to the observer; `heuristic` reports its value. Null: nothing happens. | 4e |

**Two kinds of attachment** (added at the owner's question, October 20, 2026; wording only). Anything attached to C1 at the same point as the hook could, in principle, change what C1 does, just as anything tied to a dog's collar could pull the dog. So the four additions are of two kinds, and each is limited differently:
- **H1 and H2 can pull, by design.** They are how faults get into C1, and putting faults in is the point of Stage 4. What they may change is limited to the list in 3.4. With an empty event list they change nothing, and IE-T1 and IE-T2 prove that.
- **H3 and H4 can only watch.** This is enforced by the code's types, not just promised: the observer receives only `const` references or copies, its functions return `void`, and no agent code reads any value from it. Because it gives nothing back, it has no way to pull. IE-T2 and IE-T3 prove the output is identical with it attached.

3. **Prohibited:** any change to C1's or C0's decisions, planning, safety rule, investigation, verification, corrections, safe stops, cache semantics (what is cached, when the cache is cleared, what counts as a hit) and budget accounting (what is charged, when, and how much). The observer receives copies or constant references only, returns nothing, and the agents never read anything from it. The only state the fault injector changes is listed in 3.4 (belief values, the F3 table and base model, the "not dead" overrides, and emptying the cache after an applied fault, D-036); that is fault injection, not a change to C1.
4. **Invariant 1: all 353 Stage 3 checks pass**, unchanged, after 4c and after 4e.
5. **Invariant 2: exact equality of the Stage 3 demonstration output** (Stage 3 5.5: both C1 demonstrations, lines and evaluation counts; and, for C0, identical results in the FR4 runs), with instrumentation **disabled** (no hook, no observer) **and enabled** (an empty-list fault injector on the hook, and a counting observer on H3 and H4). Tested as IE-T1 and IE-T2 (6.5).
6. **Invariant 3: observed evaluations equal the existing budget counter.** With the counting observer attached, the number of evaluations reported through H3 equals `totalEvaluations()` (the predictor's own total) for every Stage 3 demonstration and every demonstration-experiment run. Tested as IE-T3 (6.6).
7. **Later changes.** Any later change to H1–H4, or any new instrumentation, is recorded **separately** in `DECISIONS.md` as an amendment to IE-1 and approved by the owner, and invariants 1–3 are rechecked and reported before the change is committed.

**Approval.** IE-1 is recorded as D-041. ChatGPT asked for it (review of v1.0). **The owner approved it on October 20, 2026** ("yes I approve"), before 4c, as recorded in Section 12.

### 3.13 Settings
Stage 4 code takes every number as a parameter. The values used in tests and demonstrations are the frozen Section 14 values (B = 200, S_max = 5, T_max = 100, T_v = 3; 3 events per run at steps 1–10; 20% shares; master seed 4242), except where a test says otherwise.

---

## 4. Code structure

### 4.1 New and changed files

**New folder `src/experiment/`** (experimenter code; agents never include these headers):

| File | Contents | Sub-stage |
|---|---|---|
| `seeded_random.h/.cpp` | `SeededRandom` (3.1) | 4a |
| `true_search.h/.cpp` | `shortestSolutionLength` (3.2) | 4a |
| `layout_generator.h/.cpp` | `generateLayout`, `GeneratedLayout` (3.2) | 4a |
| `fault_event.h` | `FaultType`, `FaultTarget`, `F1Direction`, `Rotation`, `FaultEvent` | 4b |
| `fault_events.h/.cpp` | `RunKey`, `generateFaultEvents` (3.3), `faultEventText` | 4b |
| `fault_injector.h/.cpp` | `ActivationRecord`, `FaultInjector`, `tryApplyFault`, the F3 faulty model (3.4–3.5) | 4c |
| `fault_run.h/.cpp` | `runWithFaults` for C1 and C0 (3.6) | 4c |
| `scorer.h/.cpp` (+ `scorer_claims.cpp`) | `Scorer`, phases, event life, claims (3.7–3.8) | 4d |
| `shadow.h/.cpp` | shadow inputs and effective use (3.9) | 4e |
| `csv_output.h/.cpp` | CSV rows and `writeTextFile` (3.11) | 4f |
| `fault_demo.h/.cpp` (in `src/demo/`) | the Stage 4 demonstration | 4f |

**Changed:** the Stage 3 files in 3.12 (H1–H4); `CMakeLists.txt`; `main.cpp` and `project_info.cpp` (stage number 4, in 4f); `README.md` (4f); `docs/DECISIONS.md` (4f).

**New tests:** `test_seeded_random.cpp`, `test_layout_generator.cpp`, `test_fault_events.cpp`, `test_fault_injector.cpp`, `test_fault_run.cpp`, `test_scorer.cpp`, `test_shadow.cpp`, `test_csv_output.cpp` (more files if a file would exceed about 200 lines).

### 4.2 Interfaces fixed by this plan
Exact signatures are given in each sub-stage prompt. The fixed points:
- `SeededRandom(std::uint64_t seed)`; `std::uint64_t next()`; `int below(int n)`; `template<class T> void shuffle(std::vector<T>&)`; `template<class T> std::vector<T> pickDistinct(std::vector<T>, int k)`.
- `std::optional<int> shortestSolutionLength(const WorldState&)`.
- `struct GeneratedLayout { bool found; WorldState world; int attempts; int shortest; }`; `GeneratedLayout generateLayout(std::uint64_t layoutSeed)`.
- `struct RunKey { std::uint64_t layoutSeed; int runSeed; }`; `std::vector<std::vector<FaultEvent>> generateFaultEvents(const std::vector<RunKey>&, const std::vector<WorldState>& layouts, std::uint64_t masterSeed)`.
- `FaultInjector` holds the events and the activation records, a pointer to the true observation history, and `void atActivationPoint(<agent state>, int decisionStep)` for C1 and for C0 (two overloads; no inheritance).
- The scorer and the CSV functions are fixed in plan v1.2 (Section 5), after the review of 3.7–3.11.

---

## 5. Expected program output

The program's existing lines (Stage 3 5.5) are unchanged, except the stage number (`Stage: 4`). In 4f, a Stage 4 section is added at the end:
1. a heading and one sentence saying that this is the measuring instrument working on demonstration material, not an experiment;
2. layout 500000, rendered, with its attempts and shortest solution;
3. the fault-event list of runs 500000-1 and 500003-1;
4. C1 and C0 on runs 500000-2 and 500003-1: the activation records and the run results;
5. the `runs.csv` and `events.csv` lines for those four (run, condition) pairs.

**The exact text of this section, and the exact scoring values for 4d–4f, are added in plan v1.2**, produced by the reference implementation once ChatGPT has reviewed the definitions in 3.7–3.11. Producing them before that review would build exact numbers on definitions that may still change.

### 5.1 Things Stage 4 shows but does not decide [OBSERVATION]
- In demonstration run 500001-1, two F2 faults move C1's believed object two cells west of the true object. C1 then finds no plan within its budget and moves back and forth (the known behaviour of Stage 3 plan 5.6) for 100 steps, with **no mismatch**: its footprint never covers the true object. This is the "noticing is the hard part" limitation of 12.7.2, seen in practice. It is not a bug, and nothing is changed because of it (C1 is frozen; Phase A 9.3).

---

## 6. Required correctness tests

Each test uses exact values from the reference. Test names continue the project's style.

### 6.1 Seeded random numbers (`test_seeded_random.cpp`) — 4a
- **RN1–RN6**: the six rows of the table in 3.1.
- **RN7**: two generators with the same seed give the same 1,000 numbers; seeds 1 and 2 differ at the first number.
- **RN8**: `below(n)` stays within 0…n−1 for n = 1, 2, 3, 7, 100 over 1,000 calls each.

### 6.2 Experimenter search and layouts (`test_layout_generator.cpp`) — 4a
- **LG1**: `shortestSolutionLength` on Stage 3 DEMO is 4; on a hand-built unsolvable world (object in a corner) it is "none"; on a world with the object already on the destination it is 0.
- **LG2**: layouts 500000–500003 and 900000–900001 give exactly the values of 3.2 (attempts, shortest, positions, dead-cell count, wall count).
- **LG3**: layouts 500000 and 900000 render exactly as in 3.2; layouts 500004–500009 have the attempts and shortest solutions in 3.2.
- **LG4**: every generated layout is valid, has border walls, a shortest solution of 12–40 and at least one true dead square (checked for 500000–500009).
- **LG5** (material only): all 40 pilot and all 400 confirmatory layouts are found, with the attempt maxima and shortest-solution ranges in 3.2.
- **LG6**: generating the same layout twice gives identical worlds.

### 6.3 Fault-event lists (`test_fault_events.cpp`) — 4b
- **FE1**: the demonstration experiment gives exactly the 24 events of 3.3 (identifier, step, type, target, details).
- **FE2** (material only): the pilot list's counts in 3.3, and run 900000-1's events.
- **FE3** (material only): the confirmatory list's counts in 3.3, its per-step counts, and the first run's events.
- **FE4**: every run has 3 distinct steps from 1–10, sorted; every F1 cell is interior, not the destination, with the true status its direction requires; every F4 cell is a true dead square.
- **FE5**: generating twice gives identical lists; changing the master seed changes the list.
- **FE6**: `round-half-up` gives 0, 1, 1, 2, 10, 240 for counts 2, 3, 5, 8, 48, 1200.

### 6.4 Applying faults (`test_fault_injector.cpp`) — 4c
Using Stage 3 DEMO (agent (3, 3), object (5, 3), destination (8, 3)) unless stated:
- **FI1 F1**: free→blocked at (3, 3) → not applicable, `the wall would be under the believed agent`; at (5, 3) → `the wall would be under the believed object`; at (6, 5) → applied, the belief's (6, 5) is blocked, the true world is unchanged. Blocked→free on a believed wall → applied, now free.
- **FI2 F2**: order North, East, South, West → object believed at (5, 2). A belief with the object at (1, 1) and walls at (2, 1) and (1, 2) → not applicable, `no direction passes the test`. A belief with the agent at (5, 2) and order North, East, … → object believed at (6, 3) (North is the agent's cell).
- **FI3 F3**: after East-right, predicting East from agent (4, 3) gives Pushed with the object at (5, 4) (the standard rule gives (6, 3)); with a believed wall at (5, 4) it gives Blocked. Other actions are unaffected. A second F3 (North, left) keeps the East fault (both faulted). Restoring East leaves North faulted.
- **FI4 F4**: the cell's status becomes "not dead"; every other cell's status equals the true analysis; a later "dead" correction of the cell makes it dead again (the fault is ended by a correction).
- **FI5 F5**: with a true history in which the object moved from (5, 3) to (6, 3) and the agent is believed at (5, 3) → not applicable, `the previous square is the believed agent square`; after the agent moves North to (5, 2) → applied, object believed at (5, 3). With no object move → `the object has not moved yet`. With the previous square believed blocked → `the previous square is believed to be a wall`.
- **FI6**: after any applied fault, the prediction cache is empty.
- **FI7 A4**: in the demonstration experiment with C1, event 500003-1-1 (F5, step 1) is "Activated after deferral", application step 9, delay 8; event 500001-1-3 (F5, step 8) is "Not activated: deferred, never applicable", last reason `the object has not moved yet`.
- **FI8**: an event at step 10 in a run that succeeds in 4 world steps (Stage 3 DEMO) is "Not activated: run ended before the scheduled step". An F1 event that is not applicable is "Not activated: not applicable at the scheduled step" with its reason, and is never tried again.

### 6.5 The run with faults (`test_fault_run.cpp`) — 4c
- **FR1**: the injected-F1 run of 3.6 gives log lines identical to Stage 3 demonstration 2, and 414 evaluations.
- **FR2**: the demonstration experiment gives exactly the outcomes, steps, safe stops and evaluations of 3.6, for C1 and C0.
- **FR3**: the activation records of the demonstration experiment are exactly those of 3.6.
- **FR4**: with an empty event list, `runWithFaults` and `runSingleAgent` give identical results and lines, for both Stage 3 demonstrations and for C0.
- **FR5**: running the same run twice gives identical results (reproducibility).
- **FR6** (H1–H2 change nothing): all Stage 3 checks pass; `setBase` is called only by the fault injector.
- **IE-T1** (invariant 2, instrumentation disabled): both Stage 3 C1 demonstrations give exactly the Stage 3 5.5 lines and evaluation counts, and C0 gives identical results on the FR4 worlds.
- **IE-T2** (invariant 2, instrumentation enabled): the same, with an empty-list fault injector on the hook (4c), and also with the counting observer on H3 and H4 (added in 4e).

### 6.6 Scoring (`test_scorer.cpp`, `test_shadow.cpp`) — 4d, 4e
Fixed in plan v1.2. They will include at least:
- the injected-F1 run of 3.6, scored by hand: activated at step 1; effective use in the committed prediction at step 1 (predicted Moved, shadow Blocked), so a revealing opportunity; mismatch at step 2 caused by the event, label S2 correct; claim at step 2 (correction 1, wall (4, 3) blocked, true value) → strict detection at step 2, delay 1; correction VERIFIED at step 7 → verified correction; end state Corrected (VERIFIED); 0 false alarms;
- a false alarm (a hand-made wrong correction with no fault), a wrong accepted correction, an "overwritten without correction" value fault, and a REJECTED correction that reactivates its event;
- a claim made in phase A of the event's own activation step (not a detection: the event was not yet active);
- for each type, one computation with and one without effective use, including a dead-status read and a heuristic value;
- **IE-T3** (invariant 3): with the counting observer, observed evaluations = `totalEvaluations()` for every Stage 3 demonstration and every demonstration-experiment run, C1 and C0;
- **IE-T4** (primary scoring independent of opportunity accounting, 3.9): the demonstration experiment scored with the observer disabled and enabled gives identical M2, M3, M4, M5 and M6 values;
- the demonstration experiment's exact event and run records.

### 6.7 CSV (`test_csv_output.cpp`) — 4f
Fixed in plan v1.2: exact header lines; exact rows for the demonstration runs; empty fields; `writeTextFile` writes, reads back, and deletes one file; a failing path returns false.

---

## 7. Implementation order

As in Stages 1–3: **one sub-stage at a time**, each built, checked by the owner, reviewed by Claude, and committed by the owner before the next. **Cursor never commits.** Each sub-stage starts in a **fresh Cursor chat**, with its prompt saved first in `docs/copilot_prompts/` (D-013).

| Step | Sub-stage | Files | Tests | Deliberate failure check (Brief L7) |
|---|---|---|---|---|
| 1 | — | The owner commits this approved plan | — | — |
| 2 | **4a** seeded random numbers and layouts | `seeded_random.*`, `true_search.*`, `layout_generator.*`, tests | RN1–RN8, LG1–LG6 | Yes: on `below`'s threshold |
| 3 | **4b** fault-event lists | `fault_event.h`, `fault_events.*`, tests | FE1–FE6 | Yes: on the common-mode share |
| — | — | **The owner approves IE-1** (3.12, D-041): **done, October 20, 2026** | | |
| 4 | **4c** applying faults and the run with faults | `fault_injector.*`, `fault_run.*`, H1, H2, tests | FI1–FI8, FR1–FR6, IE-T1, IE-T2 | Yes: on A3's agent-square check |
| — | — | **Plan v1.2**: exact scoring values, CSV text and program output, from the reference; ChatGPT reviews; the owner approves v1.2 **and D-038** | | |
| 5 | **4d** scoring I and II | `scorer*`, tests | 6.6 (records and claims) | Yes: on the phase of a claim |
| 6 | **4e** shadow evaluation | `shadow.*`, H3, H4, tests | 6.6 (effective use), IE-T2 with the observer, IE-T3, IE-T4 | Yes: on one status-read site |
| 7 | **4f** CSV, demonstration, stage number, documentation | `csv_output.*`, `fault_demo.*`, `main.cpp`, `project_info.cpp`, README, DECISIONS | 6.7; **program output matches plan v1.2 exactly** | — |
| 8 | — | Fresh-build check: reset the CMake cache, rebuild, no warnings, output and tests correct | | |
| 9 | — | Owner verification gate (Section 10) | | |
| 10 | — | Record, commit, and tag **`stage-4`** (owner only) | | |

**If the C++ and the reference disagree.** The reference matched the Stage 3 C++ code exactly on everything Stage 3 tested, but faults will exercise paths Stage 3 never reached. A difference is **never** settled by changing an expected value to match the code. Claude finds which one follows the protocol and the plan:
- if the reference is wrong, it is fixed and the new value is recorded with the reason;
- if **C1 or C0** is wrong, that is a **genuine bug** (Phase A 3): it is fixed only with the owner's approval, recorded in `DECISIONS.md`, and all Stage 3 checks are re-run.

**Running programs and CTest from PowerShell.** After **Build → Build Project**, in the same terminal tab:
```
$env:Path = "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\mingw\bin;" + $env:Path
.\cmake-build-debug\ai_hologram.exe
.\cmake-build-debug\ai_hologram_tests.exe
ctest --test-dir cmake-build-debug --output-on-failure
git --no-pager status
git --no-pager diff --stat
```

---

## 8. Decisions to record in `docs/DECISIONS.md` (sub-stage 4f)

| ID | Decision | Reason | Alternatives considered |
|---|---|---|---|
| D-030 | The project's only random generator is SplitMix64 with rejection sampling for `below`, Fisher–Yates `shuffle`, and front partial Fisher–Yates `pickDistinct`, exactly as in plan 3.1. | Gives identical material on every compiler and machine. The standard distributions are not fully specified. | `std::mt19937` with standard distributions (not portable); Python's generator (used in the validation pilots; not available to the C++ code). |
| D-031 | "About 12% interior walls" means each interior cell is independently a wall with probability 12/100; agent, object and destination are three distinct free cells; up to 10,000 attempts. | The simplest exact reading of Section 14; all 440 pilot and confirmatory layouts are found within 20 attempts. | A fixed wall count (not what "about" suggests). |
| D-032 | The fault master seed is 4242, fixed before any confirmatory run; per-run streams use `(4242 << 40) + (layoutSeed << 8) + runSeed`. | The protocol fixes seed ranges but not this seed. 4242 is the value used in validation. The stream formula is distinct for every run. | A seed derived from the bootstrap seed (mixes two purposes). |
| D-033 | An F1 or F4 event with no possible cell is "not applicable", with its reason. | 12.2 does not list this case. It never occurs in the pilot or confirmatory lists; the rule only makes the code total. | Redrawing (would change other events' numbers). |
| D-034 | F5's previous square comes from the run's true observation history; A3 is checked in the reasoner's own belief. | 12.2 defines F5 by the object's actual moves; A3 says each reasoner's own view decides. The validation pilot did the same. | The reasoner's own remembered positions (not defined for C0). |
| D-035 | F3 faults on one reasoner combine in a table of faulted actions; a correction of model(a) restores only *a*. | 12.3: a process fault ends only by a correction. Replacing the whole model (as the validation pilot did) would end an earlier F3 fault silently. | Replacing the base model at each F3 event. |
| D-036 | The prediction cache is emptied after any applied fault. | The belief or the model has changed; cached results may no longer be what the reasoner would predict. | Clearing only for some types. |
| D-037 | The scorer dates every record to a phase (A before the activation point, B after it) of its decision step, and judges "active at the time of the claim" at the start of the claim's phase. | Gives the within-step order the definitions need (12.5.6–12.5.9), without changing the agents. | Step-level dating only (would credit a claim made before the fault's own activation). |
| D-038 | **Clarification of a frozen measurement definition (12.5.10, M4)**, not a change: a verified correction is a correction with the true value, made while the event was active, that later reached VERIFIED and was not REJECTED. It is fixed now, before any result exists, and it can never be revisited after results are seen. **Status: accepted by ChatGPT (review of v1.0). Approved by the owner on October 20, 2026 ("yes I approve").** | 12.5.10 says "reached VERIFIED while e was active", but a correct correction ends the fault at CONSISTENT, so read literally M4 would always be zero. This keeps the intended meaning. | The literal reading (M4 always zero). |
| D-039 | For object faults, the shadow changes a state's object only if it is at the reasoner's current wrong believed position. | "The true value" of the object is defined only for the present; inside a search it is not. | Shadowing whole searches (expensive, and still undefined after a hypothetical push). |
| D-040 | Effective use is observed in model evaluations, heuristic values and dead-status reads, not in whole trap-analysis runs. | Phase A 4.3: an opportunity is a use "in a prediction or decision". | Comparing whole maps at each analysis run (counts F1 and F4 as used even when no decision depended on them). |
| D-041 | **Instrumentation exception IE-1** (plan 3.12): `stage-3` unchanged; exactly H1–H4 with their permitted behaviour; changes to decisions, planning, verification, corrections, cache semantics and budget accounting prohibited; invariants 1–3 (353 checks; identical Stage 3 output with instrumentation on and off; observed evaluations = budget counter) proved at 4c and 4e; any later instrumentation change recorded separately and rechecked. **Approved by the owner on October 20, 2026 ("yes I approve"), before 4c.** | Phase A 3 allows C1 changes only for genuine bugs; H1–H4 are not bug fixes, so they need an explicit, narrow exception (ChatGPT, review of v1.0). The protocol requires the activation point (11.3 step 4) and shadow evaluation (12.4). | Copying C1 into the scorer (two copies could drift apart); treating H1–H4 as outside the frozen rule (rejected by ChatGPT). |

---

## 9. Documentation changes

### 9.1 Code comments
Every new file has the three-part header (Purpose / "Introduced in Stage 4" / Architecture). Beginner comments explain, where each first appears: why the project has its own random generator; what a seed is, and why the same seed must give the same experiment; why the experimenter's search is never visible to an agent; why the event list is balanced over the whole experiment; value faults and process faults; A3 and A4 in plain words; the two phases of a step; what a shadow computation is, and why it never uses the reasoner's budget; why credit comes only through records.

### 9.2 `README.md` status
> **Current status: Stage 4 — fault injection and measurement.** The project contains a 10×10 grid world (Stage 1), a world model and comparison (Stage 2), and C1, the competent single agent, with C0, a naive reference (Stage 3). Stage 4 adds the experiment's measuring instrument: seeded generation of layouts and fault-event lists, fault injection into a reasoner's beliefs or reasoning exactly as the frozen Experiment 1 protocol defines, scoring of detection and correction from the agent's own records, and CSV output. Demonstrations use demonstration seeds only. **No two-agent reasoning exists, and no experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

### 9.3 `docs/EXPERIMENT_LOG.md`
No change. Stage 4 runs no experiments.

### 9.4 `docs/copilot_prompts/`
One prompt file per sub-stage (`stage_4a_prompt.txt` … `stage_4f_prompt.txt`), plus any follow-ups, saved before use.

---

## 10. Stage 4 completion checklist (owner verification gate)

**Approval and history**
- [ ] This plan (v1.1, and v1.2 for 4d–4f) was approved and committed before the code it describes.
- [ ] Git history is intact; the gate was passed before the completion commit and the `stage-4` tag.

**Scope**
- [ ] Only the files in 4.1 (plus `CMakeLists.txt`, `README.md`, `docs/DECISIONS.md`, this plan, the handoff notes and the prompt files) were added or changed.
- [ ] Nothing from Section 2: no agent was run on pilot or confirmatory layouts; no two-agent code; no comparison statistics.
- [ ] The Stage 3 changes are exactly H1–H4, within IE-1; IE-1 was approved by the owner before 4c; `stage-3` is unchanged.
- [ ] D-038 was approved by the owner before 4d.

**Build, run, test**
- [ ] No warnings, including after a cache reset.
- [ ] `ai_hologram` output matches plan v1.2 exactly.
- [ ] `ai_hologram_tests`: all PASS, 0 failures, including all 353 Stage 3 checks.
- [ ] CTest passes.
- [ ] The five deliberate failure checks were done; the expected tests failed; everything passed after undoing them.

**Conformance to the protocol**
- [ ] Faults follow 12.2, A3 and A4 exactly; the activation records use the 13.12 classes.
- [ ] The event list follows Section 14 and 13.8 exactly (the counts in 3.3).
- [ ] Scoring follows 12.3–12.5; every [READING] is recorded in DECISIONS.md and was reviewed.
- [ ] The scorer never writes to an agent, and no agent reads the scorer or the experimenter's search.

**Code and documentation**
- [ ] No exceptions; no inheritance; functions under about 40 lines; files under about 200 lines (or a recorded reason).
- [ ] Header blocks and beginner comments (9.1); README matches 9.2; DECISIONS.md contains D-030 to D-041.
- [ ] The owner can explain the items in Section 11.

---

## 11. Things the owner should understand by the end

1. What a seed is, and why the same seed must give exactly the same experiment on any computer.
2. Why the fault list is made once for the whole experiment, and is the same for every condition.
3. The five fault types, and the difference between a value fault and a process fault.
4. Why some faults cannot be applied at their step (A3), and why F5 waits for its chance (A4).
5. Where in the decision step a fault is applied, and why there.
6. What counts as a claim, a detection, a false alarm and a verified correction, and why credit comes only through records.
7. What a shadow computation is, and why it never uses the agent's budget or is seen by the agent.
8. The difference between a detection opportunity and a revealing opportunity.
9. Why Stage 4 runs agents only on demonstration seeds.
10. Why the Stage 3 code changed only by observation-only additions, and how we proved it.

---

## 12. What actually happened (filled in by the owner at the end)

```
Owner approval of IE-1: 2026-10-20, "yes I approve" (after Claude's plain explanation)
Owner approval of D-038: 2026-10-20, "yes I approve" (same message)
Date completed:
Commit tagged stage-4:
Total number of checks in the final test run:
Did everything go as planned?
```

---

## 13. Instructions to Cursor for this stage

Each sub-stage prompt repeats these rules:
- Implement **only** the named sub-stage; never commit; never edit `docs/`, `.github/`, `.cursor/`, `LICENSE`, `.gitignore` or `results/` (except appending the handoff note).
- Use **exactly** the algorithms, orders and texts of this plan. Do not "improve" the generator, the draw order or any reason text: one changed draw changes every later number.
- Do not change any Stage 1–3 behaviour; the only allowed Stage 3 edits are H1–H4, in their sub-stages.
- If an expected value does not match, **stop and report** the actual value. Never change an expected value.

---

## 14. Open points for review (ChatGPT)

Every point is an implementation reading of the frozen protocol, not a change to it.
1. D-030–D-032: the generator, the wall rule, and the master seed and stream formula.
2. D-033: "no cell" as a reason for non-applicability.
3. D-034: F5's previous square from the true history.
4. D-035: F3 faults combine (a deliberate difference from the validation pilot).
5. D-036: cache emptied after a fault.
6. D-037: the two phases, and "active at the time of a claim".
7. D-038: the reading of M4.
8. D-039–D-040: the shadow of object faults, and the three observed computations.
9. D-041 and 3.12: **resolved in v1.1** as the explicit exception IE-1, for the owner's approval.
10. Layout generation in Stage 4 rather than Stage 7, because the event details depend on the layouts.
11. Two-reasoner scoring (OBJECTION and SAFE_STOP claims, M2b, M9, A1, I1) waits for Stage 6; the Stage 4 scorer works per reasoner, so Stage 6 can give it two.

---

## Revision history of this plan

| Version | Date | Change |
|---|---|---|
| 1.1 | 2026-10-20 | **Approved by the owner, with IE-1 and D-038.** After ChatGPT's review (4a, 4b approved; 4c conditional): 3.12 rewritten as the explicit instrumentation exception IE-1 with its seven conditions; D-038 recorded as a clarification of a frozen definition, owner approval required before 4d; primary scoring shown independent of opportunity accounting (3.9, IE-T4); per-reasoner data and provenance kept for Stage 6; tests IE-T1 to IE-T4; scoring-values plan renumbered v1.2. |
| 1.0 | 2026-10-20 | First draft for ChatGPT's review. Exact values for 4a–4c from the reference; scoring values, CSV text and program output to follow in v1.2 (originally announced as v1.1). |
