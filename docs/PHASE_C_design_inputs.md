# Phase C design inputs: spatial agent and temporal-spatial witness

**Status:** design inputs, agreed in discussion between the owner, ChatGPT and Claude on October 20, 2026. **Not approved protocol text.** They will be turned into draft Phase C rules for C2–C4 after the `stage-3` tag, then reviewed independently. Nothing here changes Phase A, Phase B or the C1 baseline.

## The owner's architecture
- **Spatial agent:** mainly performs actions and spatial planning.
- **Temporal-spatial witness:** mainly examines how observations, predictions and events relate across time, and helps the spatial agent.
- **Overlap:** both keep overlapping spatial and temporal-spatial reasoning, so that each can check the other.
- **Reciprocal checking is essential:** the spatial agent must also be able to catch the witness's mistakes.

## 1. The common relative zero
- **Shared frame:** grid coordinates (x, y) plus the decision step t.
- **Local reference:** for each agent i, Z_i = (x_i, y_i, t_i).
- **Converting an event:** for an event E = (x, y, t), relative_i(E) = (x − x_i, y − y_i, t − t_i). Converting back to the shared frame adds Z_i.
- **Two kinds of reference:** a fixed initial reference must be kept separate from a moving current reference.
- **Offsets are signed in space and time.** Positive and negative values describe displacement only; they never mean authority or superiority.
- **Tests:** zero, positive and negative offsets; boundaries; round trips; agreement after conversion to the shared frame.

## 2. Claims, and disagreement between them
- **A claim records:** element (wall, object or agent), value, cell, time step, source agent, evidence channel, and the evidence itself (which observation or prediction it came from).
- **Comparing claims:** two claims are compared only after normalising their spatial references, time references, element identities and observation times.
- **What a disagreement means:** a reason to investigate, never proof of which agent is wrong. The evidence and source of both claims are kept.
- **Tests:** stale observations, different reference moments, wrong conversions, and genuine contradictions.

## 3. Channels (the "senses")
- **Abstract senses:** channels such as eyes or touch are an abstract representation, acceptable in this grid world.
- **Evidence, not access:** a channel names **which evidence a claim uses**. It never limits what an agent receives; every reasoner still receives the complete observations required by Phase B.
- **No privileged observations** for either agent.

## 4. Fairness
- **Equal budgets:** each two-agent condition's total model-evaluation budget equals C1's.
- **Costs are defined and counted:** message costs, and any extra computation for building, converting and comparing claims.
- **No unbounded free reasoning.**
- **Same accounting** for C2, C3 and C4.

## 5. Meaningful comparisons
- **What each condition tests:**
  - C2 tests duplication;
  - C3 tests different methods with overlapping responsibilities;
  - C4 tests divided responsibilities without the same reciprocal checking.
- **No special advantage for C3:** the shared-frame mechanism is available to every two-agent condition.
- **Deliberate failure checks cover four cases:**
  - the spatial agent is wrong;
  - the witness is wrong;
  - both are wrong, including common-mode faults;
  - neither can resolve the disagreement.

## 6. Process
- **When:** written as draft Phase C rules after the `stage-3` tag and completion gate.
- **How it is checked:** exact reference values and deliberate failure checks, as in Stage 3.
- **No early tag:** no `protocol-1-approved` tag until Phase C and the complete protocol have been independently reviewed and approved.

## 7. Owner's observations from the Stage 3 gate (design goals for the witness)
- **A reflex, not only a checklist.**
  - **What C1 lacks:** C1 notices errors only after they happen, once per step, in a fixed order.
  - **The goal:** the witness should give the system anticipatory, continuous sensing, like a cat's whiskers working with its tail, so that a problem can be sensed **before contact**.
  - **Limits:** it must still be measurable and run within the same budget.
- **Awareness beyond the route.**
  - **What C1 lacks:** C1 compares only the footprint, the cells along its own route.
  - **The goal:** the witness should monitor the area a change could affect, not just the spatial agent's path.
  - **Tests:** include faults placed **outside** the route, and measure whether the witness detects them before the spatial agent reaches them (the cat sees the snake before stepping near it).
- **Paired estimates instead of one fixed heuristic.**
  - **What C1 lacks:** C1's A* uses a single spatial estimate (h), computed by a fixed formula from one perspective and never corrected.
  - **The goal:** groups of paired spatial and temporal-spatial evaluators, one per sense or evidence channel. Each group measures offsets (added or subtracted) from the shared relative zero, and the two parts adjust each other.
  - **Constraint:** an adjustment must never make the planner's estimate overestimate the true remaining cost. Otherwise A*'s shortest-plan guarantee is lost. This needs explicit tests.
  - **Same total budget.**
- **A hierarchy of groups sharing the same budget.**
  - **What C1 lacks:** C1 has one central pool of B predictions per step, one level, and a fixed priority order.
  - **The goal:** part-level groups (a paw, an eye), each with its own array of paired spatial and temporal-spatial groups predicting locally, plus whole-level groups (the mind) that help and coordinate them. Together they solve the problem as one larger group.
  - **Budget rules:**
    - each group gets a share of the **same total B** as C1;
    - each part-level group keeps a local reserve for reflex predictions;
    - coordination between levels has a counted cost, taken from the same total.
  - **How B is divided** is fixed in advance and checked by pilots under the Phase A rules, never tuned to make any condition win.
