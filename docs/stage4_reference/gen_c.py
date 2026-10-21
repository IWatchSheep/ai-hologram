import sys, pickle, collections, traceback; sys.path.insert(0,'.')
from ref_stage4 import *
P, C = pickle.load(open('layouts.pkl','rb'))
runsP = [(seed, rs, s) for seed,s,_,_ in P for rs in (1,2)]
evP = make_events(runsP)
for i in range(3):
    ls, rs, s = runsP[i]
    for cond in ('C1','C0'):
        R = run_with_faults(cond, s, evP[i])
        print(f"run {ls}-{rs} {cond}: {R['outcome']} world steps {R['world_steps']} decision steps {R['decisions']} safe stops {R['safe_stops']} evals {R['evals']}")
        for ev, cls, r in R['records']:
            print(f"   {ev['id']} {ev['type']} sched {ev['step']}: {cls}" + (f" at step {r['step']}" if r['status']=='applied' else '') + (f" ({r['reason']})" if r['reason'] else ''))
crash=collections.Counter(); cls_c=collections.Counter(); outc=collections.Counter()
for i,(ls,rs,s) in enumerate(runsP):
    for cond in ('C0','C1'):
        try:
            R = run_with_faults(cond, s, evP[i]); outc[R['outcome']]+=1
            for ev, cl, r in R['records']: cls_c[(ev['type'], cl)]+=1
        except Exception:
            crash[cond]+=1; traceback.print_exc() if crash[cond]<2 else None
print("crashes", dict(crash)); print("pooled outcomes", dict(outc))
for k,v in sorted(cls_c.items()): print(" ", k, v)
