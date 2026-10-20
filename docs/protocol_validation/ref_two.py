"""Reference implementation of protocol Section 13 (C2, C3, C4), draft v0.9.
Built on ref_c1.py (the validated C1 reference). Used only to validate the protocol text
and to produce exact expected values. Not project code."""
import sys, heapq, itertools
sys.path.insert(0, '.')
import ref_c1
from ref_c1 import (C1, W, parse, valid, apply_action, Budget, dead_map, DIRS, DIRV, INF,
                    add, footprint, contents, compare_fp, fp_update, fmt, heuristic)

TRAP_CHECK_ON_OBJECTION = False
ENFORCE_MMAX = True
A2_RADIUS = 2


class MessageLimit(Exception):
    """M_max reached in this decision step (13.2): the disagreement is treated as unresolved."""

ROT = {'left': {'North': 'West', 'West': 'South', 'South': 'East', 'East': 'North'},
       'right': {'North': 'East', 'East': 'South', 'South': 'West', 'West': 'North'}}


def make_f3(action, rot):
    """F3 (12.2): when `action` would push, the object is predicted to move in the rotated
    direction; Blocked if that cell is believed blocked."""
    def f(s, a):
        n, oc = apply_action(s, a)
        if a == action and oc == 'Pushed':
            m = s.copy(); m.agent = add(s.agent, DIRV[a])
            tgt = add(m.agent, DIRV[ROT[rot][a]])
            if s.wall(tgt) or tgt == m.agent: return s.copy(), 'Blocked'
            m.obj = tgt
            return m, 'Pushed'
        return n, oc
    return f


def fp2_cells(pre, a, p1, p2):
    """Two-prediction footprint (10.5): c0, c1, and c2 if either predicts a push or the
    believed object is at c1."""
    d = DIRV[a]; c0 = pre.agent; c1 = add(c0, d); cells = [c0, c1]
    if pre.obj == c1 or p1[1] == 'Pushed' or p2[1] == 'Pushed':
        cells.append(add(c1, d))
    return cells


def differ(p1, p2, cells):
    out = p1[1] != p2[1]
    diffs = [c for c in cells if contents(p1[0], c) != contents(p2[0], c)]
    return out or bool(diffs), diffs


