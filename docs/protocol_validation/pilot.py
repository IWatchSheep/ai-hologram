"""Feasibility pilots (protocol 9.2 only): do runs work? No comparative scores are computed.
Layouts and fault events follow Sections 12.2 and 14 (pilot seed range 900000-900039, 2 seeds each)."""
import sys, random, collections, traceback
sys.path.insert(0, '/home/claude/phaseC')
from ref_two import *
from ref_c1 import C0, run_c1

def bfs_len(s):
    from collections import deque
    seen = {(s.agent, s.obj): 0}; q = deque([s.copy()])
    while q:
        cur = q.popleft(); d = seen[(cur.agent, cur.obj)]
        if cur.obj == cur.dest: return d
        for a, _ in DIRS:
            n, oc = apply_action(cur, a)
            if oc != 'Blocked' and (n.agent, n.obj) not in seen:
                seen[(n.agent, n.obj)] = d + 1; q.append(n)
    return None

def make_layout(seed):
    rng = random.Random(seed)
    for attempt in range(10000):
        walls = [[x in (0, 9) or y in (0, 9) or rng.random() < 0.12 for x in range(10)] for y in range(10)]
        free = [(x, y) for y in range(10) for x in range(10) if not walls[y][x]]
        a, o, d = rng.sample(free, 3); s = W(walls, a, o, d)
        if not valid(s): continue
        L = bfs_len(s)
        if L is None or not (12 <= L <= 40): continue
        dead, _ = dead_map(s)
        if not any(dead[c] for c in free): continue
        return s, attempt + 1, L
    raise RuntimeError("no layout")

