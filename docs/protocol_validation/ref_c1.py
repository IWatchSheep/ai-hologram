"""Reference implementation of the Stage 3 C1/C0 design (Phase B Section 11).
Used only to validate the plan and produce exact expected output. Not project code."""
import heapq

N = 10
DIRS = [('North', (0, -1)), ('East', (1, 0)), ('South', (0, 1)), ('West', (-1, 0))]
DIRV = dict(DIRS)
INF = 10**9


def add(p, d, k=1):
    return (p[0] + d[0] * k, p[1] + d[1] * k)


class W:  # world state
    def __init__(self, walls, agent, obj, dest):
        self.walls = [row[:] for row in walls]
        self.agent, self.obj, self.dest = agent, obj, dest

    def copy(self):
        return W(self.walls, self.agent, self.obj, self.dest)

    def wall(self, c):
        x, y = c
        if not (0 <= x < N and 0 <= y < N):
            return True
        return self.walls[y][x]

    def key(self):
        return (self.agent, self.obj)

    def same(self, o):
        return self.walls == o.walls and self.key() == o.key() and self.dest == o.dest


def parse(lines):
    walls = [[False] * N for _ in range(N)]
    agent = obj = dest = None
    for y, line in enumerate(lines):
        for x, ch in enumerate(line):
            if ch == '#': walls[y][x] = True
            if ch in 'A+': agent = (x, y)
            if ch in 'O*': obj = (x, y)
            if ch in 'D*+': dest = (x, y)
    return W(walls, agent, obj, dest)


def valid(s):
    for y in range(N):
        for x in range(N):
            if (x in (0, N - 1) or y in (0, N - 1)) and not s.walls[y][x]:
                return False
    for p in (s.agent, s.obj, s.dest):
        if s.wall(p): return False
    return s.agent != s.obj


def apply_action(s, a):
    if not valid(s): return s.copy(), 'InvalidState'
    d = DIRV[a]; t1 = add(s.agent, d)
    if s.wall(t1): return s.copy(), 'Blocked'
    if t1 == s.obj:
        t2 = add(t1, d)
        if s.wall(t2): return s.copy(), 'Blocked'
        n = s.copy(); n.agent, n.obj = t1, t2; return n, 'Pushed'
    n = s.copy(); n.agent = t1; return n, 'Moved'


class Budget:
    def __init__(self, limit): self.limit = max(0, limit); self.used = 0
    def try_use(self):
        if self.used < self.limit: self.used += 1; return True
        return False
    def remaining(self): return self.limit - self.used


# ---------------- dead squares (Phase B 11.2) ----------------
def dead_map(walls_state, not_dead_overrides=(), forced_dead=()):
    s = walls_state
    pd = {}
    free = lambda c: not s.wall(c)
    from collections import deque
    pd[s.dest] = 0; q = deque([s.dest])
    while q:
        r = q.popleft()
        for _, d in DIRS:
            qq = add(r, d, -1); ag = add(r, d, -2)
            if free(qq) and free(ag) and qq not in pd:
                pd[qq] = pd[r] + 1; q.append(qq)
    dead = {}
    for y in range(N):
        for x in range(N):
            c = (x, y)
            dead[c] = free(c) and c not in pd
    for p in not_dead_overrides: dead[p] = False   # test-only (F4 in Stage 4)
    for p in forced_dead: dead[p] = True           # applied corrections
    return dead, pd


def local_check_not_dead(s, dead, p):
    if p == s.dest: return True
    for _, d in DIRS:
        b, f = add(p, d, -1), add(p, d)
        if not s.wall(b) and not s.wall(f) and not dead[f]:
            return True
    return False


def manhattan(a, b): return abs(a[0] - b[0]) + abs(a[1] - b[1])


def heuristic(s, dead, pd):
    if dead[s.obj]: return INF
    p = pd.get(s.obj, manhattan(s.obj, s.dest))
    return p + max(0, manhattan(s.agent, s.obj) - 1)


# ---------------- footprint (Phase B 10.5) ----------------
def footprint(pre, a, post_obs, outcome_obs):
    d = DIRV[a]; c0 = pre.agent; c1 = add(c0, d)
    cells = [c0, c1]
    if pre.obj == c1 or outcome_obs == 'Pushed' or post_obs.obj == c1:
        cells.append(add(c1, d))
    return cells


def contents(s, c): return (s.wall(c), s.agent == c, s.obj == c)


