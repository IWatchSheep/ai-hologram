"""Stage 4 scoring reference (Python; not project code): plan 3.7-3.11.
Scores C0/C1 runs with faults from the agent's own records, with phases A/B, claims, event life,
mismatch causes, source labels, and shadow evaluation (effective use, opportunities)."""
import sys
sys.path.insert(0, '.')
import ref_c1
from ref_stage4 import *
from ref_c1 import contents

LABEL = {'F1': 'S2', 'F2': 'S2', 'F5': 'S2', 'F3': 'S1', 'F4': 'S1'}
CURRENT = {'scorer': None}          # the scorer observing computations right now (or None)
_orig_heuristic = ref_c1.heuristic


def observed_heuristic(s, dead, pd):
    v = _orig_heuristic(s, dead, pd)
    sc = CURRENT['scorer']
    if sc is not None: sc.on_heuristic(s, v)
    return v


ref_c1.heuristic = observed_heuristic


class DeadProxy:
    """A read of one cell's dead status (plan 3.9: dead-status reads are observed)."""
    def __init__(self, d): self.d = d

    def __getitem__(self, c):
        v = self.d[c]
        sc = CURRENT['scorer']
        if sc is not None: sc.on_dead_read(c, v)
        return v


def element(ev):
    t = ev['type']
    if t == 'F1': return ('wall', ev['cell'])
    if t in ('F2', 'F5'): return ('object',)
    if t == 'F3': return ('model', ev['action'])
    return ('trap', ev['cell'])


def claim_element(rec):
    k = rec['kind']
    if k == 'object': return ('object',)
    return (k, rec['el'])


def fp2(pre1, out1, pre2, out2, a):
    d = DIRV[a]; c0 = pre1.agent; c1 = add(c0, d); cells = [c0, c1]
    if pre1.obj == c1 or pre2.obj == c1 or out1 == 'Pushed' or out2 == 'Pushed':
        cells.append(add(c1, d))
    return cells


def differ(r1, r2, pre1, pre2, a):
    if r1[1] != r2[1]: return True
    return any(contents(r1[0], c) != contents(r2[0], c) for c in fp2(pre1, r1[1], pre2, r2[1], a))


class EventScore:
    def __init__(self, ev):
        self.ev = ev; self.el = element(ev)
        self.activated = False; self.act_step = None
        self.active = False; self.ended = None          # (state, step, record id or None)
        self.opp_step = None; self.revealing = False
        self.mismatch_step = None; self.strict_step = None
        self.correcting = None                          # correction record that ended it


