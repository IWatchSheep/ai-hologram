import sys; sys.path.insert(0,'.')
from ref_score import *
L=[(s,make_layout(s)[0]) for s in range(500000,500004)]
runs=[(seed,rs,s) for seed,s in L for rs in (1,2)]
EV=make_events(runs)
def prim(R):
    sc=R['scorer']
    return [(e.ev['id'],e.strict_step,e.ended,e.correcting and e.correcting['status'],e.mismatch_step) for e in sc.ev]+[sc.false_alarms,sc.wrong_accepted,len(sc.claims),sc.justified,R['evals'],R['outcome']]
for i,(ls,rs,s) in enumerate(runs):
    for cond in ('C1','C0'):
        R=score_run(cond,s,EV[i]); R2=score_run(cond,s,EV[i],observe=False)
        sc=R['scorer']
        same = prim(R)==prim(R2)
        print(f"{ls}-{rs} {cond} {R['outcome']} ws{R['world_steps']} ds{R['decisions']} ev{R['evals']} seen{getattr(sc,'n_evals_seen',0)} claims{len(sc.claims)} FA{sc.false_alarms} wrong{sc.wrong_accepted} mism{len(sc.mism)} lab{sc.labels_correct}/{sc.labels_scored} nocause{sc.no_cause} just{sc.justified} obs-independent:{same}")
        for e,(ev,cls,r) in zip(sc.ev,R['records']):
            print(f"   {ev['id']} {ev['type']} act{e.act_step} opp{e.opp_step} rev{int(e.revealing)} mism{e.mismatch_step} strict{e.strict_step} end{e.ended} corr{e.correcting and (e.correcting['id'],e.correcting['status'])}")
        if sc.claims: print("   claims:", [(c['id'],c['kind'],c['el'],c['value'] if c['kind']!='model' else 'std',c['_true'],c['_step'],c['status']) for c in sc.claims])
        if sc.mism: print("   mism:", sc.mism)
