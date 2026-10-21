"""Stage 4 reference (Python; not project code). Produces the exact expected values in STAGE_04.md.
Part 1: seeded random numbers, true-state search, layout generator, fault-event lists.
Part 2: fault application at the activation point (C1 and C0), activation records."""
import sys
sys.path.insert(0, '.')
from ref_c1 import (C1, C0, W, valid, apply_action, Budget, dead_map, DIRS, DIRV, add, heuristic)

MASK = (1 << 64) - 1
DNAMES = [d for d, _ in DIRS]          # North, East, South, West
ROT = {'left': {'North': 'West', 'West': 'South', 'South': 'East', 'East': 'North'},
       'right': {'North': 'East', 'East': 'South', 'South': 'West', 'West': 'North'}}


class SeededRandom:
    """SplitMix64 (exactly as in plan 3.1)."""
    def __init__(self, seed): self.state = seed & MASK

    def next(self):
        self.state = (self.state + 0x9E3779B97F4A7C15) & MASK
        z = self.state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK
        return z ^ (z >> 31)

    def below(self, n):
        threshold = ((1 << 64) - n) % n
        while True:
            r = self.next()
            if r >= threshold: return r % n

    def shuffle(self, xs):
        for i in range(len(xs) - 1, 0, -1):
            j = self.below(i + 1); xs[i], xs[j] = xs[j], xs[i]

    def pick_distinct(self, xs, k):
        xs = list(xs)
        for i in range(k):
            j = i + self.below(len(xs) - i); xs[i], xs[j] = xs[j], xs[i]
        return xs[:k]


def shortest_solution(s):
    """Experimenter's breadth-first search over true states (agent, object). None if unsolvable."""
    from collections import deque
    if s.obj == s.dest: return 0
    seen = {(s.agent, s.obj): 0}; q = deque([s.copy()])
    while q:
        cur = q.popleft(); d = seen[(cur.agent, cur.obj)]
        for a in DNAMES:
            n, oc = apply_action(cur, a)
            if oc in ('Blocked', 'InvalidState'): continue
            k = (n.agent, n.obj)
            if k in seen: continue
            seen[k] = d + 1
            if n.obj == n.dest: return d + 1
            q.append(n)
    return None


def interior_cells(s):
    return [(x, y) for y in range(1, 9) for x in range(1, 9)]


def true_dead_cells(s):
    dead, _ = dead_map(s)
    return [c for c in interior_cells(s) if dead[c] and c != s.dest]


def make_layout(seed):
    rng = SeededRandom(seed)
    for attempt in range(1, 10001):
        walls = [[False] * 10 for _ in range(10)]
        for y in range(10):
            for x in range(10):
                if x in (0, 9) or y in (0, 9): walls[y][x] = True
                else: walls[y][x] = rng.below(100) < 12
        free = [(x, y) for y in range(10) for x in range(10) if not walls[y][x]]
        if len(free) < 3: continue
        a, o, d = rng.pick_distinct(free, 3)
        s = W(walls, a, o, d)
        if not valid(s): continue
        L = shortest_solution(s)
        if L is None or not (12 <= L <= 40): continue
        if not true_dead_cells(s): continue
        return s, attempt, L
    return None, 10000, None


def round_fifth(count):            # round-half-up of count / 5
    return (2 * count + 5) // 10


def run_stream_seed(master, layout_seed, run_seed):
    return ((master << 40) + (layout_seed << 8) + run_seed) & MASK


def make_events(runs, master=4242):
    """runs: list of (layout_seed, run_seed, true_state). Returns list (per run) of event dicts."""
    rng = SeededRandom(master)
    n = 3 * len(runs)
    types = ['F%d' % (i % 5 + 1) for i in range(n)]
    rng.shuffle(types)
    common, target, f1dir = {}, {}, {}
    for ty in ['F1', 'F2', 'F3', 'F4', 'F5']:
        idx = [i for i in range(n) if types[i] == ty]
        perm = idx[:]; rng.shuffle(perm)
        k = round_fifth(len(perm))
        for j, i in enumerate(perm): common[i] = j < k
        single = [i for i in perm if not common[i]]
        for j, i in enumerate(single): target[i] = 'R1' if j % 2 == 0 else 'R2'
        if ty == 'F1':
            perm2 = idx[:]; rng.shuffle(perm2)
            for j, i in enumerate(perm2): f1dir[i] = 'free->blocked' if j % 2 == 0 else 'blocked->free'
    out = []
    for r, (ls, rs, s) in enumerate(runs):
        rr = SeededRandom(run_stream_seed(master, ls, rs))
        steps = sorted(rr.pick_distinct(list(range(1, 11)), 3))
        evs = []
        for j in range(3):
            i = 3 * r + j; ty = types[i]
            ev = {'id': '%d-%d-%d' % (ls, rs, j + 1), 'step': steps[j], 'type': ty,
                  'target': 'both' if common[i] else target[i], 'common': common[i]}
            if ty == 'F1':
                ev['dir'] = f1dir[i]
                want_wall = ev['dir'] == 'blocked->free'
                cand = [c for c in interior_cells(s) if c != s.dest and s.wall(c) == want_wall]
                ev['cell'] = cand[rr.below(len(cand))] if cand else None
            elif ty == 'F2':
                order = DNAMES[:]; rr.shuffle(order); ev['order'] = order
            elif ty == 'F3':
                ev['action'] = DNAMES[rr.below(4)]; ev['rot'] = ['left', 'right'][rr.below(2)]
            elif ty == 'F4':
                cand = true_dead_cells(s)
                ev['cell'] = cand[rr.below(len(cand))] if cand else None
            evs.append(ev)
        out.append(evs)
    return out


