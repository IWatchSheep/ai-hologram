import sys, collections, time; sys.path.insert(0,'.')
from ref_stage4 import *
t=time.time()
def gen(rng_):
    L=[]; 
    for seed in rng_:
        s,att,l = make_layout(seed); L.append((seed,s,att,l))
    return L
P = gen(range(900000,900040)); C = gen(range(100000,100400))
for name, X in (("pilot",P),("confirm",C)):
    print(name, "max attempts", max(a for _,_,a,_ in X), "shortest range", min(l for *_,l in X), max(l for *_,l in X), "failures", sum(s is None for _,s,_,_ in X))
print("time", round(time.time()-t,1))
runsP = [(seed, rs, s) for seed,s,_,_ in P for rs in (1,2)]
evP = make_events(runsP)
runsC = [(seed, rs, s) for seed,s,_,_ in C for rs in (1,2,3,4,5)]
evC = make_events(runsC)
for name, ev in (("pilot",evP),("confirm",evC)):
    flat=[e for r in ev for e in r]
    c=collections.Counter((e['type'],e['target']) for e in flat)
    print(name, "events", len(flat), dict(sorted(c.items())))
    print(" F1 dirs", collections.Counter(e['dir'] for e in flat if e['type']=='F1'), "F1 no cell", sum(1 for e in flat if e['type']=='F1' and e['cell'] is None), "F4 no cell", sum(1 for e in flat if e['type']=='F4' and e['cell'] is None))
    print(" steps", sorted(collections.Counter(e['step'] for e in flat).items()))
print("pilot run 1 events:"); [print(" ", ev_text(e)) for e in evP[0]]
print("pilot run 2 events:"); [print(" ", ev_text(e)) for e in evP[1]]
print("pilot run 3 events:"); [print(" ", ev_text(e)) for e in evP[2]]
print("confirm first run events:"); [print(" ", ev_text(e)) for e in evC[0]]
print("confirm last run events:"); [print(" ", ev_text(e)) for e in evC[-1]]
import pickle; pickle.dump((P,C), open('layouts.pkl','wb'))
