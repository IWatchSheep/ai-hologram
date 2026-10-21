import sys, collections; sys.path.insert(0,'.')
from ref_score import *
L=[(s,make_layout(s)[0]) for s in range(500000,500010)]
runs=[(seed,rs,s) for seed,s in L for rs in (1,2,3)]
EV=make_events(runs)
c=collections.Counter()
for i,(ls,rs,s) in enumerate(runs):
    for cond in ('C1','C0'):
        R=score_run(cond,s,EV[i]); sc=R['scorer']
        R2=score_run(cond,s,EV[i],observe=False); sc2=R2['scorer']
        p1=[(e.strict_step,e.ended,e.mismatch_step) for e in sc.ev]+[sc.false_alarms,sc.wrong_accepted,R['evals']]
        p2=[(e.strict_step,e.ended,e.mismatch_step) for e in sc2.ev]+[sc2.false_alarms,sc2.wrong_accepted,R2['evals']]
        c['obs-indep ok' if p1==p2 else 'OBS DEPENDENT']+=1
        c['evals seen ok' if getattr(sc,'n_evals_seen',0)==R['evals'] else 'EVALS MISMATCH']+=1
        c['nocause']+=sc.no_cause; c['FA']+=sc.false_alarms; c['wrong']+=sc.wrong_accepted
        for e in sc.ev:
            c[(cond,'end',e.ended and e.ended[0])]+=1
            if e.strict_step: c[(cond,'strict')]+=1
print(dict(c))