def compare_fp(pred_state, pred_out, obs_state, obs_out, cells):
    diffs = []
    for c in cells:
        if contents(pred_state, c) != contents(obs_state, c): diffs.append(c)
    return (pred_out != obs_out), diffs


def fp_update(belief, cells, obs):
    for c in cells:
        x, y = c
        if 0 <= x < N and 0 <= y < N: belief.walls[y][x] = obs.walls[y][x]
    if obs.agent in cells: belief.agent = obs.agent
    if obs.obj in cells or belief.obj in cells: belief.obj = obs.obj


# ---------------- reasoner model with per-step cache ----------------
class Model:
    def __init__(self, base=None):
        self.base = base or apply_action
        self.restored = set()  # actions restored to the standard rule by correction
    def fn(self, a):
        return apply_action if a in self.restored else self.base


class C1:
    def __init__(self, first_obs, B=200, S_max=5, T_max=100, T_v=3, base_model=None,
                 belief=None, not_dead_overrides=(), log=print):
        self.B, self.S_max, self.T_max, self.T_v = B, S_max, T_max, T_v
        self.belief = (belief or first_obs['state']).copy()
        self.history = [first_obs]
        self.model = Model(base_model)
        self.not_dead = list(not_dead_overrides); self.forced_dead = []
        self.recompute_dead()
        self.plan = []; self.plan_kind = None
        self.pending = None   # (prediction_state, outcome, action, belief_pre, step, v2_tests)
        self.corrections = []; self.mismatches = 0
        self.safe_stops = 0; self.decision = 0; self.evals = 0
        self.pending_v1 = None; self.pending_inv = None; self.pending_trap = None
        self.log = log; self.world_steps = 0

    # budget-charged prediction with per-step cache
    def predict(self, state, a, model_fn=None, cache=True):
        key = (state.agent, state.obj, a, self.walls_version)
        if cache and model_fn is None and key in self.cache:
            return self.cache[key]
        if not self.budget.try_use(): return None
        self.evals += 1
        r = (model_fn or self.model.fn(a))(state, a)
        if cache and model_fn is None: self.cache[key] = r
        return r

    def reserve(self):
        return 1 + len([c for c in self.corrections if c['status'] == 'CONSISTENT' and c['kind'] in ('wall', 'object', 'model')])

    def can_spend(self):
        # an optional evaluation is allowed only if it leaves at least the reserve
        return self.budget.remaining() > self.reserve()

    def recompute_dead(self):
        self.dead, self.pd = dead_map(self.belief, self.not_dead, self.forced_dead)
        self.walls_version = repr(self.belief.walls) + repr(self.forced_dead)

    # ---------------- A* planner (11.2) ----------------
    def search(self, root, goal, hfun, reserve):
        """A* (v1.1): best g per state, goal accepted when taken from the queue,
        stale queue entries skipped, closed states reopened if a shorter path is found."""
        import itertools
        rk = root.key()
        info = {rk: {'state': root.copy(), 'g': 0, 'h': hfun(root), 'parent': None, 'action': None, 'disc': 0}}
        order = [rk]                # discovery order (for fallback ties)
        heap = [(info[rk]['h'], info[rk]['h'], 0, rk)]
        counter = itertools.count(1)
        closed = set()
        self.last_search_reopened = 0
        while heap:
            f, h, _, k = heapq.heappop(heap)
            node = info[k]
            if f != node['g'] + node['h']: continue          # stale entry
            if k in closed: continue
            if goal(node['state']): return 'found', self.path(info, k), (info, order)
            closed.add(k)
            st, g = node['state'], node['g']
            for a, _ in DIRS:
                key = (st.agent, st.obj, a, self.walls_version)
                if key not in self.cache and self.budget.remaining() <= reserve:
                    return 'budget', None, (info, order)
                ns, out = self.predict(st, a)
                if out in ('Blocked', 'InvalidState'): continue
                if self.dead[ns.obj]: continue
                nk = ns.key(); ng = g + 1
                if nk in info and ng >= info[nk]['g']: continue
                if nk not in info:
                    info[nk] = {'state': ns, 'h': hfun(ns), 'disc': len(order)}; order.append(nk)
                elif nk in closed:
                    closed.discard(nk); self.last_search_reopened += 1
                info[nk].update({'g': ng, 'parent': k, 'action': a, 'state': ns})
                heapq.heappush(heap, (ng + info[nk]['h'], info[nk]['h'], next(counter), nk))
        return 'exhausted', None, (info, order)

    def path(self, info, k):
        out = []
        while info[k]['parent'] is not None:
            out.append((info[k]['action'], info[k]['state'])); k = info[k]['parent']
        return list(reversed(out))

    def fallback(self, nodes):
        info, order = nodes
        best = None
        for k in order[1:]:
            n = info[k]; key = (n['h'], n['g'], n['disc'])
            if n['h'] < INF and (best is None or key < best): best = key; bk = k
        return None if best is None else self.path(info, bk)

    # ---------------- decision step (11.3) ----------------
    def step(self, obs):
        """Returns ('act', action) or ('stop', reason). obs = observation for this step."""
        self.decision += 1
        self.budget = Budget(self.B); self.cache = {}
        if self.decision > 1: self.history.append(obs)
        self.check(obs)
        if self.pending_inv:
            args = self.pending_inv; self.pending_inv = None; self.investigate(*args)
            if self.pending_inv: return self.safe_stop('budget too small for verification')
        if self.pending_trap: self.trap_v2(self.pending_trap)
        # (fault activation point: nothing in Stage 3)
        if self.pending_v1: self.continue_v1(obs)
        return self.choose()

    def final_check(self, obs):
        self.budget = Budget(self.B); self.cache = {}
        self.history.append(obs); self.check(obs)

    def check(self, obs):
        if self.pending is None: return
        pstate, pout, a, pre, step, tests = self.pending
        self.pending = None
        cells = footprint(pre, a, obs['state'], obs['outcome'])
        od, diffs = compare_fp(pstate, pout, obs['state'], obs['outcome'], cells)
        match = not od and not diffs
        self.log(f"Step {step}: {a} | predicted {pout}, observed {obs['outcome']} | {'match' if match else 'MISMATCH'}")
        for cid in tests:
            c = self.corrections[cid - 1]
            if c['status'] == 'CONSISTENT':
                if match:
                    c['status'] = 'VERIFIED'; self.log(f"  - correction {cid}: VERIFIED")
                else:
                    c['status'] = 'REJECTED'; self.revert(c); self.log(f"  - correction {cid}: REJECTED")
        if match:
            fp_update(self.belief, cells, obs['state'])
        else:
            self.mismatches += 1
            self.investigate(pre, a, obs, cells)

    # ---------------- investigation (11.4) ----------------
    def investigate(self, pre, a, obs, cells):
        prev = self.history[-2]['state']
        cands = []
        for c in cells:
            if pre.wall(c) != prev.wall(c):
                cands.append(('wall', c, prev.wall(c)))
        if (pre.obj in cells or prev.obj in cells) and pre.obj != prev.obj:
            cands.append(('object', None, prev.obj))
        if cands:
            for kind, c, v in (cands if not getattr(self, '_retrying', False) else []):
                if kind == 'wall':
                    self.log(f"  - investigation: wall at {fmt(c)} believed {'blocked' if pre.wall(c) else 'free'}, recorded {'blocked' if v else 'free'} (source S2)")
                else:
                    self.log(f"  - investigation: object believed at {fmt(pre.obj)}, recorded at {fmt(v)} (source S2)")
            corrected = pre.copy()
            for kind, c, v in cands:
                if kind == 'wall': corrected.walls[c[1]][c[0]] = v
                else: corrected.obj = v
            ok_hist = all(self.v1_history(k, c, v) for k, c, v in cands)
            if not self.can_spend():
                self.log("  - investigation: budget too small for verification; retried next step")
                self.pending_inv = (pre, a, obs, cells); return
            r = self.predict(corrected, a, cache=False)
            ok_re = False
            if r is not None:
                cc = footprint(corrected, a, obs['state'], obs['outcome'])
                od, dd = compare_fp(r[0], r[1], obs['state'], obs['outcome'], cc)
                ok_re = not od and not dd
            if ok_hist and ok_re:
                for kind, c, v in cands:
                    self.apply_correction(kind, c, v, old=(pre.wall(c) if kind == 'wall' else pre.obj))
            else:
                self.log("  - investigation: no candidate passed V1 (source S5); unexplained update")
            fp_update(self.belief, cells, obs['state'])
            if self.belief.obj != obs['state'].obj and (self.belief.obj in cells): self.belief.obj = obs['state'].obj
        else:
            self.log(f"  - investigation: belief matched the record; action model for {a} suspected (source S1)")
            trans = [k for k in range(1, len(self.history)) if self.history[k]['action'] == a]
            self.pending_v1 = {'action': a, 'todo': trans, 'cells': cells, 'obs': obs}
            # belief before the action matched the record, so the footprint is updated now
            # (provisional: if V1 fails it is logged as an unexplained update)
            fp_update(self.belief, cells, obs['state'])
            self.continue_v1(obs)

    def v1_history(self, kind, c, v):
        if kind == 'wall':
            return all(h['state'].wall(c) == v for h in self.history)
        return self.history[-2]['state'].obj == v

    def continue_v1(self, obs):
        pv = self.pending_v1
        while pv['todo']:
            if not self.can_spend(): return  # keep the reserve; continue next step
            k = pv['todo'][0]
            r = self.predict(self.history[k - 1]['state'], pv['action'], model_fn=apply_action, cache=False)
            ok = r is not None and r[1] == self.history[k]['outcome'] and r[0].same(self.history[k]['state'])
            pv['todo'].pop(0)
            if not ok:
                self.log("  - investigation: no candidate passed V1 (source S5); unexplained update")
                self.pending_v1 = None; return
        self.pending_v1 = None
        old = self.model.fn(pv['action'])
        self.model.restored.add(pv['action'])
        self.cache = {}
        self.apply_correction('model', pv['action'], 'standard', old=old)

    def apply_correction(self, kind, c, v, old):
        cid = len(self.corrections) + 1
        rec = {'id': cid, 'kind': kind, 'el': c, 'value': v, 'old': old, 'status': 'CONSISTENT',
               'since': self.decision, 'tested': False}
        self.corrections.append(rec)
        if kind == 'wall':
            self.belief.walls[c[1]][c[0]] = v; self.recompute_dead(); self.cache = {}
            self.log(f"  - correction {cid}: wall at {fmt(c)} set to {'blocked' if v else 'free'}: CONSISTENT")
        elif kind == 'object':
            self.belief.obj = v
            self.log(f"  - correction {cid}: object position set to {fmt(v)}: CONSISTENT")
        elif kind == 'model':
            self.log(f"  - correction {cid}: action model for {c} restored to the standard rule: CONSISTENT")
        elif kind == 'trap':
            self.forced_dead.append(c); self.recompute_dead(); self.cache = {}
            self.log(f"  - correction {cid}: {fmt(c)} classified as a dead square: CONSISTENT")
            self.trap_v2(rec)

    def revert(self, c):
        if c['kind'] == 'wall':
            self.belief.walls[c['el'][1]][c['el'][0]] = c['old']; self.recompute_dead()
        elif c['kind'] == 'object': pass  # old value no longer meaningful; record only
        elif c['kind'] == 'model': self.model.restored.discard(c['el'])
        elif c['kind'] == 'trap': self.forced_dead.remove(c['el']); self.recompute_dead()

    def trap_v2(self, rec):
        p = rec['el']; ok = True
        self.pending_trap = None
        hyps = []
        for a, d in DIRS:
            b = add(p, d, -1)
            if self.belief.wall(b): continue
            hyp = self.belief.copy(); hyp.agent, hyp.obj = b, p
            if not valid(hyp):
                self.log("  - anomaly: invalid hypothetical state skipped"); continue
            hyps.append((hyp, a))
        if self.budget.remaining() - len(hyps) < self.reserve():
            self.pending_trap = rec; return      # whole test next step, keeping the reserve
        for hyp, a in hyps:
            r = self.predict(hyp, a, cache=False)
            if r[1] == 'Pushed' and not self.dead[r[0].obj]: ok = False
        rec['status'] = 'VERIFIED' if ok else 'REJECTED'
        if not ok: self.revert(rec)
        self.log(f"  - correction {rec['id']}: {rec['status']}")

    # ---------------- choosing (11.3 steps 5-7) ----------------
    def choose(self):
        if self.pending_v1:
            return self.cautious()
        reserve = self.reserve()
        # active test
        if self.plan_kind != 'test':
            for c in self.corrections:
                if c['status'] == 'CONSISTENT' and c['kind'] == 'wall' and self.decision - c['since'] >= self.T_v:
                    tp = self.test_plan(c, reserve)
                    if tp is not None:
                        self.plan, self.plan_kind = tp, 'test'
                        self.log(f"Plan: active test of correction {c['id']} ({len(tp)} actions)")
                        break
        # keep current plan?
        if self.plan:
            a, exp = self.plan[0]
            r = self.predict(self.belief, a)
            if r is None:
                self.plan = []; self.plan_kind = None
                return self.safe_stop('no budget for a committed prediction')
            if r[0].same(exp) and not self.dead[r[0].obj]:
                return self.commit(a, r)
            self.plan = []; self.plan_kind = None
        kind, plan, nodes = self.search(self.belief, lambda s: s.obj == s.dest,
                                        lambda s: heuristic(s, self.dead, self.pd), reserve)
        if kind == 'found' and not plan:
            return self.safe_stop('belief shows goal reached')
        if kind == 'found':
            self.plan, self.plan_kind = plan, 'goal'
            self.log(f"Plan: search found a plan of {len(plan)} actions")
        else:
            if kind == 'exhausted': self.surprise()
            fb = self.fallback(nodes)
            if not fb: return self.safe_stop('budget too small to plan' if kind == 'budget' else 'no safe action')
            self.plan, self.plan_kind = fb, 'fallback'
            self.log(f"Plan: fallback toward the best state found ({len(fb)} actions)")
        a, exp = self.plan[0]
        r = self.predict(self.belief, a)
        if r is None:
            self.plan = []; self.plan_kind = None
            return self.safe_stop('no budget for a committed prediction')
        return self.commit(a, r)

    def test_plan(self, c, reserve):
        cell = c['el']; targets = []
        for a, d in DIRS:
            n = add(cell, d, -1)
            if not self.belief.wall(n) and n != self.belief.obj: targets.append((n, a))
        if not targets: return None
        tcells = [t for t, _ in targets]
        kind, plan, _ = self.search(self.belief, lambda s: s.agent in tcells,
                                    lambda s: (INF if self.dead[s.obj] else min(manhattan(s.agent, t) for t in tcells)), reserve)
        if kind != 'found': return None
        end = plan[-1][1] if plan else self.belief
        act = dict(targets)[end.agent]
        key = (end.agent, end.obj, act, self.walls_version)
        if key not in self.cache and not self.can_spend(): return None
        r = self.predict(end, act)
        return plan + [(act, r[0])]

    def commit(self, a, r):
        self.plan.pop(0)
        if not self.plan: self.plan_kind = None
        tests = []
        for c in self.corrections:
            if c['status'] != 'CONSISTENT' or c['tested']: continue
            alt = self.alt_prediction(c, a)
            if alt is None: continue
            if alt[1] != r[1] or alt[0].key() != r[0].key():
                tests.append(c['id']); c['tested'] = True
        self.pending = (r[0], r[1], a, self.belief.copy(), self.decision, tests)
        self.world_steps += 1
        return ('act', a)

    def alt_prediction(self, c, a):
        d = DIRV[a]; c1 = add(self.belief.agent, d); c2 = add(c1, d)
        if c['kind'] == 'wall':
            if c['el'] not in (c1, c2): return None
            alt = self.belief.copy(); alt.walls[c['el'][1]][c['el'][0]] = c['old']
            if not valid(alt): return None
            return self.predict(alt, a, cache=False)
        if c['kind'] == 'model':
            if c['el'] != a: return None
            return self.predict(self.belief, a, model_fn=c['old'], cache=False)
        if c['kind'] == 'object':
            if c['old'] not in (c1, c2) and self.belief.obj not in (c1, c2): return None
            alt = self.belief.copy(); alt.obj = c['old']
            if not valid(alt): return None
            return self.predict(alt, a, cache=False)
        return None

    def cautious(self):
        pv = self.pending_v1
        for a, _ in DIRS:
            need = (0 if (self.belief.agent, self.belief.obj, a, self.walls_version) in self.cache else 1) + 1
            if self.budget.remaining() - need < self.reserve(): break
            r1 = self.predict(self.belief, a); r2 = self.predict(self.belief, a, model_fn=apply_action, cache=False)
            if r1[1] == r2[1] and r1[0].key() == r2[0].key() and not self.dead[r1[0].obj]:
                self.log("Plan: cautious mode (investigation unfinished)")
                self.plan = [(a, r1[0])]
                return self.commit(a, r1)
        return self.safe_stop('investigation unfinished, no cautious action')

    def surprise(self):
        p = self.belief.obj
        if self.dead[p] or p == self.belief.dest: return
        self.log(f"  - consequence surprise: search exhausted with the object at {fmt(p)}")
        if local_check_not_dead(self.belief, self.dead, p):
            self.log("  - full-state dead end (dead-square classification consistent; no claim)")
            return
        hist_ok = not any(h['state'].obj == p and any(g['state'].obj == self.belief.dest for g in self.history[i:])
                          for i, h in enumerate(self.history))
        if hist_ok:
            self.log(f"  - consequence mismatch: {fmt(p)} violates the dead-square rule (source S1)")
            self.apply_correction('trap', p, True, old=False)

    def safe_stop(self, reason):
        self.safe_stops += 1
        self.log(f"Step {self.decision}: safe stop ({reason})")
        return ('stop', reason)