- **Zero and "greater than zero" kept separate.**
  - **What C1 lacks:** C1 accepts being trapped as an outcome.
  - **The goal:**
    - keep "zero" (the current reference) and "greater than zero" (the reachable room to act) as separate, measured quantities;
    - watch the room to act every step, so that a shrinking space is sensed before the trap closes;
    - treat its collapse as a contradiction that triggers explanation and escape (check beliefs, look for a way out), not as an accepted outcome.
  - **Tests:** layouts where the room to act shrinks step by step, measuring how early each condition notices.
- **Trading zero locations versus being merged.**
  - **What C1 lacks:** C1 changes its reference only when a mismatch forces it to.
  - **The goal:**
    - **a voluntary trade:** an agent adopts another agent's reference when independent evidence supports it, or when it is worth it for the goal (the reward);
    - **an imposed merge:** copying another's view without evidence must be resisted, as in the previous point.
  - **Tests:** check that agents adopt each other's views only with evidence, and keep their own otherwise.
- **Graded trust built from measured rewards.**
  - **What C1 lacks:** verification is one all-or-nothing event, and all history counts fully forever.
  - **The goal:**
    - confidence that grows with repeated, measured confirmations;
    - a change that began as imposed (a bad trade) can become the accepted, and finally the only, reference once enough evidence accumulates;
    - old references fade **in weight**, but records and provenance are kept.
  - **Tests:**
    - follow how confidence changes over a run;
    - check that fading never erases what is needed to detect a fault that reverts.
- **Confirmation through the other agent's experience.**
  - **What C1 lacks:** C1 confirms only through its own predictions, or active tests it carries out itself (only for walls).
  - **The goal:**
    - one agent's observations and outcomes can count as confirming evidence for the other, under the independence rules (a trade, not a merge), reducing the risk each agent takes;
    - the source of a confirmed change is kept on record, even when it no longer matters to reasoning (don't forget the stranger).
  - **Tests:** compare confirmation through the other agent with self-testing, in steps taken and in risk.
- **Safe ground, unsafe ground and retreat.**
  - **What C1 lacks:** C1's safe stop freezes the agent where the trouble arose.
  - **The goal:**
    - when danger is detected, retreat to a known safe earlier reference instead of freezing;
    - build a map of safe and unsafe ground from repeated history;
    - more checking on unsafe ground and less on safe ground, within the same total budget;
    - leave safe ground only once trust has reached a threshold (the graded confidence above).
  - **Tests:**
    - retreat behaviour;
    - how vigilance is spread by ground;
    - no move leaves safe ground without enough trust.
- **More than one group active, and low trust in danger zones.**
  - **What C0 and C1 lack:** C0 sees only its plan. C1 checks only along its route and only for known kinds of error; an unexplained surprise (S5) doesn't lower its trust.
  - **The goal:**
    - several groups active at once, including at least one watching for dangers unrelated to the current goal;
    - unexplained surprises mark the area as low-trust, and trust in trades of zero there is lowered.
  - **Tests:** faults unrelated to the current goal, measuring whether they are noticed.
  - **For later experiments (outside Experiment 1's scope):** worlds with third-party dangers and new kinds of hazard (mud, another animal), to test whether the architecture notices dangers it was never told about.
- **Nested jobs judged by real effect.**
  - **The goal:**
    - each group's array holds mini-jobs inside a larger job;
    - every mini-job's result is judged by its real effect on the job above it (zero versus greater than zero), using the other agent's view as a check.
  - **What C0 and C1 show:** C0 counted progress that never happened. C1 checks real effect but has only two levels (step and goal).
  - **Tests:** cases where a mini-job "succeeds" internally while the larger job makes no real progress, measuring whether that is caught.
- **Records that cannot be made to forget.**
  - **The risk:** making a system forget its past and live only in the present is a way to manipulate it (the feral cat that traded its freedom for candy).
  - **The goal:**
    - the witness's records resist being erased or rewritten, whether by faults or by the other agent;
    - weight may fade, records may not.
  - **Tests:** attempts to make one agent "forget", such as faults that overwrite history, measuring whether the system notices.
- **Adapting to survive, and trust amplified by independent agreement** (owner's note, after the Phase C v0.9 draft).
  - **The owner's image:**
    - the dissenter who adapts is not reckless: the one who trusts and lives is followed by the one who did not trust;
    - those who keep judgements built on logical errors fade;
    - when the spatial and the temporal-spatial agents both agree on changing the location of zero, the change is amplified gradually, like water poured into a cup: a feral cat becoming tame and living, rather than staying wild and being killed (a lion in a city, versus a domestic cat).
  - **In Experiment 1 (Phase C):** an agent moves its zero only through its own evidence, and the safe test lets the world decide, so adapting to evidence is what succeeds. Agreement is never used as proof (Brief R3).
  - **For a later experiment:**
    - trust that grows gradually when the two agents reach the same change **independently**, each from its own evidence, and never when one copies the other;
    - it must be tested against common-mode faults (both wrong together), to show that amplification helps more than it misleads.
- **Sacrifice for the larger job** (owner's note: chess).
  - **The owner's image:** if a member of a mini-group (spatial, temporal-spatial) acknowledges the greater truth and acts on it, the king may not be caught. It may not be good for the pawn, but it lets the game continue.
  - **In Experiment 1:**
    - cautious actions and safe stops give up progress to avoid a trap (Brief 4.2, step 4b: acting is not always better than not acting);
    - a reasoner gives up its own belief when its own evidence shows it was wrong.
  - **For a later experiment (with groups and nested jobs):** a group member that recognises the larger job's truth may act against its own mini-job's interest, so that the larger job survives. Tests should measure whether such sacrifices help the whole more than they cost, and whether a member can be wrongly persuaded to sacrifice itself (a common-mode or false-alarm risk).