def make_events(layouts_seeds, master=4242):
    rng = random.Random(master)
    n = len(layouts_seeds) * 3
    types = (['F1', 'F2', 'F3', 'F4', 'F5'] * (n // 5 + 1))[:n]; rng.shuffle(types)
    per_type = collections.defaultdict(list)
    for i, ty in enumerate(types): per_type[ty].append(i)
    common, target, f1dir = {}, {}, {}
    for ty, idx in per_type.items():
        perm = idx[:]; rng.shuffle(perm)
        k = round(0.2 * len(perm))
        for j, i in enumerate(perm): common[i] = j < k
        single = [i for i in perm if not common[i]]
        for j, i in enumerate(single): target[i] = 'R1' if j % 2 == 0 else 'R2'
        if ty == 'F1':
            perm2 = idx[:]; rng.shuffle(perm2)
            for j, i in enumerate(perm2): f1dir[i] = 'free->blocked' if j % 2 == 0 else 'blocked->free'
    runs = []
    for r, (lay, seed) in enumerate(layouts_seeds):
        s = lay
        rr = random.Random(hash((r, seed, master)) & 0xffffffff)
        steps = sorted(rr.sample(range(1, 11), 3))
        evs = []
        for j in range(3):
            i = 3 * r + j; ty = types[i]
            ev = {'step': steps[j], 'type': ty, 'common': common[i], 'target': target.get(i, 'both')}
            interior = [(x, y) for y in range(1, 9) for x in range(1, 9) if (x, y) != s.dest]
            if ty == 'F1':
                ev['dir'] = f1dir[i]
                cand = [c for c in interior if s.wall(c) == (ev['dir'] == 'blocked->free')]
                ev['cell'] = rr.choice(cand) if cand else None
            elif ty == 'F2':
                order = [d for d, _ in DIRS]; rr.shuffle(order); ev['order'] = order
            elif ty == 'F3':
                ev['action'] = rr.choice([d for d, _ in DIRS]); ev['rot'] = rr.choice(['left', 'right'])
            elif ty == 'F4':
                dead, _ = dead_map(s)
                cand = [c for c in interior if dead[c]]
                ev['cell'] = rr.choice(cand) if cand else None
            evs.append(ev)
        runs.append(evs)
    return runs

WAIT_F5 = '--wait' in sys.argv
import ref_two
ref_two.TRAP_CHECK_ON_OBJECTION = '--trapcheck' in sys.argv

class Counters:
    def __init__(self): self.c = collections.Counter()

def apply_event(X, ev, true_hist, counters):
    """12.2 effect on one reasoner X. Returns True if applied."""
    ty = ev['type']; b = X.belief
    if ty == 'F1':
        c = ev['cell']
        if c is None: return False
        if ev['dir'] == 'free->blocked' and (b.agent == c or b.obj == c): return False
        b.walls[c[1]][c[0]] = not b.walls[c[1]][c[0]]; X.recompute_dead(); X.cache = {}
        return True
    if ty == 'F2':
        for dname in ev['order']:
            n = add(b.obj, DIRV[dname])
            if 0 <= n[0] < 10 and 0 <= n[1] < 10 and not b.wall(n) and n != b.agent:
                b.obj = n; X.cache = {}; return True
        return False
    if ty == 'F3':
        X.model.base = make_f3(ev['action'], ev['rot']); X.model.restored.discard(ev['action']); X.cache = {}
        return True
    if ty == 'F4':
        if ev['cell'] is None: return False
        X.not_dead.append(ev['cell']); X.recompute_dead(); X.cache = {}
        return True
    if ty == 'F5':
        objs = [h['state'].obj for h in true_hist]
        prev = None
        for i in range(len(objs) - 1, 0, -1):
            if objs[i] != objs[i - 1]: prev = objs[i - 1]; break
        if prev is None: counters.c[('F5why','object not yet moved')] += 1; return False
        if prev == b.agent: counters.c[('F5why','stale cell = believed agent cell (invalid belief)')] += 1; return False
        if b.wall(prev): counters.c[('F5why','stale cell believed blocked')] += 1; return False
        b.obj = prev; X.cache = {}; return True
    return False

def make_hook(evs, cond, counters, true_hist_getter):
    done = set()
    def hook(X, step):
        for k, ev in enumerate(evs):
            wait = WAIT_F5 and ev['type'] == 'F5'
            if (ev['step'] != step) and not (wait and step > ev['step']): continue
            if cond in ('C2', 'C3', 'C4') and not ev['common'] and ev['target'] != X.name: continue
            if (k, getattr(X, 'name', 'R')) in done: continue
            done.add((k, getattr(X, 'name', 'R')))
            ok = apply_event(X, ev, true_hist_getter(), counters)
            if wait and not ok:
                done.discard((k, getattr(X, 'name', 'R'))); continue      # keep waiting
            counters.c[(cond, ev['type'], 'applied' if ok else 'not applicable')] += 1
            if ok and ev['type'] == 'F5':
                counters.c[('F5delay', step - ev['step'])] += 1
                counters.c[('F5late', 'applied after step 10' if step > 10 else 'applied by step 10')] += 1
    return hook

class C1F(C1):
    def step(self, obs):
        self.decision += 1
        self.budget = Budget(self.B); self.cache = {}
        if self.decision > 1: self.history.append(obs)
        self.check(obs)
        if self.pending_inv:
            args = self.pending_inv; self.pending_inv = None; self.investigate(*args)
            if self.pending_inv: return self.safe_stop('budget too small for verification')
        if self.pending_trap: self.trap_v2(self.pending_trap)
        if self.hook: self.hook(self, self.decision)
        if self.pending_v1: self.continue_v1(obs)
        return self.choose()

def run_single(cond, s, evs, counters, B=200, S_max=5, T_max=100):
    o = {'step': 1, 'state': s.copy(), 'action': None, 'outcome': None}
    hist = [o]
    if cond == 'C1':
        ag = C1F(o, B=B, S_max=S_max, T_max=T_max, log=lambda *_: None)
    else:
        ag = C0(o, B=B)
        ag.not_dead = []; ag.forced_dead = []
        def rd(ag=ag):
            ag.dead, ag.pd = dead_map(ag.belief, ag.not_dead, ag.forced_dead); ag.walls_version = repr(ag.belief.walls)
        ag.recompute_dead = rd
    ag.name = 'R'
    ag.hook = make_hook(evs, cond, counters, lambda: hist)
    env = s.copy(); steps = 0; dec = 0; stops = 0
    while True:
        dec += 1
        if cond == 'C0':
            ag.budget = Budget(B); ag.cache = {}
            ag.hook(ag, dec)
            if ag.plan == [] and False: pass
            d = ag.step(o)
        else:
            d = ag.step(o)
        if d[0] == 'act':
            env, out = apply_action(env, d[1]); steps += 1
            o = {'step': dec + 1, 'state': env.copy(), 'action': d[1], 'outcome': out}; hist.append(o)
            if env.obj == env.dest or steps >= T_max:
                if cond == 'C1': ag.final_check(o)
                return ('Success' if env.obj == env.dest else 'Step limit'), dec
        else:
            stops += 1
            o = {'step': dec + 1, 'state': env.copy(), 'action': None, 'outcome': None}; hist.append(o)
            if stops >= S_max: return 'Stopped -- unresolved', dec

def run_two(cond, s, evs, counters):
    sysm = TwoSystem(cond, s, lambda *_: None)
    hist = [sysm.obs]
    sysm.fault_hook = make_hook(evs, cond, counters, lambda: hist)
    env = s.copy(); res = None
    while res is None:
        a = sysm.decide()
        if a is None:
            sysm.obs = {'step': sysm.decision + 1, 'state': env.copy(), 'action': None, 'outcome': None}; hist.append(sysm.obs)
            if sysm.safe_stops >= sysm.S_max: res = 'Stopped -- unresolved'
            continue
        env, out = apply_action(env, a); sysm.steps += 1
        sysm.obs = {'step': sysm.decision + 1, 'state': env.copy(), 'action': a, 'outcome': out}; hist.append(sysm.obs)
        if env.obj == env.dest or sysm.steps >= sysm.T_max:
            for X in (sysm.R1, sysm.R2): X.final_check(sysm.obs)
            res = 'Success' if env.obj == env.dest else 'Step limit'
    return res, sysm.decision, sysm

if __name__ == '__main__':
    layouts = []
    for seed in range(900000, 900040):
        s, att, L = make_layout(seed); layouts.append((seed, s, L))
    ls = [(s, rs) for (_, s, _) in layouts for rs in (1, 2)]
    runs = make_events(ls)
    counters = Counters(); crashes = collections.Counter(); maxdec = collections.Counter()
    ends = collections.Counter(); msg_over = collections.Counter(); acted = collections.Counter(); loops = collections.Counter(); limit_hits = collections.Counter()
    for i, ((s, rs), evs) in enumerate(zip(ls, runs)):
        for cond in ('C0', 'C1', 'C2', 'C3', 'C4'):
            try:
                if cond in ('C0', 'C1'):
                    r, dec = run_single(cond, s, evs, counters)
                else:
                    r, dec, sysm = run_two(cond, s, evs, counters)
                    msg_over[cond] = max(msg_over[cond], sysm.max_step_msgs); limit_hits[cond] += sysm.mmax_hits
                    if sum(1 for r in sysm.records if r['kind'] == 'immediate-consequence') >= 5: loops[cond] += 1
                    if sysm.steps > 0: acted[cond] += 1
                maxdec[cond] = max(maxdec[cond], dec)
                ends[r] += 1
            except Exception as e:
                crashes[cond] += 1
                if crashes[cond] <= 2: traceback.print_exc()
    print("layouts:", len(layouts), " shortest-solution range:", min(l for *_, l in layouts), "-", max(l for *_, l in layouts))
    print("runs per condition:", len(ls), " crashes:", dict(crashes) or 0)
    print("max decision steps per condition:", dict(maxdec), "(bound: T_max + S_max = 105)")
    print("run endings, pooled over all conditions (no per-condition scores, 9.3):", dict(ends))
    print("two-reasoner runs that took at least one world action:", dict(acted), "of", len(ls))
    print("largest number of messages in any single decision step (M_max = 16, enforced per step):", dict(msg_over))
    print("decision steps that reached M_max:", dict(limit_hits) or 0)
    print("runs with 5 or more repeated consequence objections (livelock indicator):", dict(loops) or 0)
    tot = collections.Counter()
    for key, n in counters.c.items():
        if key[0] == 'F5why': print('F5 not applicable because:', key[1], n); continue
        if key[0] in ('F5delay', 'F5late'): continue
        cond, ty, st = key; tot[(ty, st)] += n
    print("fault applications, pooled (per reasoner application):", dict(sorted(tot.items())))
    dl = sorted((k[1], n) for k, n in counters.c.items() if k[0] == 'F5delay')
    print("F5 delay (applied step - scheduled step), per reasoner application:", dl)
    print("F5 timing:", {k[1]: n for k, n in counters.c.items() if k[0] == 'F5late'})
    expected = 0
    for evs in runs:
        for ev in evs:
            if ev['type'] == 'F5': expected += 1 * 2 + (2 if ev['common'] else 1) * 3   # C0, C1 + C2-C4 targets
    applied = sum(n for k, n in counters.c.items() if len(k) == 3 and k[1] == 'F5' and k[2] == 'applied')
    print("F5 reasoner applications expected:", expected, " applied:", applied, " never activated:", expected - applied)
    sched = collections.Counter(ev['type'] for evs in runs for ev in evs)
    print("scheduled events per type:", dict(sorted(sched.items())), " common-mode:", sum(ev['common'] for evs in runs for ev in evs))