def fmt(p): return f"({p[0]}, {p[1]})"


def run_c1(true_state, title, log, **kw):
    log(""); log(title)
    o = {'step': 1, 'state': true_state.copy(), 'action': None, 'outcome': None}
    agent = C1(o, log=log, **kw)
    env = true_state.copy(); steps = 0; result = None
    if env.obj == env.dest:
        log("Result: Success after 0 world steps, 0 safe stops"); return None, 'Success', 0
    while True:
        dec = agent.step(o)
        if dec[0] == 'act':
            env, out = apply_action(env, dec[1]); steps += 1
            o = {'step': agent.decision + 1, 'state': env.copy(), 'action': dec[1], 'outcome': out}
            if env.obj == env.dest:
                agent.final_check(o); result = 'Success'; break
            if steps >= agent.T_max:
                agent.final_check(o); result = 'Step limit'; break
        else:
            o = {'step': agent.decision + 1, 'state': env.copy(), 'action': None, 'outcome': None}
            if agent.safe_stops >= agent.S_max: result = 'Stopped -- unresolved'; break
    ver = sum(1 for c in agent.corrections if c['status'] == 'VERIFIED')
    log(f"Result: {result} after {steps} world steps, {agent.safe_stops} safe stops")
    log(f"Mismatches: {agent.mismatches}")
    log(f"Corrections: {len(agent.corrections)} ({ver} verified)")
    log(f"Model evaluations used: {agent.evals}")
    return agent, result, steps


