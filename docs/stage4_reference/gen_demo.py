import sys, collections; sys.path.insert(0,'.')
from ref_stage4 import *
L=[]
for seed in range(500000,500004):
    s,att,l = make_layout(seed); L.append((seed,s,att,l)); print(seed, "attempts", att, "shortest", l, "dead", len(true_dead_cells(s)))
runs=[(seed,rs,s) for seed,s,_,_ in L for rs in (1,2)]
ev=make_events(runs)
for i,(ls,rs,s) in enumerate(runs):
    print(f"run {ls}-{rs}")
    for e in ev[i]: print("   ", ev_text(e))
    for cond in ('C1','C0'):
        R=run_with_faults(cond,s,ev[i])
        print(f"  {cond}: {R['outcome']} ws {R['world_steps']} ds {R['decisions']} stops {R['safe_stops']} evals {R['evals']}", "corr", len(getattr(R['agent'],'corrections',[])), "mism", getattr(R['agent'],'mismatches',0))
        for e,cls,r in R['records']: print(f"      {e['id']}: {cls}" + (f" at step {r['step']}" if r['status']=='applied' else '') + (f" ({r['reason']})" if r['reason'] else ''))
