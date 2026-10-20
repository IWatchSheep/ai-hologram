# Stage 3 — Competent Single Agent (C1)

| | |
|---|---|
| **Project** | AI Hologram (`ai-hologram` repository) |
| **Stage** | 3 of the roadmap in *AI_Hologram_Project_Brief_v1.1.txt*, Section 10 |
| **Plan version** | 1.2 |
| **Date** | October 19, 2026 |
| **Status** | **APPROVED by the owner on October 19, 2026**, following ChatGPT's recommendation (version 1.1, plus the goal-at-start test ChatGPT asked to be kept, added in 1.2). Ready for implementation, beginning with Step 1 (the owner commits this plan); then **sub-stage 3a only**. Approval of the plan does not mean any Stage 3 code is implemented or verified: each sub-stage still needs its own tests and review. |
| **Governing specification** | AI_Hologram_Project_Brief_v1.1.txt (approved) |
| **Governing protocol** | `docs/EXPERIMENT_1_PROTOCOL.md` v0.6. **Phase A and Phase B approved** (commit `da9af19`); not frozen. **This plan implements Phase B Section 11 (C0 and C1) and the Phase B definitions it relies on (Sections 10 and 12). Where this plan and Phase B disagree, Phase B wins and the owner must be told.** |
| **Builds on** | Stage 2, complete and tagged `stage-2` (commit `e112145`) |

---

## 1. Purpose of Stage 3