class Reasoner(C1):
    def __init__(self, first_obs, name, syslog, B, constraints_allowed=False, **kw):
        self.name = name
        self.syslog = syslog
        super().__init__(first_obs, B=B, log=lambda m: syslog(f"{name}: {m}"), **kw)
        self.version = 0; self.plan_version = 0
        self.excluded = set(); self.forbidden = set()
        self.candidate = None; self.blocked = None
        self.used_this_step = 0

    # --- belief version (13.6a) ---
    def apply_correction(self, kind, c, v, old):
        self.version += 1; super().apply_correction(kind, c, v, old)

    def revert(self, c):
        self.version += 1; super().revert(c)

    def investigate(self, pre, a, obs, cells):
        before = (repr(self.belief.walls), self.belief.obj)
        super().investigate(pre, a, obs, cells)
        if self.pending_v1 is not None or (repr(self.belief.walls), self.belief.obj) != before:
            self.version += 1

    # --- C4 planning constraints (13.7): pruning only, never beliefs ---
    def predict(self, state, a, model_fn=None, cache=True):
        r = super().predict(state, a, model_fn, cache)
        return r

    def search(self, root, goal, hfun, reserve):
        if not (self.excluded or self.forbidden):
            return super().search(root, goal, hfun, reserve)
        orig = self.predict
        def constrained(state, a, model_fn=None, cache=True):
            r = orig(state, a, model_fn, cache)
            if r is None: return r
            if (state.agent, state.obj, a) in self.excluded or (r[1] == 'Pushed' and r[0].obj in self.forbidden):
                return state.copy(), 'Blocked'
            return r
        self.predict = constrained
        try:
            return super().search(root, goal, hfun, reserve)
        finally:
            del self.predict

    # --- decision-step pieces ---
    def start(self, obs, fault_hook=None):
        self.decision += 1
        self.budget = Budget(self.B); self.cache = {}
        if self.decision > 1: self.history.append(obs)
        self.blocked = None
        self.check(obs)
        if self.pending_inv:
            args = self.pending_inv; self.pending_inv = None; self.investigate(*args)
            if self.pending_inv: self.blocked = 'budget too small for verification'
        if self.pending_trap: self.trap_v2(self.pending_trap)
        if fault_hook: fault_hook(self)                     # fault activation point (12.3)
        if self.pending_v1: self.continue_v1(obs)

    def commit(self, a, r):          # proposal only: the final action is fixed by the system
        self.plan.pop(0)
        if not self.plan: self.plan_kind = None
        self.candidate = (a, r)
        return ('act', a)

    def safe_stop(self, reason):
        return ('stop', reason)

    def propose(self):
        if self.blocked: return ('stop', self.blocked)
        before = id(self.plan)
        res = self.choose()
        if id(self.plan) != before or self.plan_kind is None:
            self.plan_version = self.version
        return res

    def announced(self, L):
        a1, r1 = self.candidate
        out = [(a1, r1[0])]
        if self.plan_version == self.version:
            out += [(a, s) for a, s in self.plan[:L - 1]]
        return out

    def pred(self, state, a, use_reserve=False):
        key = (state.agent, state.obj, a, self.walls_version)
        if key in self.cache: return self.cache[key]
        if not use_reserve and self.budget.remaining() <= self.reserve(): return None
        return self.predict(state, a)

    def retain(self, a, r):
        tests = []
        for c in self.corrections:
            if c['status'] != 'CONSISTENT' or c['tested']: continue
            alt = self.alt_prediction(c, a)
            if alt is None: continue
            if alt[1] != r[1] or alt[0].key() != r[0].key():
                tests.append(c['id']); c['tested'] = True
        self.pending = (r[0], r[1], a, self.belief.copy(), self.decision, tests)

    def safe_for_me(self, r):
        return r is not None and r[1] != 'InvalidState' and not (r[1] == 'Pushed' and self.dead[r[0].obj])

    # --- evidence checks during an investigation (11.4 evidence lookups) ---
    def present_wrong(self, cells, obs_state):
        """Present values in `cells` (walls) and the current object position, checked only
        against the CURRENT observation (their own time)."""
        wrong = []
        for c in cells:
            if self.belief.wall(c) != obs_state.wall(c):
                wrong.append(('wall', c, obs_state.wall(c)))
        if self.belief.obj != obs_state.obj and (self.belief.obj in cells or obs_state.obj in cells):
            wrong.append(('object', None, obs_state.obj))
        return wrong

    def correct_present(self, wrong):
        for kind, c, v in wrong:
            if kind == 'wall':
                if not self.v1_history('wall', c, v): continue
                self.log(f"investigation: wall at {fmt(c)} believed {'blocked' if self.belief.wall(c) else 'free'}, observed {'blocked' if v else 'free'} now (source S2)")
                self.apply_correction('wall', c, v, old=self.belief.wall(c))
            else:
                if self.history[-1]['state'].obj != v: continue      # V1: own time = current observation
                self.log(f"investigation: object believed at {fmt(self.belief.obj)}, observed at {fmt(v)} now (source S2)")
                self.apply_correction('object', None, v, old=self.belief.obj)

    def replay_model(self, a):
        """Replay recorded transitions with action a through this reasoner's OWN current model
        (budgeted). Returns 'fail', 'pass' or 'none' (nothing to replay / budget)."""
        trans = [k for k in range(1, len(self.history)) if self.history[k]['action'] == a]
        if not trans: return 'none'
        for k in trans:
            if self.budget.remaining() <= self.reserve(): return 'none'
            r = self.predict(self.history[k - 1]['state'], a, model_fn=self.model.fn(a), cache=False)
            if r is None: return 'none'
            if r[1] != self.history[k]['outcome'] or not r[0].same(self.history[k]['state']):
                return 'fail'
        return 'pass'

    def suspect_model(self, a, obs):
        self.log(f"investigation: own action model for {a} fails a recorded transition (source S1)")
        trans = [k for k in range(1, len(self.history)) if self.history[k]['action'] == a]
        self.pending_v1 = {'action': a, 'todo': trans, 'cells': [], 'obs': obs}
        self.version += 1
        self.continue_v1(obs)