class Scorer:
    def __init__(self, ag, events, hist, true_layout):
        self.ag = ag; self.hist = hist; self.true = true_layout
        self.ev = [EventScore(e) for e in events]
        self.true_dead, _ = dead_map(true_layout)
        self.seen_corr = 0; self.seen_status = {}; self.seen_mism = 0
        self.claims = []; self.false_alarms = 0; self.wrong_accepted = 0
        self.mism = []                                  # (step, label, cause ids)
        self.labels_correct = 0; self.labels_scored = 0; self.no_cause = 0
        self.justified = 0; self.committed_causes = []
        self.step = 0; self.observe = True

    # ---------- helpers ----------
    def true_state(self): return self.hist[-1]['state']

    def is_active_now(self, e):
        b = self.ag.belief; T = self.true_state(); t = e.ev['type']
        if t == 'F1': c = e.ev['cell']; return b.wall(c) != self.true.wall(c)
        if t in ('F2', 'F5'): return b.obj != T.obj
        if t == 'F3': return e.ev['action'] not in self.ag.model.restored
        return e.ev['cell'] not in getattr(self.ag, 'forced_dead', [])

    def live(self):
        return [e for e in self.ev if e.activated and e.ended is None and e.active]

    def value_true(self, rec):
        k = rec['kind']
        if k == 'wall': return rec['value'] == self.true.wall(rec['el'])
        if k == 'object':
            # An object correction says where the object was just before the investigated action
            # (its V1 check is time-aware). Its reference is the latest mismatch record: the
            # observation before the checked one (decision step s compares o_s with the
            # prediction from s-1, so the pre-action observation is o_(s-1) = hist[s-2]).
            recs = getattr(self.ag, 'mism_records', [])
            if not recs: return False
            s = recs[-1][0]
            return rec['value'] == self.hist[s - 2]['state'].obj
        if k == 'model': return True
        return self.true_dead[rec['el']]

    # ---------- phase boundaries ----------
    def end_phase(self, phase):
        ag = self.ag
        start_active = {id(e): (e.activated and e.ended is None and e.active) for e in self.ev}
        corrs = getattr(ag, 'corrections', [])
        new_claims = corrs[self.seen_corr:]; self.seen_corr = len(corrs)
        for rec in new_claims:
            el = claim_element(rec); ok = self.value_true(rec)
            rec['_true'] = ok; rec['_step'] = self.step
            self.claims.append(rec)
            if not ok: self.wrong_accepted += 1
            hit = [e for e in self.ev if e.el == el and start_active[id(e)]]
            if not hit: self.false_alarms += 1
            for e in hit:
                if e.strict_step is None: e.strict_step = self.step
        # new mismatch records (phase A only; the check happens there)
        for (step, label) in getattr(ag, 'mism_records', [])[self.seen_mism:]:
            causes = [e for e in self.committed_causes if start_active[id(e)]]
            self.mism.append((step, label, [e.ev['id'] for e in causes]))
            for e in causes:
                if e.mismatch_step is None: e.mismatch_step = step
            if not causes: self.no_cause += 1
            else:
                self.labels_scored += 1
                if label in {LABEL[e.ev['type']] for e in causes}: self.labels_correct += 1
        self.seen_mism = len(getattr(ag, 'mism_records', []))
        if new_claims or self.seen_mism: pass
        # event life
        for e in self.ev:
            if not e.activated: continue
            now = self.is_active_now(e)
            if e.ended is None and e.active and not now:
                fix = [r for r in new_claims if claim_element(r) == e.el and r['_true']]
                if fix:
                    e.correcting = fix[-1]; e.ended = ('Corrected', self.step)
                elif e.ev['type'] in ('F3', 'F4'):
                    recs = [r for r in corrs if claim_element(r) == e.el and r['status'] != 'REJECTED']
                    e.correcting = recs[-1] if recs else None; e.ended = ('Corrected', self.step)
                else:
                    e.ended = ('Overwritten without correction', self.step)
                e.active = False
            elif e.ended is not None and e.ended[0] == 'Corrected' and e.correcting is not None \
                    and e.correcting['status'] == 'REJECTED' and now:
                e.ended = None; e.active = True; e.correcting = None
        self.committed_causes = []

    def after_activation(self, step):
        for e in self.ev:
            r = self.ag.injector.rec[e.ev['id']]
            if r['status'] == 'applied' and not e.activated:
                e.activated = True; e.act_step = step; e.active = True
                if not self.is_active_now(e):
                    e.active = False; e.ended = ('Overwritten without correction', step)

    def after_decide(self, decision):
        self.end_phase('B')
        if decision[0] == 'stop':
            if any(e.activated and e.ended is None and e.active for e in self.ev): self.justified += 1
            return
        p = getattr(self.ag, 'pending', None)
        if p is None: return
        pstate, pout, a, pre, step, tests = p
        r = (pstate, pout)
        for e in self.live():
            sh = self.shadow_eval(e, pre, a, own=True, model_fn=None, result=r)
            if sh:
                self.committed_causes.append(e); e.revealing = True
                if e.opp_step is None: e.opp_step = self.step

    # ---------- shadow evaluation ----------
    def shadow_state(self, e, s):
        t = e.ev['type']
        if t == 'F1':
            c = e.ev['cell']; v = self.true.wall(c)
            if s.wall(c) == v: return s, False
            n = s.copy(); n.walls[c[1]][c[0]] = v; return n, True
        if t in ('F2', 'F5'):
            b = self.ag.belief; T = self.true_state()
            if b.obj == T.obj or s.obj != b.obj: return s, False
            n = s.copy(); n.obj = T.obj; return n, True
        return s, False

    def shadow_eval(self, e, s, a, own, model_fn, result):
        t = e.ev['type']; ag = self.ag
        fn = model_fn or ag.model.fn(a)
        if t == 'F3':
            # v1.2.2: any evaluation of the faulted action while it is not restored; effective if its
            # result differs from the standard rule's (a standard-rule variant never differs).
            if a != e.ev['action'] or a in ag.model.restored: return False
            return differ(result, apply_action(s, a), s, s, a)
        if t == 'F4': return False
        n, changed = self.shadow_state(e, s)
        if not changed: return False
        if not valid(n): return True
        return differ(result, fn(n, a), s, n, a)

    def shadow_map(self, e):
        ag = self.ag; t = e.ev['type']
        if t == 'F1':
            n, changed = self.shadow_state(e, ag.belief)
            if not changed: return None
            return dead_map(n, ag.not_dead, getattr(ag, 'forced_dead', ()))
        if t == 'F4':
            nd = list(ag.not_dead)
            if e.ev['cell'] in nd: nd.remove(e.ev['cell'])
            return dead_map(ag.belief, nd, getattr(ag, 'forced_dead', ()))
        return None

    def mark(self, e):
        if e.opp_step is None: e.opp_step = self.step

    def on_eval(self, s, a, own, model_fn, result):
        if not self.observe: return
        self.n_evals_seen = getattr(self, 'n_evals_seen', 0) + 1
        for e in self.live():
            if e.opp_step is None and self.shadow_eval(e, s, a, own, model_fn, result): self.mark(e)

    def on_heuristic(self, s, v):
        if not self.observe: return
        for e in self.live():
            if e.opp_step is not None: continue
            t = e.ev['type']
            if t in ('F1', 'F4'):
                m = self.shadow_map(e)
                if m is not None and _orig_heuristic(s, m[0], m[1]) != v: self.mark(e)
            elif t in ('F2', 'F5'):
                n, changed = self.shadow_state(e, s)
                if not changed: continue
                if not valid(n): self.mark(e); continue
                if _orig_heuristic(n, self.ag._fresh()[0], self.ag._fresh()[1]) != v: self.mark(e)

    def on_dead_read(self, c, v):
        if not self.observe: return
        for e in self.live():
            if e.opp_step is not None or e.ev['type'] not in ('F1', 'F4'): continue
            m = self.shadow_map(e)
            if m is not None and m[0][c] != v: self.mark(e)