From the brief (Section 10, Stage 3):
> Goal: one agent that runs the full predict → act → observe → compare → correct → replan loop and completes the task. Contents: a planner (e.g. breadth-first search over the agent's own belief), belief correction from mismatches, replanning, step logging. Note: this is the STRONG baseline for Experiment 1. It must be built carefully and must not be weakened later.

Stage 3 builds **C1, the competent single agent**, exactly as approved in protocol Phase B Section 11. It also builds **C0, the naive agent** (Phase B 11.8), because the protocol's required Stage 3 tests include C0 (Phase B 12.9, item 10).

C1 is the **main baseline** of Experiment 1. Phase A Section 3 says:
- it is built and tagged **before** any two-agent code exists;
- after that, it changes **only** to fix a genuine bug, recorded in `DECISIONS.md`, after which every condition is re-run;
- it is **never** adjusted in response to how the two-agent system performs.

**What Stage 3 does not do:**
- inject faults (Stage 4);
- define messages or agents' communication (Stage 5);
- build Agent 1 or Agent 2 (Stage 6);
- run experiments (Stages 7–8).

**In tests and in one demonstration, wrong beliefs and wrong action models are written by hand**, as in Stage 2. This is not fault injection.

[FACT] The Definition Gate (Brief Section 10) forbids fault-injection, agent-communication and experiment code until the protocol is complete and approved. Stages 0–3 "do not depend on these choices and may proceed while the protocol is written."

## 2. What Stage 3 must NOT contain

**Excluded features:**
- No fault injection, fault schedules, random numbers, or experiment runner. The fault activation point (Phase B 11.3, step 4) exists in the code as a clearly commented step that **does nothing** in Stage 3.
- No scoring code. Shadow evaluation (Phase B 12.4) belongs to Stage 4.
- No messages, second reasoner, or two-agent code.

**Budget and information rules:**
- No free computation other than those listed in Phase B 10.7. **Every** successor state, including a partial one, comes from a budgeted model evaluation (Section 3.4).
- No information restriction beyond Phase B 11.1. C1 receives every observation in full and may read any part of it, and of its history, during an investigation (Section 3.13).

**Code rules:**
- No exceptions, no assertions that stop the program, no inheritance (D-010).
- No changes to the Stage 1 rules, the Stage 2 types or functions, or their tests (except P2).
- No external libraries, threads, networking or GUI.

If anything here seems necessary, **stop and ask the owner**.

---

## 3. Concepts and exact behaviour

Section numbers in brackets refer to the protocol. Everything in this section is required behaviour; Section 6 lists the tests that check it.

### 3.1 Observation and history [10.2]

**The observation.**
```
struct Observation {
    int decisionStep;                      // the decision step this observation is delivered at
    WorldState state;                      // the exact true state
    std::optional<Action> lastAction;      // empty at the start and after a safe stop
    std::optional<MoveOutcome> lastOutcome;
};
```

**Who builds it.** The run loop (3.10) builds every observation **from `Environment` only**. Each one is delivered unchanged to the agent at every decision step.

**History.** The agent keeps every observation it receives, in order, in `std::vector<Observation> history`. Nothing ever changes an entry.

### 3.2 Footprint, comparison and update [10.5]

**`computeFootprint(beliefBefore, action, observedAfter, observedOutcome)`**

It returns the cells **c0**, **c1** and, when included, **c2**:
- c0 is the agent's cell in `beliefBefore`;
- c1 = c0 + *a*;
- c2 = c1 + *a*.

c2 is included if and only if:
- **(i)** `beliefBefore.object == c1`; **or**
- **(ii)** `observedOutcome == Pushed`; **or**
- **(iii)** `observedAfter.object == c1`.

**`compareFootprint(predictedState, predictedOutcome, observedState, observedOutcome, cells)`**

It reports:
- whether the outcomes differ;
- for each cell, in the order c0, c1, c2, which of {wall, agent here, object here} differ, with the predicted and observed values.

Cells outside the grid count as walls (`isWall`).

**`computeFootprintForTwoPredictions(...)`**

It is for later stages, but is built and tested now:
- c0 and *a* are shared;
- c2 is included if **either** belief has the object at c1, **or** either prediction is Pushed.

**`applyFootprintUpdate(belief, cells, observedState)`**

It changes:
1. for each footprint cell inside the grid, the belief's wall value, which becomes the observed one;
2. the agent's position, if the observed agent is in a footprint cell;
3. the object's position, if the observed object, **or** the believed object, is in a footprint cell.

**Nothing outside the footprint is changed.**

### 3.3 Dead squares, local check and heuristic [11.2, 10.7]

**`DeadSquareMap`.** For every cell it holds `bool dead` and `int pushDistance`, where −1 means "not reachable by relaxed pushes".

**`computeDeadSquares(beliefWalls, destination)`.** A breadth-first search **backwards from the destination over object cells only**:
1. Start with `pushDistance[destination] = 0`, and put the destination in a queue.
2. Take cell *r* from the front of the queue. For each direction *d*, in the order North, East, South, West:
   - let *q* = *r* − *d* (the object's cell before a push in direction *d*);
   - let *s* = *r* − 2*d* (where the agent must stand).
3. If *q* and *s* are both believed free, and *q* has no distance yet:
   - set `pushDistance[q] = pushDistance[r] + 1`;
   - add *q* to the queue.
4. Repeat from step 2 until the queue is empty.

A cell is **dead** if it is believed free and has no distance. Walls and the destination are never dead.

This is exactly the set *R* of Phase B 11.2. It never builds agent–object states and never calls the model, so its cost is bounded by 100 cells × 4 directions.

**Overrides**, applied after the analysis, in this order:
1. **Test-only "not dead" overrides.** A list of cells whose status is set to "not dead". This is the mechanism Stage 4 will use for F4. In Stage 3, only tests set it.
2. **Applied corrections.** A list of cells set to "dead".

Overrides change only the `dead` flag of the listed cells. Every other cell's status is unaffected.

**When the analysis runs.** Only when the believed walls or the override lists change. Each run increments a counter (`analysisRuns`).

**`localCheckSaysNotDead(belief, map, p)`.** True if and only if:
- *p* is the destination, **or**
- some direction *d* has *p* − *d* and *p* + *d* believed free, and *p* + *d* not dead.

It reads the walls and the neighbours' `dead` flags only, and never the stored status of *p*. Each call increments `localChecks`.

**`heuristic(state, map)`:**
- If the object's cell is dead, the heuristic is **infinite**.
- Otherwise it is *P* + max(0, manhattan(agent, object) − 1), where:
  - *P* = `pushDistance[object]` if that is at least 0;
  - *P* = manhattan(object, destination) otherwise. This happens only for an overridden cell.

Each call increments `heuristicCalls`.

When nothing is overridden, the heuristic never overestimates the number of actions still needed (it is admissible). The reasons: each push moves the object one cell, the relaxed distance can only be less than or equal to the real number of pushes, and the agent must first walk next to the object.

### 3.4 The step predictor: budget, cache and reserve [10.7]

All model evaluations of a reasoner go through one **`StepPredictor`**. It holds:
- the reasoner's `WorldModel`;
- a `BudgetCounter` with limit B, created anew at the **start of every decision step**, so unused budget is lost;
- a **cache** of results for (agent position, object position, action), valid for the current belief walls.

**Rules:**
- **Ordinary predictions.**
  - A prediction from the **current belief walls** with the **current model** is looked up in the cache first.
  - A cache hit is free. This is the "reuse within the same step" allowed by Phase B 10.7.
  - A miss calls `WorldModel::predict`, which uses exactly one evaluation, and stores the result.
- **Clearing the cache.** The cache is cleared at the start of every decision step, and whenever the belief walls, the trap overrides, or the action model change.
- **Variant predictions are never cached and never read from the cache.** These are predictions from a different belief, such as an alternative or hypothetical state, or with a different model, such as the old model or the standard rule. Each costs exactly one evaluation.
- **When the budget runs out,** every prediction returns "no prediction" and changes nothing.
- **Counts.** The predictor counts evaluations per step and in total. The free-computation counters of 3.3 and the comparison counter are kept beside it and reported.

**The reserve** *(clarified in v1.1)*. The reserve is the number of evaluations C1 must always keep for the **mandatory** predictions of the step:
- the final **committed prediction** (1);
- one **V2 alternative prediction** for each correction that may need testing.

  **reserve = 1 + (number of corrections with status CONSISTENT whose element is a wall, the object, or an action model)**

The reserve is recomputed whenever a correction's status changes.

**Two kinds of evaluation:**
- **Mandatory:** the committed prediction (and the plan check that becomes it), and the V2 alternative predictions.
- **Optional:** everything else. This means:
  - the belief re-prediction of an investigation (3.7 A.3);
  - unfinished V1 replays (3.8);
  - the trap V2 test (3.8);
  - cautious-mode checks (3.9);
  - the active-test walk search;
  - the main search.

**The rule for every optional evaluation.** It may be made **only if it leaves at least the reserve**: an uncached optional evaluation requires `remaining() > reserve` before it is made, or `remaining() − k >= reserve` for a group of *k* made together. A cached prediction costs nothing, so it is always allowed.

**How each part of the step keeps the reserve:**

| Part of the step | What happens when the reserve would be broken |
|---|---|
| Investigation re-prediction (3.7 A.3) | The investigation is **deferred**: C1 logs `investigation: budget too small for verification; retried next step`, takes a safe stop with reason `budget too small for verification`, and repeats the whole verification at the next step, with a fresh budget. |
| Unfinished V1 replays | They pause, and continue at the next step (cautious mode meanwhile). |
| Trap V2 test (up to 4 evaluations) | It is made all at once only if `remaining() − k >= reserve`. Otherwise it waits until a later step, and the correction stays CONSISTENT. |
| Cautious-mode checks | Checking stops. If no action has qualified, C1 takes a safe stop with reason `investigation unfinished, no cautious action`. |
| Active-test walk search and its test prediction | The test is skipped this step. |
| Main search | It returns OutOfBudget, and the fallback is used. If the search could not create any node, C1 takes a safe stop with reason `budget too small to plan`. |

**Mandatory predictions:**
- **The committed prediction** may use the reserve. If no budget is left for it, **C1 takes no world action**. It takes a safe stop with reason `no budget for a committed prediction`, so it never acts without a checked prediction.
- **V2 alternative predictions** are made in correction-number order while budget remains. A correction whose alternative cannot be afforded is simply not tested at this step.

**Consequence.** C1 never charges more than B in a step, and every world action has a committed prediction (tests SP7, C1-7, C1-11).

### 3.5 The planner [11.2] *(corrected in v1.1)*

**`search(root, goalTest, heuristic, predictor, reserve)`.** A standard A* search over the agent's **own belief**. The result is a shortest plan whenever the budget allows the search to finish.

**Bookkeeping**, kept per discovered state. A state is identified by (agent position, object position); the walls and destination are the belief's.
- `bestG`: the fewest actions found so far from the root;
- `h`: the heuristic, computed once, when the state is discovered;
- the parent state and action of the best path found so far;
- a discovery number (0 for the root, then 1, 2, 3 … in order of discovery).

There is also a set of **closed** states, those already expanded.

**The queue.** It holds entries (*f* = *g* + *h*, *h*, entry number, state). Entries are ordered by *f*, then *h*, then entry number, smallest first. Entry numbers increase by one each time an entry is added. This is the documented tie-breaking order.

**Steps:**
1. Discover the root (*g* = 0) and add its entry.
2. Take the first entry from the queue.
   - **Skip it if it is outdated**: its *g* is no longer the state's `bestG` (a shorter path was found later), or the state is already closed.
3. **If the state satisfies `goalTest`, return Found** with the best path to it. The goal is accepted when it is taken from the queue, not when it is first generated.
4. **Otherwise, close the state and expand it.** For each action, in the order North, East, South, West:
   - If the prediction is not cached and `remaining() <= reserve`, return **OutOfBudget**.
   - Otherwise, predict through the step predictor.
   - Skip the result if its outcome is Blocked or InvalidState.
   - Skip it if its object is on a believed dead square.
   - Let *g′* = *g* + 1. If the successor was already discovered and *g′* ≥ its `bestG`, skip it.
   - Otherwise, the successor is **new or improved**:
     - if it is new, discover it (compute *h* and give it a discovery number);
     - if it was closed, **reopen it** (remove it from the closed set);
     - set its `bestG`, parent and action to the new path, and add a new queue entry.
     - Any older entry for the state becomes outdated, and step 2 skips it.
5. If the queue is empty, return **Exhausted**.

**Shortest plans.** When nothing is overridden, the heuristic of 3.3 is **admissible**, and also **consistent**: one action never lowers it by more than 1.
- **Why consistent:**
  - a move changes the agent–object distance by at most 1;
  - a real push from *q* to *q* + *d* is also a relaxed push, so `pushDistance` falls by at most 1;
  - next to the object, the walking term is 0 both before and after a push.
- **Result:** with a consistent heuristic, a state taken from the queue already has its shortest *g*, so a plan returned as Found is a **shortest** plan.
- **Overridden cells.** With a "not dead" override (tests only, F4 in Stage 4), the heuristic may not be consistent. Reopening then still keeps the search correct.
- **Checked by brute force:** consistency held on all 37,571 transitions sampled from 400 random layouts.
- **Checked by the reference implementation:** on 3,000 random layouts, the plan lengths equalled those found by an exhaustive breadth-first search in **every** case. A planner that accepted the goal when it was generated, without improving paths, returned a longer plan in 86 of them (PL2).

**The plan.** A plan is the best path's list of (action, expected state).

**Fallback.** After OutOfBudget or Exhausted, choose the discovered **non-root** state with the smallest (*h*, `bestG`, discovery number) whose *h* is finite. Its best path becomes the plan. If there is none, there is **no action**.

**Goal tests and heuristics used:**
- **Main goal:** the object is on the destination. Heuristic: 3.3.
- **Active-test goal** (3.8): the agent is on one of the test cells. Heuristic: the smallest manhattan distance from the agent to a test cell, or infinite if the object is on a dead square. This is also consistent, so the walk is a shortest one.

### 3.6 C1's decision step [11.3]

**C1's state.** C1 holds:
- the belief;
- the history;
- the dead-square map and its override lists;
- the agent's action model (3.8);
- the current plan and what kind it is (goal, fallback or test);
- the committed prediction from the last step, together with the belief before that action and the corrections it tests;
- the correction records;
- the mismatch records;
- any unfinished V1 work;
- the safe-stop count and the decision-step count.

All of these are plain structs and standard containers, so tests can build C1 in any state directly.

**At each decision step *t*, given observation *o_t*:**

1. **Start the step.** Create a new budget of B, clear the cache, and add *o_t* to the history (*o_1* is the first entry).
2. **Check the committed prediction from *t*−1, if there is one.**
   - **Footprint.** Compute the footprint from the belief **before** that action, the action, and *o_t*. Compare.
   - **Log the step line.** Write it for decision step *t*−1 (5.3).
   - **Corrections this prediction was testing:** **VERIFIED** if it matched, **REJECTED** if not. A rejected correction's old value is restored (3.8).
   - **If it matched,** apply the footprint update.
   - **If it did not match,** add a mismatch record and investigate (3.7).
3. **Retry deferred work** *(v1.1)*.
   - A deferred investigation (3.4) is repeated first. If it must be deferred again, C1 takes a safe stop (`budget too small for verification`) and the step ends.
   - A waiting trap V2 test is then attempted (3.8).
   - Finally, unfinished V1 work continues (3.8).
4. **Fault activation point.** It does nothing in Stage 3. It must stay a separate, commented step.
5. **Choose an action.** If V1 work is unfinished, use **cautious mode** (3.9). Otherwise:
   1. **Active test.** If no test plan is in progress, and some wall correction is CONSISTENT and became CONSISTENT at least T_v decision steps ago, try to plan an active test (3.8). If one is found, it becomes the plan, and C1 logs `Plan: active test of correction N (K actions)`.
   2. **Keep the plan?** If a plan exists, predict its next action from the current belief. If the predicted state equals the expected state and does not put the object on a believed dead square, **keep the plan** and go to step 6.
   3. **Otherwise, search** (3.5) with the main goal.
      - **Found:** this becomes the plan. Log `Plan: search found a plan of K actions`.
        - *(v1.2)* If the plan is **empty**, the belief already shows the object on the destination although the run has not ended, so the belief is wrong about the object. C1 takes a safe stop with reason `belief shows goal reached`.
      - **Exhausted:** first handle a possible **consequence surprise** (3.7 B). Then use the fallback.
      - **OutOfBudget:** use the fallback.
      - **Using the fallback:** log `Plan: fallback toward the best state found (K actions)`. If the fallback has **no action**, take a **safe stop** (3.9) with reason `budget too small to plan` (after OutOfBudget) or `no safe action` (after Exhausted).
6. **Commit.** Predict the chosen (first) action from the current belief; this is a cache hit when possible.
   - If **no prediction can be made** (no budget left), take **no world action**: clear the plan and take a safe stop with reason `no budget for a committed prediction` *(v1.1)*.
   - Otherwise, remove that action from the plan.
   - **V2 bookkeeping.** For each CONSISTENT correction not yet tested, in correction-number order and while budget remains, compute the **alternative prediction** (3.8). If the alternative and the committed prediction differ in outcome, agent position or object position, this step is that correction's **V2 test**.
   - **Remember** the committed prediction, the belief before the action, and the corrections it tests.
7. **Act.** Return the action. In the case of step 5, return a safe stop instead.

**The final check.** When a run ends after a world action, C1 still checks the last committed prediction against the final observation (step 2 only). The last step is therefore logged and checked.

### 3.7 Investigation [11.4]

**A. Mismatch between prediction and observation.** Here *o_t−1* is the second-last history entry (the observation before the action), and *o_t* is the last.

1. **Anomaly check.** If the believed agent before the action differs from `o_t−1.state.agent`, record an **anomaly** (a possible bug) in the mismatch record.
2. **Belief candidates** (evidence lookups):
   - **Walls.** For each footprint cell whose believed wall value (before the action) differs from *o_t−1*, add a candidate `wall(c)` with *o_t−1*'s value.
   - **Object.** If the believed object or the recorded object (in *o_t−1*) is in a footprint cell, and they differ, add a candidate `object` with the value `o_t−1.state.object`. That value may lie outside the footprint: investigation may read any part of the history.
   - Suspected source: **S2**.
3. **If there are belief candidates, verify them together.**
   - **The corrected belief.** This is the belief before the action, with **all** belief candidates applied.
   - **History check.** Run each candidate's history check (3.8, V1).
   - **Re-prediction.** Make **one** variant prediction of the action from the corrected belief with the **current model**. Compare its footprint with *o_t*, using the footprint computed from the corrected belief.
     - This is an optional evaluation. If it would break the reserve, the investigation is deferred (3.4). It is repeated at the next step without logging its investigation lines again, and nothing is updated in the meantime.
   - **If every check passes,** each candidate becomes its own **CONSISTENT** correction (3.8).
   - **If any check fails,** log `investigation: no candidate passed V1 (source S5); unexplained update`. No correction record is created.
   - **In both cases,** apply the footprint update with *o_t*.
4. **If there are no belief candidates,** the belief before the action matched the record, so the action model for *a* is suspected (source **S1**).
   - Apply the footprint update with *o_t* **now**. This is provisional: if V1 later fails, it is logged as an unexplained update.
   - Start the V1 work for `model(a)` (3.8).
5. **Log the lines** in the order given in 5.3: first one investigation line per candidate (or the model-suspicion line), then one line per correction, or the unexplained-update line. Investigation lines are written before V1 runs.
6. **C1 never uses S3 or S4.**

**B. Consequence surprise and consequence mismatch.** This applies when the main search returns **Exhausted** and the object's cell *p* in the belief is neither dead nor the destination.

1. Log `consequence surprise: search exhausted with the object at (x, y)`.
2. **If `localCheckSaysNotDead(belief, map, p)` is true,** log `full-state dead end (dead-square classification consistent; no claim)`. Nothing else happens.
3. **Otherwise,** check the history: no recorded observation may show the object at *p* with a later observation showing it on the destination.
   - **If the history agrees,** log `consequence mismatch: (x, y) violates the dead-square rule (source S1)`, and create the correction `trap(p)` = dead (3.8).
4. **Stage 3 scope.** An exhaustive search must finish within one decision step's budget. It is **not** continued over several steps in Stage 3 (D-024).

### 3.8 Corrections and verification [11.5]

**A correction record holds:**
- an identifier (1, 2, 3 …, per run);
- the element (wall cell, object, action model for action *a*, or trap cell);
- the new value and the old value;
- the status (CONSISTENT, VERIFIED or REJECTED);
- the decision step at which it became CONSISTENT;
- whether it has been tested.

**Applying a correction:**
- **Wall:** set the belief wall, rerun the dead-square analysis, and clear the cache.
- **Object:** set the believed object position.
- **Action model:** mark action *a* as restored to the **standard rule** (`applyAction`) in the agent's action model, and clear the cache. In Experiment 1 the world rules are fully known, so the only candidate correction of `model(a)` is "use the standard rule for *a*".
- **Trap:** add *p* to the applied-corrections override list ("dead"), rerun the analysis, clear the cache, then run the trap V2 test.

**Reverting a correction** (when it is REJECTED):
- **Wall:** restore the old value.
- **Action model:** remove the restoration.
- **Trap:** remove the override.
- **Object:** the record is marked REJECTED, but the belief is not changed, because the old value no longer refers to the present.

**The agent's action model.** It is built as an `ActionModel` that uses the standard rule for restored actions and the **base model** for all others. The base model is `applyAction`, unless a test supplies another.

**V1 checks:**

| Element | History check | Prediction work |
|---|---|---|
| `wall(c)` | *c*'s wall value equals the new value in **every** recorded observation (walls never change). | Shared re-prediction (3.7 A.3), 1 evaluation |
| `object` | The new value equals the object's position in *o_t−1*. Earlier positions are **not** compared, because the object moves. | Shared re-prediction, 1 evaluation |
| `model(a)` | For **each** history entry *k* whose `lastAction` is *a*: predict *a* from `history[k−1].state` with the **standard rule**. The result must equal `history[k].state` and `history[k].lastOutcome`. | One variant evaluation per such entry, in history order |
| `trap(p)` | The local check fails at *p*, and the history check of 3.7 B.3 passes. | None (free) |

**Unfinished V1 work.** Model(a) replays are optional evaluations. If the next replay would break the reserve (`remaining() <= reserve`), the work stays pending. It continues at the next decision step, and C1 is in cautious mode until it finishes.
- **If a replay fails:** log `no candidate passed V1 (source S5); unexplained update`, and drop the work.
- **If every replay passes:** apply the correction.

**Which action model each verification prediction uses.** This is required, and tested by instrumented models (VF8):

| Prediction | Model used |
|---|---|
| Re-prediction for belief candidates (3.7 A.3) | the agent's **current** action model |
| V1 replays for `model(a)` | the **standard rule** (`applyAction`) |
| Alternative prediction for a wall or object correction (V2 bookkeeping) | the agent's **current** action model, on the belief with the element set to its **old** value |
| Alternative prediction for a `model(a)` correction | the **old** model for *a* (the one in use before the correction) |
| Trap V2 test | the agent's **current** action model |
| Committed predictions, plan checks and the planner | the agent's **current** action model |

**V2 (independent test):**
- **Alternative predictions.** For a committed action *a* from the agent's cell, let c1 and c2 be the cells one and two steps ahead in direction *a*.
  - **Wall correction at cell *c*:** an alternative is computed only if *c* is c1 or c2.
  - **Object correction:** only if the old or the current believed object position is c1 or c2.
  - **`model(b)` correction:** only if *b* is *a*.
  - **Hypothetical states.** An alternative belief that is not a valid world (`isValidWorld`) is not predicted.
- **Testing.** If the alternative differs from the committed prediction in outcome, agent position or object position, the step tests that correction. The next check then makes it VERIFIED (on a match) or REJECTED (on a mismatch).
- **Trap V2:**
  - **Hypothetical states.** For each direction *d* (N, E, S, W) with *p* − *d* believed free, build the hypothetical state: the belief's walls and destination, the **agent at *p* − *d*** and the **object at *p***.
  - **Validity.** Each such state must be a valid world (`isValidWorld`). If one is not, skip that direction and log an anomaly.
  - **Prediction.** Predict the push *d* with the current model. This is a variant evaluation, so at most 4 in total.
  - **Budget.** The test is made only if all its predictions can be made while keeping the reserve (`remaining() − k >= reserve` for *k* valid directions). Otherwise it waits for a later step (3.6, step 3), and the correction stays CONSISTENT meanwhile.
  - **VERIFIED** if no predicted push moves the object onto a cell that is **not** dead. Otherwise **REJECTED**, and the correction is reverted.
- **Active test (wall corrections only in Stage 3, D-023):**
  - **Test cells.** These are the cells *n* = *c* − *d*, for each direction *d*, where *n* is believed free and is not the believed object cell.
  - **Walking there.** C1 searches (3.5) with the active-test goal and heuristic.
  - **The test action.** If a walk is found, the plan is the walk followed by the move *d* from the reached cell into *c*. That move's prediction comes from the end state, and is made only if it keeps the reserve (or is cached).
  - The V2 bookkeeping of the commit step marks the test automatically.
  - **If no walk is found within the budget,** no test happens this step.
- **Object and model(a) corrections** are verified only when a later committed prediction discriminates (no active test in Stage 3).

### 3.9 Cautious mode and safe stops [11.6]

**Cautious mode** applies while V1 work for `model(a)` is unfinished.
- For each action, in the order N, E, S, W, predict it with the **current** model, which may be cached, and with the **standard rule**, as a variant.
- The first action whose two predictions agree in outcome, agent position and object position, and whose result does not put the object on a believed dead square, is chosen.
- C1 logs `Plan: cautious mode (investigation unfinished)`.
- Each check is made only if it keeps the reserve. Before checking an action, C1 needs 1 evaluation for the standard-rule variant, plus 1 more if the current-model prediction is not cached.
- If no action qualifies, or checking must stop to keep the reserve, C1 takes a **safe stop**. The committed prediction is the chosen action's current-model prediction, which is already cached.

**A safe stop:**
- takes no world action;
- is a decision step;
- uses that step's budget (any unfinished V1 work has already continued in step 3);
- increments the safe-stop count;
- is logged as `Step t: safe stop (reason)`;
- has no committed prediction, so the next observation is the same state, with no last action and no last outcome.

### 3.10 The run loop and outcomes [Phase A 5.1]

`runSingleAgent(trueInitialState, agent, limits, log)`:

1. **Start.** Create an `Environment` from the true state.
   - *(v1.2)* If the object is **already on the destination**, the run ends at once: **Success** after 0 world steps, with no decision made.
   - Otherwise, the first observation is the true state, at decision step 1.
2. **Repeat:**
   1. **Ask the agent** for a decision at the current observation.
   2. **If it is an action,** apply it with `Environment::step`, and build the next observation from the result.
      - **If the object is now on the destination:** final check (3.6), then **Success**.
      - **If world steps = T_max:** final check, then **Step limit**.
   3. **If it is a safe stop:**
      - **If the safe-stop count = S_max:** **Stopped — unresolved**.
      - **Otherwise:** the next observation repeats the state, with no last action.
3. **Return the result:** the outcome, world steps, safe stops, mismatches, corrections (and how many were VERIFIED), and total model evaluations.

### 3.11 C0, the naive agent [11.8]

- **Same components.** C0 uses the same step predictor (with limit B), planner, heuristic and dead-square analysis.
- **Planning.**
  - **When:** at decision step 1, and again only when its plan is used up.
  - **How:** it searches from its **own** belief with reserve 0. Found gives a plan; otherwise it uses the fallback.
- **No action.** If there is no plan and no fallback action, C0 takes the first action (N, E, S, W) that its model predicts is Moved or Pushed. If every action is predicted Blocked, it takes North. C0 has no safe stops (D-026).
- **Executing.** C0 takes the next plan action, and sets its belief to that action's **stored** expected state. There is no new prediction, so it costs nothing.
- **It never reads any observation after the first.**
- It makes no checks, investigations, corrections, claims or safe stops.
- **The run loop is the same** (3.10). C0 can end only with Success or Step limit.

### 3.12 Settings

**From Phase C.** B (budget per step), S_max (safe-stop limit), T_max (maximum world steps) and T_v (active-test delay) are protocol numbers, set in **Phase C**. Stage 3 code takes them as **parameters** and never hard-codes them.

**For the demonstrations only,** the illustrative values B = 200, S_max = 5, T_max = 100 and T_v = 3 are used, and the program says so.

### 3.13 Information access (no unfair restriction on C1) [11.1]

- C1 receives **every** observation **in full** (3.1), and keeps all of them.
- **During an investigation,** C1 may read **any** cell or position of the current observation and of every recorded observation (3.7 A.2 reads the object's position outside the footprint).
- **What the footprint rule governs** is **when** C1 compares beliefs it would otherwise not re-check. It never governs **what** C1 receives. Without faults, C1's belief equals the true state after every step (test C1-3).
- **Tests check all of this** (C1-4, IV-8).

---

## 4. Code structure

### 4.1 New and changed files

All new code is in namespace `ai_hologram`, in a new folder `src/agent/`, compiled into `ai_hologram_core`.

```
src/
├── main.cpp                         (changed) header lines; also runs the agent demos
├── core/project_info.cpp            (changed) currentStage() returns 3
├── agent/
│   ├── observation.h                NEW  Observation (3.1)
│   ├── footprint.h / .cpp           NEW  footprint, comparison, two-prediction footprint, update (3.2)
│   ├── dead_squares.h / .cpp        NEW  DeadSquareMap, analysis, overrides, local check (3.3)
│   ├── heuristic.h / .cpp           NEW  heuristic (3.3)
│   ├── step_predictor.h / .cpp      NEW  per-step budget, cache, counts (3.4)
│   ├── planner.h / .cpp             NEW  A* search, fallback (3.5)
│   ├── records.h                    NEW  enums and records: source labels, statuses, elements,
│   │                                     mismatch and correction records
│   ├── agent_model.h / .cpp         NEW  base model + restored actions → ActionModel (3.8)
│   ├── investigation.h / .cpp       NEW  belief candidates, model suspicion, consequence checks (3.7)
│   ├── verification.h / .cpp        NEW  V1 checks, alternative predictions, trap V2, active-test targets (3.8)
│   ├── single_agent.h / .cpp        NEW  C1 state and decision step (3.6, 3.9)
│   ├── single_agent_choose.cpp      NEW  C1 action choice (split for file length)
│   ├── naive_agent.h / .cpp         NEW  C0 (3.11)
│   ├── run_loop.h / .cpp            NEW  runSingleAgent and RunResult (3.10)
│   └── agent_log.h / .cpp           NEW  exact log-line text (5.3)
└── demo/
    └── agent_demo.h / .cpp          NEW  the two C1 demonstrations

tests/
├── test_suites.h, test_main.cpp     (changed) new groups
├── test_project_info.cpp            (changed) P2 checks currentStage() == 3
├── test_footprint.cpp               NEW
├── test_dead_squares.cpp            NEW
├── test_step_predictor.cpp          NEW
├── test_planner.cpp                 NEW
├── test_investigation.cpp           NEW
├── test_verification.cpp            NEW
├── test_single_agent.cpp            NEW
└── test_naive_agent.cpp             NEW
```

**Also changed:**
- `CMakeLists.txt`;
- `README.md` (status paragraph only);
- `docs/DECISIONS.md` (D-019 to D-027);
- the handoff-notes file;
- the prompt files in `docs/copilot_prompts/`.

**If a file would exceed about 200 lines,** split it, with the names chosen in the sub-stage prompt. A file name not listed above needs the owner's approval before it is created.

### 4.2 Interfaces fixed by this plan

Copilot may add private helpers. The public names and meanings below are fixed:
- **Footprint:** `computeFootprint`, `compareFootprint`, `computeFootprintForTwoPredictions`, `applyFootprintUpdate`.
- **Dead squares:** `DeadSquareMap`, `computeDeadSquares`, `localCheckSaysNotDead`, `heuristic`.
- **Prediction:** `StepPredictor`, with `startStep()`, `predict(state, action)` (cached), `predictVariant(state, action, model)` (never cached), `remaining()`, `usedThisStep()`, `usedTotal()` and `clearCache()`.
- **Planning:** `search`, returning a `SearchResult { kind (Found / OutOfBudget / Exhausted); plan; discovered states }`, and `fallback`.
- **Agents:**
  - `SingleAgent` (C1), with `decide(const Observation&)` returning a `Decision` (an action, or a safe stop with a reason), and `finalCheck(const Observation&)`;
  - `NaiveAgent` (C0), with the same `decide`.
- **Running:** `runSingleAgent` and `RunResult`.

Functions stay under about 40 lines. Beginner comments explain (9.1):
- `std::priority_queue`, or the chosen ordered container, for the open list;
- why the cache key includes the belief walls;
- what a variant prediction is;
- why the fault activation point is empty;
- why every successor state must come from the predictor.

---

## 5. Expected program output

### 5.1 Header lines
```
AI Hologram -- Stage 3
Grid world with a competent single agent (C1) and scripted demonstrations.
No faults are injected, no two-agent reasoning exists, and no experiment is run.
```

### 5.2 Earlier demonstrations
The Stage 1 demonstration and the Stage 2 prediction demonstrations follow **unchanged**, from the blank line before `Initial world:` to `How beliefs are corrected is decided in Stage 3.`. The Stage 2 code is not changed (Section 2), so its last line stays as it is: it describes Stage 2.

### 5.3 Agent log lines (exact formats)

**Planning** (written when a decision is made, before that step's line):
- `Plan: search found a plan of K actions`
- `Plan: fallback toward the best state found (K actions)`
- `Plan: active test of correction N (K actions)`
- `Plan: cautious mode (investigation unfinished)`

**Steps:**
- `Step t: <Action> | predicted <Outcome>, observed <Outcome> | match` (or `| MISMATCH`). This is written when the step's observation is checked.
- `Step t: safe stop (no safe action)`
- `Step t: safe stop (investigation unfinished, no cautious action)`
- `Step t: safe stop (budget too small to plan)` *(v1.1)*
- `Step t: safe stop (budget too small for verification)` *(v1.1)*
- `Step t: safe stop (no budget for a committed prediction)` *(v1.1)*
- `Step t: safe stop (belief shows goal reached)` *(v1.2)*

**Lines after a step line.** These start with two spaces, a hyphen and a space, and appear in this order:
1. **V2 results:** `  - correction N: VERIFIED` or `  - correction N: REJECTED`
2. **Investigation lines:**
   - `  - investigation: wall at (x, y) believed free, recorded blocked (source S2)` (or `believed blocked, recorded free`)
   - `  - investigation: object believed at (x, y), recorded at (x, y) (source S2)`
   - `  - investigation: belief matched the record; action model for <Action> suspected (source S1)`
   - `  - anomaly: believed agent position differs from the record`
   - `  - investigation: budget too small for verification; retried next step` *(v1.1)*
3. **Corrections**, or the unexplained update:
   - `  - correction N: wall at (x, y) set to blocked: CONSISTENT` (or `set to free`)
   - `  - correction N: object position set to (x, y): CONSISTENT`
   - `  - correction N: action model for <Action> restored to the standard rule: CONSISTENT`
   - `  - investigation: no candidate passed V1 (source S5); unexplained update`

**Consequence lines** (written during planning, before the `Plan:` line):
- `  - consequence surprise: search exhausted with the object at (x, y)`
- `  - full-state dead end (dead-square classification consistent; no claim)`
- `  - consequence mismatch: (x, y) violates the dead-square rule (source S1)`
- `  - correction N: (x, y) classified as a dead square: CONSISTENT`, then its V2 line (written when the trap V2 test is made, possibly at a later step)
- `  - anomaly: invalid hypothetical state skipped` *(v1.1)*

**At the end of a run:**
```
Result: <Success | Step limit | Stopped -- unresolved> after W world steps, S safe stops
Mismatches: M
Corrections: C (V verified)
Model evaluations used: E
```

### 5.4 The two C1 demonstrations
- **Demo 1:** the Stage 1 demo layout, with a correct belief.
- **Demo 2:** the layout below, which is the demo layout with a wall added at (4, 3). The agent's starting belief is the same layout **without** that wall: a hand-written wrong belief, labelled as such, and not fault injection.

```
##########
#........#
#........#
#..A#O..D#
#........#
#........#
#...##...#
#........#
#........#
##########
```

The C1 demos run in `runSingleAgent`. Demo 2 constructs C1 with the hand-written starting belief, which is the only difference from demo 1. This matches Stage 2's practice of writing wrong beliefs by hand.

### 5.5 Exact expected output of the `ai_hologram` program

```
AI Hologram -- Stage 3
Grid world with a competent single agent (C1) and scripted demonstrations.
No faults are injected, no two-agent reasoning exists, and no experiment is run.

Initial world:
##########
#........#
#........#
#..A.O..D#
#........#
#........#
#...##...#
#........#
#........#
##########

Step 1: East -> Moved
Step 2: East -> Pushed
Step 3: East -> Pushed
Step 4: East -> Pushed
Step 5: East -> Blocked

Final world:
##########
#........#
#........#
#......A*#
#........#
#........#
#...##...#
#........#
#........#
##########
Goal reached: yes
Steps taken: 5

Prediction demo 1: correct belief
Step 1: East | predicted Moved, observed Moved | match
Step 2: East | predicted Pushed, observed Pushed | match
Step 3: East | predicted Pushed, observed Pushed | match
Step 4: East | predicted Pushed, observed Pushed | match
Step 5: East | predicted Blocked, observed Blocked | match
Mismatches: 0
Model evaluations used: 5

Prediction demo 2: belief wrongly includes a wall at (6, 3)
Step 1: East | predicted Moved, observed Moved | MISMATCH
  - wall at (6, 3): predicted blocked, observed free
Step 2: East | predicted Pushed, observed Pushed | match
Step 3: East | predicted Pushed, observed Pushed | match
Step 4: East | predicted Pushed, observed Pushed | match
Step 5: East | predicted Blocked, observed Blocked | match
Mismatches: 1
Model evaluations used: 5
Note: in these demos the belief is replaced by the observation after every step.
How beliefs are corrected is decided in Stage 3.

Single-agent demonstrations use illustrative settings: budget 200 per step,
safe-stop limit 5, step limit 100, active-test delay 3 (not the experiment's values).

C1 demo 1: correct belief
Plan: search found a plan of 4 actions
Step 1: East | predicted Moved, observed Moved | match
Step 2: East | predicted Pushed, observed Pushed | match
Step 3: East | predicted Pushed, observed Pushed | match
Step 4: East | predicted Pushed, observed Pushed | match
Result: Success after 4 world steps, 0 safe stops
Mismatches: 0
Corrections: 0 (0 verified)
Model evaluations used: 19

C1 demo 2: belief wrongly has no wall at (4, 3)
Plan: search found a plan of 4 actions
Step 1: East | predicted Moved, observed Blocked | MISMATCH
  - investigation: wall at (4, 3) believed free, recorded blocked (source S2)
  - correction 1: wall at (4, 3) set to blocked: CONSISTENT
Plan: fallback toward the best state found (10 actions)
Step 2: North | predicted Moved, observed Moved | match
Step 3: East | predicted Moved, observed Moved | match
Step 4: East | predicted Moved, observed Moved | match
Plan: active test of correction 1 (2 actions)
Step 5: West | predicted Moved, observed Moved | match
Step 6: South | predicted Blocked, observed Blocked | match
  - correction 1: VERIFIED
Plan: search found a plan of 12 actions
Step 7: East | predicted Moved, observed Moved | match
Step 8: South | predicted Pushed, observed Pushed | match
Step 9: South | predicted Pushed, observed Pushed | match
Step 10: West | predicted Moved, observed Moved | match
Step 11: South | predicted Moved, observed Moved | match
Step 12: East | predicted Pushed, observed Pushed | match
Step 13: East | predicted Pushed, observed Pushed | match
Step 14: East | predicted Pushed, observed Pushed | match
Step 15: South | predicted Moved, observed Moved | match
Step 16: East | predicted Moved, observed Moved | match
Step 17: North | predicted Pushed, observed Pushed | match
Step 18: North | predicted Pushed, observed Pushed | match
Result: Success after 18 world steps, 0 safe stops
Mismatches: 1
Corrections: 1 (1 verified)
Model evaluations used: 414
Note: demo 2's wrong belief is hand-written; it is not fault injection.
```

This output was produced by a **reference implementation of this plan, written by Claude in Python** to check the design before review. That implementation is not project code and is not committed.

The model-evaluation counts (**19 and 414**) are part of the expected output **on purpose**. They check that budget accounting, the cache and the planner follow 3.4–3.5 exactly.

*(v1.1)* They were recalculated after the planner was corrected. v1.0's figures (17 and 401) came from the planner that accepted the goal when generated, and are obsolete. The step lines of both demos are unchanged.

The reference implementation was also run on other cases, without faults and with hand-made wrong beliefs:
- **Fault-free runs:** across 300 random layouts, C1 (with B = 2000) reached the goal in **all 160** that were solvable. The 140 unsolvable layouts ended correctly, without success. In a separate set of 200 fault-free runs, C1's belief equalled the true state after every check, with no mismatches.
- **Wrong action models:** were noticed when used, restored, and verified.
- **A wrongly "not dead" corner:** produced a consequence mismatch and a verified trap correction.
- **A full-state dead end on a non-dead square:** produced a surprise but **no claim**.

These are checks of the design, not experimental results.

### 5.6 Things Stage 3 shows but does not decide [OBSERVATION]
1. **The budget matters.** In the reference runs (with the corrected planner), with B = 200, C1 still reached the goal in 158 of the 160 solvable layouts. In the other 2, the fallback went back and forth until the step limit. The value of B is a Phase C decision, checked by pilots (Phase A 9.2), never tuned to favour any condition.
2. **F4 at tight budgets.** Because of D-024, a missed dead square can be noticed only if an exhaustive search finishes within one step's budget. This limitation is kept in every report.
3. **A faulty action model is noticed only when used.** In one reference case, a hand-made model that pushed east wrongly led C1 to believe the goal was unreachable. C1 then never pushed east, so the fault was never noticed. This is the expected behaviour under Phase B 11.1 ("checks are triggered by use"), and is reported as a property of the design (Phase B 12.7).

---

## 6. Required correctness tests

**General rules:**
- Every test world, whether constructed or parsed, **must be valid** (`isValidWorld`), unless the test is specifically about invalid states.
- Wrong beliefs and wrong models are written by hand.
- Test IDs are used in descriptions, as in Stages 1–2.
- **Instrumented models** are `ActionModel` functions that count, or record, their calls, used to check which model was called and how often.

**Coverage.** The tests cover every item of Phase B 12.9, plus ChatGPT's list for this plan (Section 6.10).

### 6.1 Project info
- P1. `projectName()` returns `"AI Hologram"`.
- P2. `currentStage()` returns `3`.

### 6.2 Footprint (`test_footprint.cpp`) — Phase B 12.9 items 1–3
- FP1. **Invariant:** on the demo layout, a fixed hand-written sequence of 30 actions (including blocked moves and pushes) applied with footprint updates keeps the belief equal to the true state after every step.
- FP2–FP9. **Each row** of the Phase B 10.5 disagreement table gives exactly the listed cells (c2 included or not) and the listed differences:
  - FP2: Moved/Moved;
  - FP3: wall missing from the belief, Blocked;
  - FP4: wall wrongly believed, Moved;
  - FP5: predicted Blocked, observed Pushed;
  - FP6: object elsewhere, Pushed;
  - FP7: object elsewhere, push blocked (rule iii);
  - FP8: predicted Pushed, Moved;
  - FP9: different agent positions.
- FP10. After a **matching** check, the update changes no belief value that was in the comparison, and changes nothing outside the footprint.
- FP11. The two-prediction footprint includes c2 if **either** belief has the object at c1, or either predicts Pushed. The rule is symmetric: swapping the two predictions gives the same cells.
- FP12. A footprint cell outside the grid counts as a wall, and reading it never goes outside the array.

### 6.3 Dead squares and heuristic (`test_dead_squares.cpp`)
- DS1. **Dead squares:** in a small layout, a corner that is not the destination is dead. The destination is never dead, and walls are never dead.
- DS2. **Distances:** on the demo layout, `pushDistance` is 3 at (5, 3) and 0 at (8, 3), with other hand-checked values.
- DS3. **Dead square versus full-state dead end:**
  - In a layout whose object region is sealed off from the agent, the object's cell is **not** dead (it can reach the destination by relaxed pushes).
  - Yet the full search with a large budget returns **Exhausted**.
  - This shows that dead squares are only part of the full-state dead ends.
- DS4. **Local check:**
  - For a true dead square whose status is overridden to "not dead", `localCheckSaysNotDead` returns false.
  - For a non-dead cell it returns true.
  - For the destination it returns true.
- DS5. **Overrides:** a "not dead" override changes only the listed cell, and no other cell's status. The "dead" corrections are applied after it.
- DS6. **When the analysis runs:** it runs only when the believed walls or the override lists change (`analysisRuns`).
- DS7. **No hidden model use:** the analysis and the heuristic make **no** model evaluations (an instrumented model is never called, and the budget is unchanged).
- DS8. **Heuristic values:**
  - the hand-computed values for given states;
  - infinite when the object is on a dead square;
  - the manhattan fallback for an overridden cell;
  - `heuristicCalls` counts correctly.

### 6.4 Step predictor (`test_step_predictor.cpp`) — Phase B 12.9 item 8
- SP1. **Cost:** an uncached prediction costs exactly one evaluation. Repeating it in the same step costs nothing and gives the same result.
- SP2. **Clearing the cache:** the cache is cleared by `startStep()`, and by a change of belief walls, overrides, or model.
- SP3. **Carry-over:** unused budget is not carried over. Each step starts with exactly B.
- SP4. **Exhausted budget:** predictions return nothing and change nothing.
- SP5. **Variant predictions** never read or fill the cache, and always cost one evaluation.
- SP6. **Model calls:** with an instrumented model, the number of model calls equals the number of evaluations charged. No prediction happens without being charged.
- SP7. **Several pending V2 corrections** *(v1.1)*:
  - **Setup:** C1 starts on the demo layout with **three** CONSISTENT wall corrections pending (reserve = 4), run once for each budget B from 1 to 11.
  - **For every B:** no step charges more than B, every world action has a committed prediction, and the reserve is never used by optional evaluations.
  - **Small budgets:** at B ≤ 4, C1 takes no world action and safe-stops (`budget too small to plan`).
  - The reference implementation confirmed all of these.

### 6.5 Planner (`test_planner.cpp`)
- PL1. **The demo plan:** on the demo layout, from a correct belief, `search` returns Found with plan East, East, East, East, using **16** evaluations.
- PL2. **Shortest plans** *(extended in v1.1)*:
  - In small layouts with known shortest solutions, Found plans have the shortest length.
  - **First route is not the shortest.** The layout below is valid, and its shortest solution has **9** actions: West, West, West, North, West, North, East, East, East. On it, a planner that accepted the goal when first generated, without improving paths, returned 11 actions. The A* of 3.5 must return a 9-action plan (this exact plan, under the tie-breaking of 3.5).
    ```
    ##########
    #....D##.#
    #.O......#
    #....A#..#
    #.#......#
    #.....##.#
    #........#
    #........#
    #........#
    ##########
    ```
  - **Better path to a known state.** In a hand-built case, a state is first discovered through a longer route. The test checks that its `bestG` and parent are replaced when a shorter route is found, and that the outdated queue entry is skipped.
- PL3. **Dead squares:** no discovered state ever has its object on a believed dead square.
- PL4. **Running out of budget:** with a small budget and a reserve, `search` returns OutOfBudget, and charged evaluations never reduce `remaining()` below the reserve.
- PL5. **Fallback:** the fallback picks the discovered non-root state with the smallest (*h*, best *g*, discovery number). With no such state, it returns no action.
- PL6. **Exhaustion:** a sealed layout returns Exhausted.
- PL7. **Active-test goal:** it finds a shortest walk to a test cell.
- PL8. **Determinism:** the same inputs give the same plan and the same evaluation count. Ties follow the N, E, S, W order.
- PL10. **Object already on the destination** *(v1.2, kept at ChatGPT's request)*:
  - From a valid state whose object is already on the destination, and whose agent is several cells away (so the heuristic is not zero; it is 4 in the reference case), `search` returns **Found** with an **empty** plan and uses **0** evaluations.
  - This holds because the root is taken from the queue first, and the goal is tested before any expansion. The non-zero walking term does not mislead the planner.
- PL9. **Only budgeted states:** every state the planner examines came from a budgeted evaluation, apart from the root. An instrumented model's calls equal the charged evaluations.

### 6.6 Investigation (`test_investigation.cpp`) — Phase B 12.9 item 4
- IV1. **Wall missing from the belief:** the belief has no wall at c1, but there is one. The candidate is `wall(c1)` = blocked, source S2.
- IV2. **Wall wrongly believed at c2:** a push was predicted Blocked but observed Pushed. The candidate is `wall(c2)` = free.
- IV3. **Object wrongly believed at c1:** the candidate is `object`, with *o_t−1*'s object position, source S2.
- IV4. **Object at c1 but not believed there** (observed Pushed): the candidate is `object`, with *o_t−1*'s position.
- IV5. **Wrong action model:**
  - the belief matches the record, so the action model for *a* is suspected (S1);
  - the footprint is updated at once (provisionally);
  - V1 work starts.
- IV6. **Two belief candidates at once** (a wrong wall at c2 and a wrong object): they are verified together, giving two CONSISTENT corrections.
- IV7. **No candidate passes V1:** the result is S5, an unexplained update is logged, **no** correction record is created, and the belief is brought into line with *o_t*.
- IV8. **Information access:** the object candidate's value is read from *o_t−1* even when it lies **outside** the footprint.
- IV9. **Anomaly:** a believed agent position that differs from the record is flagged as an anomaly.

### 6.7 Verification (`test_verification.cpp`) — Phase B 12.9 items 5–6, 11–12
- VF1. **Object V1 is time-aware:** a correct object correction passes, even though the object was at other positions in earlier observations.
- VF2. **Wrong object value:** an object value that differs from *o_t−1* fails V1.
- VF3. **Wall V1:** it checks **every** recorded observation, and fails if any one differs.
- VF4. **Model V1:** it replays **each** recorded transition with action *a*, from that transition's own pre-action state, using the standard rule. It fails if any replay differs.
- VF5. **Model V1 over several steps:** when the budget is short, the work continues over several steps (a replay that would break the reserve pauses it), and C1 is in cautious mode until it finishes.
- VF6. **V2 marking:** a discriminating committed prediction makes the correction VERIFIED on a match. A mismatch makes it REJECTED, and the old wall or model is restored.
- VF7. **When alternatives are computed:** only when the corrected element is relevant (wall *c* is c1 or c2; the object's old or current position is c1 or c2; model *b* = *a*). Otherwise no evaluation is spent.
- VF8. **Which model is used:** instrumented models confirm the table in 3.8. The belief re-prediction uses the current model, model V1 uses the standard rule, a model correction's alternative uses the old model, and the trap V2 test uses the current model.
- VF9. **Trap claims:**
  - **Valid:** a dead square overridden to "not dead", with the object on it, gives a consequence mismatch, a CONSISTENT correction, and VERIFIED by the trap V2 test.
  - **Invalid:** a proposed "dead" status for a cell that is not a dead square fails V1, or is REJECTED by V2.
- VF10. **Trap V2 states:** every hypothetical state is a valid world (`isValidWorld`), and at most 4 evaluations are used. A direction whose hypothetical state would be invalid is skipped and logged as an anomaly.
- VF11. **Full-state dead end:** on the DS3 layout, the exhausted search gives a consequence surprise and the log line `full-state dead end`, but **no claim** and no correction record.
- VF12. **Consequence mismatch:** a correct consequence mismatch is labelled S1.

### 6.8 C1 as a whole (`test_single_agent.cpp`) — Phase B 12.9 items 6, 8, 9
- C1-1. **Demo 1:** the log lines and result match 5.5 exactly (19 evaluations).
- C1-2. **Demo 2:** the log lines and result match 5.5 exactly (414 evaluations, one VERIFIED correction).
- C1-3. **Fault-free belief:** in fault-free runs on three hand-written valid layouts, C1's belief equals the true state after every check, and no mismatch occurs.
- C1-4. **Information access:**
  - every observation C1 receives is identical to the environment's state at that step;
  - C1's history holds every observation, unchanged and in order;
  - C1 never reads `Environment` directly.
- C1-5. **Safe stops:** when no safe action exists, C1 safe-stops. A safe stop is a decision step, uses that step's budget, and is followed by an observation with no last action. Reaching S_max ends the run "Stopped — unresolved".
- C1-6. **Cautious mode:** it chooses only an action whose current-model and standard-rule predictions agree, and safe-stops if there is none.
- C1-7. **Reserve:** every world action has a committed prediction. No optional evaluation (re-prediction, V1 replay, trap V2, cautious check, active-test search, main search) ever leaves less than the reserve.
- C1-8. **Active tests:** one is planned only after T_v decision steps, only for a CONSISTENT wall correction, and only when a walk is found. Its test action is safe.
- C1-9. **Outcomes:** each run outcome (Success, Step limit, Stopped — unresolved) is produced by its own case.
- C1-10. **Final check:** a run that ends after a world action still checks and logs its last step.
- C1-12. **Goal already reached** *(v1.2)*:
  - A run whose initial state already has the object on the destination ends with **Success after 0 world steps**, and the agent is never asked to decide.
  - A C1 whose (hand-written, wrong) belief puts the object on the destination, when it is not there, takes the safe stop `belief shows goal reached`, and never takes a world action.
- C1-11. **Budget too small** *(v1.1)*:
  - When the budget cannot cover a committed prediction, C1 takes **no world action**, and logs the safe stop `no budget for a committed prediction`, or `budget too small to plan`.
  - When an investigation's re-prediction cannot be afforded, it is deferred, logged, and repeated at the next step.

### 6.9 C0 (`test_naive_agent.cpp`) — Phase B 12.9 item 10
- C0-1. **No observations read:** giving C0 different later observations changes nothing it does.
- C0-2. **When C0 plans:** only at step 1 and when its plan is used up. Steps that follow a plan make **no** evaluations.
- C0-3. **No action available:** C0 takes the first action predicted Moved or Pushed, otherwise North.
- C0-4. **Demo 1:** from the demo layout, C0 reaches the goal in 4 world steps.
- C0-5. **C0 makes no records:** it never checks, makes no mismatch or correction records, and never safe-stops.

### 6.10 Coverage of the review requirements

| Requirement (ChatGPT, approval of Phase B) | Tests |
|---|---|
| Dead-square analysis and its conservative limits | DS1–DS8 |
| Dead squares versus general full-state dead ends | DS3, VF11 |
| F4 consequence surprises and mismatches | VF9, VF11, VF12 |
| V1 and V2 verification | VF1–VF10, C1-2, C1-8 |
| Valid hypothetical states during V2 | VF10 (and 3.8: no alternative belief is predicted unless it is valid) |
| The exact action model for each verification prediction | VF8 |
| Budget accounting for every evaluation | SP1–SP7, PL1, PL4, PL9, C1-1, C1-2, C1-7, C1-11 |
| Correct footprint comparisons | FP1–FP12 |
| Safe stops and cautious mode | C1-5, C1-6, C1-11, VF5 |
| No unfair information restriction on C1 | C1-3, C1-4, IV8 |

---

## 7. Implementation order

As in Stages 1–2:
- **one sub-stage at a time**, each built, checked by the owner, reviewed by Claude, and committed before the next;
- **Copilot never commits**;
- each sub-stage starts in a **fresh Copilot chat**, with a prompt saved first in `docs/copilot_prompts/` (D-013).

Stage 3 is the largest stage so far, so it has **six** sub-stages.

| Step | Sub-stage | Files | Tests | Deliberate failure check (Brief L7) |
|---|---|---|---|---|
| 1 | — | The owner commits this approved plan: `Stage 3: add approved Stage 3 plan` | — | — |
| 2 | **3a** footprint and observation | `observation.h`, `footprint.*`, `test_footprint.cpp` | FP1–FP12 | Yes: on the footprint's c2 rule |
| 3 | **3b** dead squares and heuristic | `dead_squares.*`, `heuristic.*`, `test_dead_squares.cpp` | DS1–DS8 (DS3's search part waits for 3c and is added then) | — |
| 4 | **3c** step predictor and planner | `step_predictor.*`, `planner.*`, their tests | SP1–SP6, PL1–PL9, DS3 completed | Yes: on the reserve check |
| 5 | **3d** records, investigation, verification | `records.h`, `agent_model.*`, `investigation.*`, `verification.*`, their tests | IV1–IV9, VF1–VF12 (the parts needing the decision step wait for 3e) | Yes: on the time-aware object V1 |
| 6 | **3e** C1 decision step and run loop | `single_agent*`, `run_loop.*`, `agent_log.*`, `test_single_agent.cpp` | C1-1 to C1-10, remaining VF parts | — |
| 7 | **3f** C0, demos, stage number, documentation | `naive_agent.*`, `agent_demo.*`, `main.cpp`, `project_info.cpp`, P2, README (9.2), DECISIONS (Section 8), `test_naive_agent.cpp` | C0-1 to C0-5, P1–P2; **`ai_hologram` output matches 5.5 exactly** | — |
| 8 | — | Fresh-build check: reset the CMake cache, rebuild everything, no warnings, output and tests correct | | |
| 9 | — | Owner verification gate (Section 10) | | |
| 10 | — | Record, commit, and tag **`stage-3`** (owner only, after Step 9). This tag marks the **C1 baseline** required by Phase A Section 3. | | |

**Commit messages** follow the pattern `Stage 3x: <what was added>`.

**Deliberate failure checks.** For each one, Claude gives one exact temporary edit after reviewing the code. The owner confirms that exactly the expected tests fail, undoes the edit, rebuilds, and confirms that everything passes. The edit is never committed.

**Running programs and CTest from PowerShell** (rule 2g). After **Build → Build Project**, in the same terminal tab:
```
$env:Path = "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\mingw\bin;" + $env:Path
& "C:\Program Files\JetBrains\CLion 2026.2.3.1\bin\cmake\win\x64\bin\ctest.exe" --test-dir cmake-build-debug --output-on-failure
```

---

## 8. Decisions to record in `docs/DECISIONS.md` (sub-stage 3f)

| ID | Decision | Reason | Alternatives considered |
|---|---|---|---|
| D-019 | C1 and C0 are built from plain structs and free functions in `src/agent/`. The agents' state is a plain struct, so tests can build any state directly. There is no inheritance. | Keeps the code testable and readable for a beginner (Brief 8.2), and keeps the no-inheritance rule (D-010). | An agent base class with C0 and C1 subclasses. |
| D-020 | The planner is a standard A* over the agent's own belief *(corrected in v1.1)*. It tracks the best *g* per state, replaces a path when a shorter one is found (reopening closed states), skips outdated queue entries, and accepts the goal when it is taken from the queue. Ties are broken by (*g* + *h*, *h*, entry number). The heuristic is admissible and consistent when nothing is overridden. The fallback chooses the best discovered state by (*h*, best *g*, discovery number). | Guarantees shortest plans when the search finishes, which makes a strong baseline (Brief Stage 3). It is deterministic. ChatGPT's review of v1.0 showed that accepting the goal when generated was not guaranteed shortest; the reference implementation confirmed this on 86 of 3,000 layouts. | Breadth-first search (shortest, but expensive); greedy best-first (cheaper, not shortest); goal accepted when generated (v1.0; not guaranteed shortest). |
| D-021 | A per-step prediction cache allows free reuse of an identical prediction (same walls, positions, action and model) within one decision step. Variant predictions are never cached. A **reserve** (1 committed prediction + 1 V2 alternative per pending wall, object or model correction) is protected from every optional evaluation. If no committed prediction can be made, C1 takes no world action and safe-stops *(clarified in v1.1)*. | Implements Phase B 10.7 "reuse" exactly. The reserve guarantees that C1 never acts without a checked prediction, and can test its pending corrections. | No cache (wasteful); caching across steps (not allowed by Phase B); a reserve covering only the committed prediction (pending corrections could never be tested at a tight budget). |
| D-022 | Belief candidates from one mismatch are verified **together** with one shared re-prediction. A `model(a)` candidate's only correction is "use the standard rule for *a*". | The candidates come from exact evidence lookups, so applying them together reproduces the observation when they are right. In Experiment 1 the world rules are fully known (Phase B 11.4). | Verifying each candidate alone (fails when two elements are wrong); learning a new model (out of scope). |
| D-023 | In Stage 3, active tests are planned only for **wall** corrections. Object and action-model corrections are verified when a later committed prediction discriminates. | Phase B 11.5 says C1 *may* choose an active test. A wall test is simple and always safe (moving into a wall). An object's old value soon stops referring to the present, and a model test needs a safe push. | Active tests for every element type (more code; ill-defined for objects). |
| D-024 | An exhaustive search (for a consequence surprise) must finish within one decision step's budget. It is not continued over several steps in Stage 3. **Accepted by ChatGPT as a documented simplification. It may reduce C1's ability to notice F4 at tight budgets, and this limitation must be kept in every report of Experiment 1.** | Phase B 11.4 B allows, but does not require, multi-step completion. One-step searches are much simpler (Phase B 12.7.4). | Keeping search state across steps. Any later change would follow the Phase A pilot rules (9.2–9.4): it may be made only because something does not work, never because it favours any condition. |
| D-025 | After a mismatch in which the belief matched the record, so that `model(a)` is suspected, the footprint is updated **at once**. If V1 later fails, the update is logged as unexplained. | The belief before the action was confirmed by the record, so the world's change must be taken in to continue. Found by the reference implementation, where omitting it made every later prediction wrong. | Waiting for V1 before updating (the belief stays stale and causes false mismatches). |
| D-026 | When C0 has no plan and no fallback action, it takes the first action its own model predicts as Moved or Pushed, otherwise North. C0 has no safe stops. | Keeps C0 deterministic and naive. The run always progresses to an outcome (Phase A 5.1). | Letting C0 stop (it has no safe stops in Phase B 11.8). |
| D-027 | Stage 3 code takes B, S_max, T_max and T_v as parameters. The demos use illustrative values (200, 5, 100, 3), and the program says so. | The real values are Phase C decisions. Hard-coding them would risk silently fixing protocol numbers. | Hard-coded constants. |

---

## 9. Documentation changes

### 9.1 Code comments
Every new file has the three-part header (Purpose / "Introduced in Stage 3" / Architecture). Beginner comments explain, where each first appears:
- the open list's ordering, and the container used for it;
- why every successor state must come from the step predictor;
- the cache and why its key includes the belief walls;
- what a variant prediction is, and why it is never cached;
- what the reserve is for;
- the footprint rule, and why C1 does not re-check its whole belief every step (Phase B 12.1);
- why the fault activation point is empty in Stage 3;
- the difference between a dead square and a full-state dead end.

### 9.2 `README.md` status
Replace the status paragraph with:
> **Current status: Stage 3 — competent single agent.** The project contains a 10×10 grid world with tested movement rules; a world model, budget counter and comparison (Stage 2); and C1, the competent single agent defined in the approved Experiment 1 protocol (Phase B). C1 plans with a budgeted search over its own belief, checks each prediction against the observation, investigates mismatches, verifies corrections, and can take safe stops. C0, a naive agent used only as a reference, is also included. Demonstrations use a hand-written wrong belief. **No faults are injected, no two-agent reasoning exists, and no experiments have been run.** Files in `results/` are organizational examples, not experimental findings.

### 9.3 `.github/copilot-instructions.md`
No change.

### 9.4 `docs/EXPERIMENT_LOG.md`
No change. Stage 3 runs no experiments; the reference checks in 5.5 are design checks, not experiments.

### 9.5 `docs/copilot_prompts/`
One prompt file per sub-stage (`../../../../Downloads/stage_3a_prompt.txt` … `stage_3f_prompt.txt`), plus any follow-ups, saved before use (D-013).

---

## 10. Stage 3 completion checklist (owner verification gate)

**Approval and history**
- [ ] This plan was approved and committed before any Stage 3 code.
- [ ] Git history is intact; the gate was passed before the completion commit and the `stage-3` tag.

**Scope**
- [ ] Only the files in 4.1 (plus `CMakeLists.txt`, `README.md`, `docs/DECISIONS.md`, this plan, the handoff notes and the prompt files) were added or changed.
- [ ] Nothing from Section 2: no fault injection, scoring, messages or randomness; the Stage 1 and Stage 2 code is unchanged (except P2).
- [ ] Every Copilot instruction is saved in `docs/copilot_prompts/`.

**Build, run, test**
- [ ] No warnings, including after a cache reset.
- [ ] `ai_hologram` output matches **5.5 exactly**, including both model-evaluation counts.
- [ ] `ai_hologram_tests`: all PASS, 0 failures; every test in Section 6 is covered.
- [ ] CTest passes.
- [ ] The three deliberate failure checks (3a, 3c, 3d) were done; the expected tests failed; everything passed after undoing them.

**Conformance to the protocol**
- [ ] C1 follows Phase B Section 11 and this plan's Section 3. Any difference is recorded and approved.
- [ ] Every successor state comes from the step predictor; the only free computations are those in Phase B 10.7.
- [ ] C1 receives every observation in full; nothing restricts what it receives.
- [ ] The fault activation point exists and is empty.

**Code and documentation**
- [ ] No exceptions; no inheritance; functions under about 40 lines; files under about 200 lines (or a recorded reason).
- [ ] Header blocks and beginner comments (9.1).
- [ ] README status matches 9.2; DECISIONS.md contains D-019 to D-027.
- [ ] The owner can explain the items in Section 11.

---

## 11. Things the owner should understand by the end

1. What C1's decision step does, in order, and why the order matters.
2. What the footprint is, and why checking only the footprint loses nothing when there are no faults.
3. How A* uses the heuristic, and why the heuristic must not overestimate.
4. What the per-step budget, the cache and the reserve are for.
5. The difference between a dead square and a full-state dead end.
6. How an investigation tells a wrong belief (S2) from a wrong action model (S1).
7. What V1 and V2 check, and why V1 must be time-aware for the object.
8. Why a correction can be CONSISTENT without being VERIFIED, and what an active test is.
9. When C1 takes a safe stop, and why safe stops are limited.
10. Why C0 exists, and why it never reads observations after the first.
11. Why C1 must not be changed after the `stage-3` tag except to fix a genuine bug.

---

## 12. What actually happened (filled in by the owner at the end)

```
Date completed:
Commit tagged stage-3:
Total number of checks in the final test run (N):

Did everything go as planned?  yes / no
Differences from the plan, and why:

Problems encountered and how they were solved:

Anything learned that should change future stage plans:
```

---

## 13. Instructions to GitHub Copilot for this stage

1. Read `.github/copilot-instructions.md`, this plan, the brief, and protocol Phase B (Sections 10–12) before writing anything.
2. Implement **one sub-stage at a time**, only when the owner asks for it by name.
3. Before creating or changing any file, check whether it exists and read it first.
4. **From the start:**
   - every new file has the three-part header and the beginner comments in 9.1;
   - every function is under about 40 lines;
   - every file is under about 200 lines;
   - **no exceptions**, no assertions, no inheritance, no random numbers.
5. Implement Section 3 **exactly**, including orders, tie-breaking and log text. If something is unclear, impossible, or seems to disagree with Phase B, **stop and ask**. Never weaken a rule or a test to make something pass.
6. **Every test world must be valid**, unless the test is about invalid states.
7. **Every** prediction goes through the `StepPredictor`, and from there through `BudgetCounter`. Never create a successor state any other way, including in "helper" calculations.
8. Do not rename, move, or delete any file, and do not use refactoring tools on files. Do not change Stage 1 or Stage 2 code or tests, except P2.
9. Do not commit, tag, or push.
10. When finished, **append** a handoff note (rule 2f). Say honestly what was built and run, which files changed, and the exact test result. Remind the owner to use **Build → Build Project** before CTest.

---

## Revision history of this plan

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-10-19 | First draft by Claude after protocol Phase B was approved (commit `da9af19`). Implements Phase B Section 11 (C1 and C0) with the definitions of Sections 10 and 12, in six sub-stages, with tests covering every item of Phase B 12.9 and ChatGPT's list for this plan (6.10). The exact expected output (5.5) was produced by a reference implementation of this plan written by Claude in Python; that implementation also found one gap, the belief update after a suspected action-model error, now fixed by D-025. Decisions D-019 to D-027 interpret Phase B where it leaves choices open. For independent review by ChatGPT and approval by the owner. |
| 1.1 | 2026-10-19 | ChatGPT reviewed v1.0, supported the Stage 3 architecture, and requested two corrections. **(1) Planner (3.5, D-020):** v1.0 marked states visited when created and accepted the goal when generated, which is not guaranteed shortest. It is now a standard A*: best *g* per state, path replacement and reopening, outdated entries skipped, goal accepted when taken from the queue, and tie-breaking documented. Its consistency (and so admissibility) is argued in 3.5. The reference implementation was rerun: its plans matched exhaustive breadth-first search lengths on all 3,000 random layouts, against 86 longer plans from the v1.0 rule. PL2 now includes such a layout (9 versus 11 actions) and a path-replacement case. **The expected output (5.5) was regenerated; the step lines are unchanged, and the evaluation counts changed from 17 and 401 to 19 and 414.** PL1 now expects 16 evaluations. **(2) Reserve (3.4, 3.6–3.9, D-021):** the reserve is defined as the committed prediction plus the V2 alternatives. Every optional evaluation (investigation re-prediction, V1 replays, trap V2, cautious checks, active-test search, main search) must keep it, with the behaviour of each when it cannot. C1 never takes a world action without a committed prediction, and safe-stops with a logged reason instead. New log lines (5.3) and tests SP7 (several pending V2 corrections, every B from 1 to 11; checked in the reference implementation) and C1-11. **D-024** is accepted as a documented simplification, and its F4 limitation is kept (5.6, D-024). No other change; existing tests and comment requirements kept. Not approved. |
| 1.2 | 2026-10-19 | **APPROVED.** ChatGPT approved version 1.1 for implementation, and the owner approved it. ChatGPT accepted the evaluation counts 19 and 414 as reference-output expectations, not as independently verified C++ results, and asked that a test be kept for the object already on the destination. Added accordingly: **PL10** (the planner returns Found with an empty plan and 0 evaluations, despite a non-zero heuristic). Checking that case in the reference implementation showed two situations the plan did not yet state, now specified: the run ends with Success after 0 world steps if the object starts on the destination (3.10), and C1 safe-stops with `belief shows goal reached` if its search returns an empty plan while the run has not ended (3.6; new log line 5.3; test **C1-12**). The expected output (5.5) is unchanged; the reference implementation confirmed it. No other change. |