class TwoSystem:
    def __init__(self, cond, true_state, log, B=200, S_max=5, T_max=100, L=10, M_max=16,
                 r1=None, r2=None):
        self.cond, self.log, self.B, self.S_max, self.T_max, self.L, self.M_max = cond, log, B, S_max, T_max, L, M_max
        self.true = true_state.copy()
        o = {'step': 1, 'state': true_state.copy(), 'action': None, 'outcome': None}
        self.obs = o
        self.R1 = Reasoner(o, 'R1', log, B // 2, **(r1 or {}))
        self.R2 = Reasoner(o, 'R2', log, B // 2, **(r2 or {}))
        self.safe_stops = 0; self.steps = 0; self.decision = 0
        self.records = []          # disagreements: dicts
        self.objections = []       # open future objections
        self.messages = 0; self.mmax_hits = 0; self.step_msgs = 0; self.max_step_msgs = 0
        self.open_warnings = set()
        self.unresolved_cells = set()

    def msg(self, n=1):
        if ENFORCE_MMAX and self.step_msgs + n > self.M_max:
            self.mmax_hits += 1
            raise MessageLimit()
        self.messages += n; self.step_msgs += n
        self.max_step_msgs = max(self.max_step_msgs, self.step_msgs)

    def stop(self, reason):
        self.safe_stops += 1
        self.log(f"Step {self.decision}: system safe stop ({reason})")
        return None

    # ---- look-ahead by R2 along R1's announced plan (13.6, 13.6a) ----
    def lookahead(self, r2_immediate, start):
        """Look-ahead from `start`: in C3, R2's own immediate prediction (after agreement);
        in C4, R2's belief with agent/object positions from R1's announced step-1 state."""
        R1, R2 = self.R1, self.R2
        ann = R1.announced(self.L)
        state = start
        if self.cond == 'C3' and r2_immediate[1] == 'Pushed' and R2.dead[state.obj]:
            return ('trap', 1, ann[0][0], state.obj, None)
        for k in range(1, len(ann)):
            a, expected = ann[k]
            pre_r2 = state
            if R2.budget.remaining() <= R2.reserve(): return None
            r = R2.predict(state, a)
            if r is None: return None
            exp_out = 'Pushed' if expected.obj != ann[k - 1][1].obj else 'Moved'
            cells = fp2_cells(pre_r2, a, r, (expected, exp_out))
            dis, diffs = differ(r, (expected, exp_out), cells)
            if dis:
                return ('future', k + 1, a, diffs, (pre_r2, r, ann[k - 1][1], expected, exp_out, cells))
            if r[1] == 'Pushed' and R2.dead[r[0].obj]:
                return ('trap', k + 1, a, r[0].obj, None)
            state = r[0]
        return None

    # ---- investigations ----
    def investigate_immediate(self, a, p1, p2, cells):
        """13.4 step 8 a-d for the immediate step. Returns new (p1, p2) or None if unresolved."""
        R1, R2 = self.R1, self.R2
        obs_state = self.obs['state']
        self.msg(2)                                                # belief reports
        self.log(f"Step {self.decision}: disagreement on {a}; belief reports exchanged")
        w1 = R1.present_wrong(cells, obs_state); w2 = R2.present_wrong(cells, obs_state)
        if w1 or w2:
            if w1: self.msg(); self.log(f"Step {self.decision}: OBJECTION by R2 names R1")
            if w2: self.msg(); self.log(f"Step {self.decision}: OBJECTION by R1 names R2")
            R1.correct_present(w1); R2.correct_present(w2)
        else:
            for X in (R1, R2):
                res = X.replay_model(a)
                if res == 'fail': X.suspect_model(a, self.obs)
        q1 = R1.pred(R1.belief, a, use_reserve=True); q2 = R2.pred(R2.belief, a, use_reserve=True)
        if q1 is None or q2 is None: return None
        dis, _ = differ(q1, q2, fp2_cells(R1.belief, a, q1, q2))
        return None if dis else (q1, q2)

    def investigate_future(self, rec):
        """13.4b: present elements only, against evidence of their own time; never a future
        state against an observation. May correct present values; never blocks the step."""
        R1, R2 = self.R1, self.R2
        kind, k, a, info, extra = rec
        target = (self.decision + k - 1, a)                   # the absolute step the warning is about
        if target in self.open_warnings:
            self.log(f"Step {self.decision}: future disagreement at look-ahead step {k} ({a}); warning already open, retained")
            return
        self.open_warnings.add(target)
        self.msg(); self.log(f"Step {self.decision}: future disagreement at look-ahead step {k} ({a}); OBJECTION by R2 recorded")
        self.objections.append((self.decision, k, a))
        if kind == 'future':
            pre_r2, r, pre_r1, expected, exp_out, cells = extra
            obs_state = self.obs['state']
            for X in (R1, R2):
                w = [x for x in X.present_wrong(cells, obs_state) if x[0] == 'wall']   # walls only: static, present value
                X.correct_present(w)
            for X in (R1, R2):
                if X.replay_model(a) == 'fail': X.suspect_model(a, self.obs)

    # ---- one decision step ----
    def review_once(self, a1, p1):
        """Reviewer's prediction of the proposed action (C2: via its own choice)."""
        R2 = self.R2
        if self.cond == 'C2':
            prop2 = R2.propose()
            if prop2[0] == 'act' and R2.candidate[0] == a1: return R2.candidate[1]
        return R2.pred(R2.belief, a1, use_reserve=True)

    def resolve_unresolved(self, a1):
        R1, R2 = self.R1, self.R2
        q1 = R1.pred(R1.belief, a1, use_reserve=True); q2 = R2.pred(R2.belief, a1, use_reserve=True)
        if R1.safe_for_me(q1) and R2.safe_for_me(q2):                 # 8f safe test
            self.msg(); self.log(f"Step {self.decision}: TEST_REQUEST: {a1} is safe for both and settles the disagreement")
            return a1, q1, q2
        if self.cond == 'C3':                                           # alternative proposal by Agent 2 (13.6)
            saved = (R2.plan, R2.plan_kind)
            prop2 = R2.propose()
            if prop2[0] == 'act' and R2.candidate[0] != a1:
                a2, r2 = R2.candidate
                r1 = R1.pred(R1.belief, a2, use_reserve=True)
                self.msg(); self.log(f"Step {self.decision}: alternative PROPOSAL by R2: {a2}")
                if r1 is not None and not differ(r1, r2, fp2_cells(R1.belief, a2, r1, r2))[0] \
                        and R1.safe_for_me(r1) and R2.safe_for_me(r2):
                    self.log(f"Step {self.decision}: R1 reviews and agrees; alternative adopted")
                    return a2, r1, r2
            R2.plan, R2.plan_kind = saved
        for a, _ in DIRS:                                               # 9 cautious action
            c1 = R1.pred(R1.belief, a); c2 = R2.pred(R2.belief, a)
            if c1 is None or c2 is None: break
            dis, _ = differ(c1, c2, fp2_cells(R1.belief, a, c1, c2))
            if not dis and R1.safe_for_me(c1) and R2.safe_for_me(c2):
                self.log(f"Step {self.decision}: cautious action {a}")
                return a, c1, c2
        return None, None, None

    def decide(self):
        self.step_msgs = 0
        try:
            return self._decide()
        except MessageLimit:
            R1, R2 = self.R1, self.R2
            self.log(f"Step {self.decision}: message limit M_max={self.M_max} reached; disagreement treated as unresolved")
            for a, _ in DIRS:                                           # 9 cautious action (no further messages)
                c1 = R1.pred(R1.belief, a); c2 = R2.pred(R2.belief, a)
                if c1 is None or c2 is None: break
                dis, _ = differ(c1, c2, fp2_cells(R1.belief, a, c1, c2))
                if not dis and R1.safe_for_me(c1) and R2.safe_for_me(c2):
                    self.log(f"Step {self.decision}: cautious action {a}")
                    R1.retain(a, c1); R2.retain(a, c2)
                    return a
            return self.stop('message limit reached, no cautious action')

    def _decide(self):
        R1, R2, cond = self.R1, self.R2, self.cond
        self.decision += 1
        for c in list(self.unresolved_cells):                 # evidence changed an analysis: lift the requirement
            if R1.dead[c] == R2.dead[c]:
                self.unresolved_cells.discard(c); R1.forbidden.discard(c)
        hook = getattr(self, 'fault_hook', None)
        R1.start(self.obs, (lambda X: hook(X, self.decision)) if hook else None)
        R2.start(self.obs, (lambda X: hook(X, self.decision)) if hook else None)
        for X in (R1, R2):
            if X.blocked: return self.stop(X.blocked)
        prop = R1.propose()
        if prop[0] == 'stop': return self.stop(prop[1])
        a1, p1 = R1.candidate; self.msg()
        p2 = self.review_once(a1, p1)
        if p2 is None: return self.stop('no budget for a committed prediction')
        final = None
        if cond in ('C2', 'C3'):
            self.msg()
            for attempt in (1, 2):
                dis, diffs = differ(p1, p2, fp2_cells(R1.belief, a1, p1, p2))
                if not dis: break
                if attempt == 2:                                       # still disagreeing after re-proposal
                    final, p1, p2 = self.resolve_unresolved(a1)
                    if final is None: return self.stop('unresolved disagreement, no cautious action')
                    break
                self.records.append({'kind': 'immediate', 'step': self.decision, 'action': a1})
                v1 = R1.version
                res = self.investigate_immediate(a1, p1, p2, fp2_cells(R1.belief, a1, p1, p2))
                if R1.version != v1:                                   # proposer corrected: choose again (once)
                    self.log(f"Step {self.decision}: R1 corrected; R1 chooses again")
                    R1.plan = []; R1.plan_kind = None
                    prop = R1.propose()
                    if prop[0] == 'stop': return self.stop(prop[1])
                    a1, p1 = R1.candidate; self.msg()
                    p2 = self.review_once(a1, p1)
                    if p2 is None: return self.stop('no budget for a committed prediction')
                    continue
                if res is not None:
                    p1, p2 = res; break
                final, p1, p2 = self.resolve_unresolved(a1)
                if final is None: return self.stop('unresolved disagreement, no cautious action')
                break
            if final is None: final = a1
            if final == a1 and cond == 'C3':
                la = self.lookahead(p2, start=p2[0])                  # only after the immediate step agreed
                if la is not None and la[0] == 'trap' and la[1] == 1:
                    # a contested push onto a cell R2's analysis marks dead, at the IMMEDIATE step (13.4b):
                    self.msg(); self.log(f"Step {self.decision}: consequence OBJECTION by R2: {a1} pushes the object onto {fmt(la[3])}, dead in R2's analysis")
                    self.records.append({'kind': 'immediate-consequence', 'step': self.decision, 'action': a1})
                    if TRAP_CHECK_ON_OBJECTION:
                        # proposed interpretation: a partner's consequence objection is a consequence trigger for the
                        # receiver, investigated with exactly the 11.4 B local check and the 11.5 trap V1/V2
                        cell = la[3]
                        # first, the present walls the local check reads (p's neighbours), against the current
                        # observation (13.4b: present values, own time)
                        nb = [(x, y) for y in range(10) for x in range(10)
                              if abs(x - cell[0]) + abs(y - cell[1]) <= A2_RADIUS]   # bounded: radius 2
                        for X in (R1, R2):          # the alert: both look at the spot with their own evidence
                            X.correct_present([w for w in X.present_wrong(nb, self.obs['state']) if w[0] == 'wall'])
                        if not R2.dead[cell]:       # the witness's own look withdrew its objection
                            self.log(f"Step {self.decision}: R2's own evidence withdraws the objection (RESOLUTION)")
                            R1.retain(a1, p1); R2.retain(a1, R2.pred(R2.belief, a1, use_reserve=True) or p2)
                            return a1
                        fixed = R1.dead[cell]
                        if not fixed and not ref_c1.local_check_not_dead(R1.belief, R1.dead, cell):
                            R1.log(f"investigation of R2's objection: local check fails at {fmt(cell)} (source S1)")
                            R1.apply_correction('trap', cell, True, old=False)
                            fixed = True
                        if fixed:
                            R1.plan = []; R1.plan_kind = None
                            prop = R1.propose()
                            if prop[0] == 'stop': return self.stop(prop[1])
                            a1, p1 = R1.candidate
                            p2 = R2.pred(R2.belief, a1, use_reserve=True)
                            if p2 is None: return self.stop('no budget for a committed prediction')
                            final = a1
                            q1, q2 = p1, p2
                            R1.retain(final, q1); R2.retain(final, q2)
                            return final
                        # G9 loop breaker: still unresolved after both looked -> the push onto `cell` is not safe
                        # for both (Brief 4b); Agent 1 plans around it until evidence changes either analysis
                        self.log(f"Step {self.decision}: consequence dispute about {fmt(cell)} unresolved by evidence; push treated as not safe for both")
                        R1.forbidden.add(cell); self.unresolved_cells.add(cell)
                        R1.plan = []; R1.plan_kind = None
                        prop = R1.propose()
                        if prop[0] == 'stop': return self.stop(prop[1])
                        a1, p1 = R1.candidate
                        p2 = R2.pred(R2.belief, a1, use_reserve=True)
                        if p2 is None: return self.stop('no budget for a committed prediction')
                        R1.retain(a1, p1); R2.retain(a1, p2)
                        return a1
                    final, p1, p2 = self.resolve_unresolved(a1)
                    if final is None: return self.stop('unresolved disagreement, no cautious action')
                elif la is not None:
                    self.investigate_future(la)
        else:                                                          # C4
            final = a1
            rounds = 0
            while True:
                ann_state = R1.candidate[1][0]
                start = R2.belief.copy(); start.agent, start.obj = ann_state.agent, ann_state.obj
                if not valid(start):                                    # impossible in R2's belief: that is about
                    self.log(f"Step {self.decision}: R2 look-ahead skipped (start state invalid in its own belief)")
                    break                                               # the immediate step, outside its territory
                la = self.lookahead(p2, start=start)
                if la is None: break
                kind, k, a, info, extra = la
                self.msg(); self.log(f"Step {self.decision}: R2 context OBJECTION at look-ahead step {k} ({a}); R1 accepts as a planning constraint")
                if kind == 'trap': R1.forbidden.add(info)
                else: R1.excluded.add((extra[2].agent, extra[2].obj, a))
                R1.plan = []; R1.plan_kind = None
                rounds += 1
                prop = R1.propose()
                if prop[0] == 'stop': return self.stop(prop[1])
                a1, p1 = R1.candidate; final = a1
                p2 = R2.pred(R2.belief, a1, use_reserve=True)
                if p2 is None: return self.stop('no budget for a committed prediction')
                if rounds >= 2: break
        q1 = p1 if final == a1 else R1.pred(R1.belief, final, use_reserve=True)
        q2 = p2 if final == a1 else R2.pred(R2.belief, final, use_reserve=True)
        if q1 is None or q2 is None: return self.stop('no budget for a committed prediction')
        R1.retain(final, q1); R2.retain(final, q2)
        return final

    def run(self, title=None):
        if title: self.log(""); self.log(title)
        env = self.true.copy(); result = None
        if env.obj == env.dest: result = 'Success'
        while result is None:
            a = self.decide()
            if a is None:
                self.obs = {'step': self.decision + 1, 'state': env.copy(), 'action': None, 'outcome': None}
                if self.safe_stops >= self.S_max: result = 'Stopped -- unresolved'
                continue
            env, out = apply_action(env, a); self.steps += 1
            self.log(f"Step {self.decision}: {a} -> {out}")
            self.obs = {'step': self.decision + 1, 'state': env.copy(), 'action': a, 'outcome': out}
            if env.obj == env.dest or self.steps >= self.T_max:
                for X in (self.R1, self.R2): X.final_check(self.obs)
                result = 'Success' if env.obj == env.dest else 'Step limit'
        self.log(f"Result: {result} after {self.steps} world steps, {self.safe_stops} safe stops")
        self.log(f"Evaluations: R1 {self.R1.evals}, R2 {self.R2.evals}; messages {self.messages}")
        return result


def counterfactual(pre, a, model, fault, other_pred):
    """A1 (13.9): restore ONLY this fault's element in this reasoner's prediction inputs,
    hold everything else fixed, recompute, and compare on the same cells.
    fault = ('wall', c, true) | ('object', None, true) | ('model', action, None)."""
    kind, c, v = fault
    p = pre.copy(); fn = model
    if kind == 'wall': p.walls[c[1]][c[0]] = v
    elif kind == 'object': p.obj = v
    elif kind == 'model': fn = apply_action
    return fn(p, a)