def fmt(c): return '(%d, %d)' % c if c is not None else 'none'


def ev_text(ev):
    t = ev['type']
    if t == 'F1': d = '%s %s' % (fmt(ev['cell']), ev['dir'])
    elif t == 'F2': d = ' '.join(ev['order'])
    elif t == 'F3': d = '%s %s' % (ev['action'], ev['rot'])
    elif t == 'F4': d = fmt(ev['cell'])
    else: d = '-'
    return '%s step %d %s target %s | %s' % (ev['id'], ev['step'], t, ev['target'], d)


def layout_text(s):
    rows = []
    for y in range(10):
        row = ''
        for x in range(10):
            c = (x, y)
            if s.wall(c): ch = '#'
            elif c == s.agent: ch = '+' if c == s.dest else 'A'
            elif c == s.obj: ch = '*' if c == s.dest else 'O'
            elif c == s.dest: ch = 'D'
            else: ch = '.'
            row += ch
        rows.append(row)
    return rows


# ======================= Part 2: fault application (plan 3.4) =======================
def make_faulty_model(rotations):
    """rotations: {action: 'left'|'right'}. 12.2 F3 for every listed action; standard rule otherwise."""
    def f(s, a):
        n, oc = apply_action(s, a)
        if a in rotations and oc == 'Pushed':
            m = s.copy(); m.agent = add(s.agent, DIRV[a])
            tgt = add(m.agent, DIRV[ROT[rotations[a]][a]])
            if s.wall(tgt) or tgt == m.agent: return s.copy(), 'Blocked'
            m.obj = tgt
            return m, 'Pushed'
        return n, oc
    return f


def object_previous(true_hist):
    objs = [o['state'].obj for o in true_hist]
    for i in range(len(objs) - 1, 0, -1):
        if objs[i] != objs[i - 1]: return objs[i - 1]
    return None


def try_apply(X, ev, true_hist):
    """Applies event ev to reasoner X (C1 or C0 reference object). Returns (applied, reason)."""
    ty = ev['type']; b = X.belief
    if ty == 'F1':
        c = ev['cell']
        if c is None: return False, 'no cell of the required kind'
        if ev['dir'] == 'free->blocked' and b.agent == c: return False, 'the wall would be under the believed agent'
        if ev['dir'] == 'free->blocked' and b.obj == c: return False, 'the wall would be under the believed object'
        b.walls[c[1]][c[0]] = not b.walls[c[1]][c[0]]
    elif ty == 'F2':
        for dname in ev['order']:
            n = add(b.obj, DIRV[dname])
            if 0 <= n[0] < 10 and 0 <= n[1] < 10 and not b.wall(n) and n != b.agent:
                b.obj = n; break
        else:
            return False, 'no direction passes the test'
    elif ty == 'F3':
        X.rotations[ev['action']] = ev['rot']
        X.model.base = make_faulty_model(dict(X.rotations))
        X.model.restored.discard(ev['action'])
    elif ty == 'F4':
        if ev['cell'] is None: return False, 'the layout has no true dead square'
        X.not_dead.append(ev['cell'])
    elif ty == 'F5':
        prev = object_previous(true_hist)
        if prev is None: return False, 'the object has not moved yet'
        if prev == b.agent: return False, 'the previous square is the believed agent square'
        if b.wall(prev): return False, 'the previous square is believed to be a wall'
        b.obj = prev
    X.recompute_dead(); X.cache = {}
    return True, ''