# ---------- agents with phases and observation ----------
class Observed:
    def _activation(self):
        sc = self.scorer
        if sc: sc.end_phase('A')
        if self.injector: self.injector.at_activation_point(self, self.decision)
        if sc: sc.after_activation(self.decision)

    @property
    def dead(self):
        return DeadProxy(self._fresh()[0])

    @dead.setter
    def dead(self, v): pass

    def predict(self, state, a, model_fn=None, cache=True):
        before = self.evals
        r = super().predict(state, a, model_fn, cache)
        if self.evals > before and self.scorer is not None:
            prev = CURRENT['scorer']; CURRENT['scorer'] = None
            self.scorer.on_eval(state, a, model_fn is None, model_fn, r)
            CURRENT['scorer'] = prev
        return r


class C1S(Observed, C1F):
    def step(self, obs):
        self.decision += 1
        if self.scorer: self.scorer.step = self.decision
        self.budget = Budget(self.B); self.cache = {}
        if self.decision > 1: self.history.append(obs)
        self.check(obs)
        if self.pending_inv:
            args = self.pending_inv; self.pending_inv = None; self.investigate(*args)
            if self.pending_inv:
                self._activation()
                return self.safe_stop('budget too small for verification')
        if self.pending_trap: self.trap_v2(self.pending_trap)
        if self.pending_v1: self.continue_v1(obs)
        self._activation()
        return self.choose()

    def check(self, obs):
        before = self.mismatches; n0 = len(self._lines)
        super().check(obs)
        if self.mismatches > before:
            lines = self._lines[n0:]
            if any('(source S1)' in l for l in lines): lab = 'S1'
            elif any('(source S5)' in l for l in lines): lab = 'S5'
            else: lab = 'S2'
            self.mism_records.append((self.decision, lab))


class C0S(Observed, C0F):
    def step(self, obs):
        self.decision += 1
        if self.scorer: self.scorer.step = self.decision
        self._activation()
        return C0.step(self, obs)


def score_run(cond, s, events, observe=True, B=200, S_max=5, T_max=100, belief=None):
    o = {'step': 1, 'state': s.copy(), 'action': None, 'outcome': None}
    hist = [o]
    lines = []
    if cond == 'C1':
        ag = C1S.__new__(C1S); ag._lines = lines; ag.mism_records = []; ag.scorer = None
        C1F.__init__(ag, o, B=B, S_max=S_max, T_max=T_max, log=lines.append, belief=belief)
    else:
        ag = C0S.__new__(C0S); ag.scorer = None; ag.mism_records = []
        C0F.__init__(ag, o, B=B)
    ag.rotations = {}
    ag.injector = Injector(events, hist)
    sc = Scorer(ag, events, hist, s); sc.observe = observe
    ag.scorer = sc
    env = s.copy(); steps = 0; stops = 0
    while True:
        CURRENT['scorer'] = sc if observe else None
        d = ag.step(o)
        CURRENT['scorer'] = None
        sc.after_decide(d)
        if d[0] == 'act':
            env, out = apply_action(env, d[1]); steps += 1
            o = {'step': ag.decision + 1, 'state': env.copy(), 'action': d[1], 'outcome': out}; hist.append(o)
            if env.obj == env.dest or steps >= T_max:
                if cond == 'C1':
                    CURRENT['scorer'] = sc if observe else None
                    ag.final_check(o)
                    CURRENT['scorer'] = None
                    sc.end_phase('F')
                res = 'Success' if env.obj == env.dest else 'Step limit'; break
        else:
            stops += 1
            o = {'step': ag.decision + 1, 'state': env.copy(), 'action': None, 'outcome': None}; hist.append(o)
            if stops >= S_max: res = 'Stopped -- unresolved'; break
    return {'agent': ag, 'scorer': sc, 'outcome': res, 'world_steps': steps, 'decisions': ag.decision,
            'safe_stops': stops, 'records': ag.injector.finish(), 'evals': ag.evals, 'lines': lines}