DEMO = ["##########", "#........#", "#........#", "#..A.O..D#", "#........#",
        "#........#", "#...##...#", "#........#", "#........#", "##########"]
DEMO2 = ["##########", "#........#", "#........#", "#..A#O..D#", "#........#",
         "#........#", "#...##...#", "#........#", "#........#", "##########"]

if __name__ == '__main__':
    lines = []
    lg = lines.append
    run_c1(parse(DEMO), "C1 demo 1: correct belief", lg)
    true2 = parse(DEMO2); bel2 = true2.copy(); bel2.walls[3][4] = False
    run_c1(true2, "C1 demo 2: belief wrongly has no wall at (4, 3)", lg, belief=bel2)
    print("\n".join(lines))


class C0:
    """Naive agent (Phase B 11.8): plans, never reads observations after the first, never checks."""
    def __init__(self, first_obs, B=200, base_model=None, belief=None):
        self.B = B; self.belief = (belief or first_obs['state']).copy()
        self.model = Model(base_model); self.plan = []; self.evals = 0; self.searches = 0
        self.dead, self.pd = dead_map(self.belief); self.walls_version = repr(self.belief.walls)
    predict = C1.predict; search = C1.search; path = C1.path; fallback = C1.fallback
    def step(self, obs):
        self.budget = Budget(self.B); self.cache = {}
        if not self.plan:
            self.searches += 1
            kind, plan, nodes = self.search(self.belief, lambda s: s.obj == s.dest,
                                            lambda s: heuristic(s, self.dead, self.pd), 0)
            self.plan = plan if kind == 'found' else (self.fallback(nodes) or [])
            if not self.plan:
                # C0 has no safe stops: it takes the first action its own model says is not Blocked,
                # or North if every action is predicted Blocked.
                for a, _ in DIRS:
                    r = self.predict(self.belief, a)
                    if r is not None and r[1] in ('Moved', 'Pushed'):
                        self.plan = [(a, r[0])]; break
                if not self.plan: self.plan = [('North', self.belief.copy())]
        a, exp = self.plan.pop(0)
        self.belief = exp.copy()      # advances by its own stored prediction; no observation read
        return ('act', a)