class Injector:
    """One per (run, reasoner). Holds the events and the activation records (plan 3.5)."""
    def __init__(self, events, true_hist):
        self.events = events; self.hist = true_hist
        self.rec = {ev['id']: {'status': 'scheduled', 'step': None, 'reason': ''} for ev in events}

    def at_activation_point(self, X, step):
        for ev in self.events:
            r = self.rec[ev['id']]
            if r['status'] == 'scheduled' and ev['step'] == step:
                ok, why = try_apply(X, ev, self.hist)
                if ok: r.update(status='applied', step=step, reason='')
                elif ev['type'] == 'F5': r.update(status='pending', reason=why)
                else: r.update(status='not applicable', step=step, reason=why)
            elif r['status'] == 'pending' and step > ev['step']:
                ok, why = try_apply(X, ev, self.hist)
                if ok: r.update(status='applied', step=step, reason='')
                else: r['reason'] = why

    def finish(self):
        out = []
        for ev in self.events:
            r = self.rec[ev['id']]
            if r['status'] == 'applied':
                cls = 'Activated at its scheduled step' if r['step'] == ev['step'] else 'Activated after deferral'
            elif r['status'] == 'scheduled': cls = 'Not activated: run ended before the scheduled step'
            elif r['status'] == 'pending': cls = 'Not activated: deferred, never applicable'
            else: cls = 'Not activated: not applicable at the scheduled step'
            out.append((ev, cls, r))
        return out



class LiveDeadMap:
    """Matches the C++ DeadSquareCache and StepPredictor cache context: the dead-square map and the
    cache key always follow the CURRENT believed walls and overrides (the Stage 3 reference
    recomputed them only at corrections, which can go stale after an unexplained footprint update)."""
    def _key(self):
        return (repr(self.belief.walls), tuple(self.not_dead), tuple(getattr(self, 'forced_dead', ())), self.belief.dest)

    def _fresh(self):
        k = self._key()
        if getattr(self, '_live_key', None) != k:
            self._live = dead_map(self.belief, self.not_dead, getattr(self, 'forced_dead', ()))
            self._live_key = k
        return self._live

    dead = property(lambda self: self._fresh()[0], lambda self, v: None)
    pd = property(lambda self: self._fresh()[1], lambda self, v: None)
    walls_version = property(lambda self: repr(self.belief.walls) + repr(getattr(self, 'forced_dead', [])), lambda self, v: None)


class C1F(LiveDeadMap, C1):
    """C1 with the activation point after step 3 is complete (protocol 11.3; C++ order)."""
    def step(self, obs):
        self.decision += 1
        self.budget = Budget(self.B); self.cache = {}
        if self.decision > 1: self.history.append(obs)
        self.check(obs)
        if self.pending_inv:
            args = self.pending_inv; self.pending_inv = None; self.investigate(*args)
            if self.pending_inv:
                # the step ends in a safe stop during step 3; the activation point still runs once
                if self.injector: self.injector.at_activation_point(self, self.decision)
                return self.safe_stop('budget too small for verification')
        if self.pending_trap: self.trap_v2(self.pending_trap)
        if self.pending_v1: self.continue_v1(obs)
        if self.injector: self.injector.at_activation_point(self, self.decision)
        return self.choose()


class C0F(LiveDeadMap, C0):
    def __init__(self, first_obs, B=200):
        self.not_dead = []; self.decision = 0
        self.belief = first_obs['state'].copy()
        super().__init__(first_obs, B=B)

    def recompute_dead(self):
        pass

    def step(self, obs):
        self.decision += 1
        if self.injector: self.injector.at_activation_point(self, self.decision)
        return C0.step(self, obs)


def run_with_faults(cond, s, events, B=200, S_max=5, T_max=100, log=lambda *_: None):
    o = {'step': 1, 'state': s.copy(), 'action': None, 'outcome': None}
    hist = [o]
    ag = C1F(o, B=B, S_max=S_max, T_max=T_max, log=log) if cond == 'C1' else C0F(o, B=B)
    ag.rotations = {}
    ag.injector = Injector(events, hist)
    env = s.copy(); steps = 0; stops = 0
    while True:
        d = ag.step(o)
        if d[0] == 'act':
            env, out = apply_action(env, d[1]); steps += 1
            o = {'step': ag.decision + 1, 'state': env.copy(), 'action': d[1], 'outcome': out}; hist.append(o)
            if env.obj == env.dest or steps >= T_max:
                if cond == 'C1': ag.final_check(o)
                res = 'Success' if env.obj == env.dest else 'Step limit'; break
        else:
            stops += 1
            o = {'step': ag.decision + 1, 'state': env.copy(), 'action': None, 'outcome': None}; hist.append(o)
            if stops >= S_max: res = 'Stopped -- unresolved'; break
    return {'agent': ag, 'outcome': res, 'world_steps': steps, 'decisions': ag.decision,
            'safe_stops': stops, 'records': ag.injector.finish(), 'evals': ag.evals}
