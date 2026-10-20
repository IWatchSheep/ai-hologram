import sys, random, itertools; sys.path.insert(0,'.')
from ref_two import *
D=parse(ref_c1.DEMO)   # agent (3,3), object (5,3)
def pred(belief, a, model=apply_action): return model(belief, a)
def classify(pre1, m1, faults1, pre2, m2, faults2, a):
    """Scoring-only: A1 intervention for each fault, and effective use per 12.5.2."""
    p1, p2 = pred(pre1,a,m1), pred(pre2,a,m2)
    cells = fp2_cells(pre1, a, p1, p2); base = differ(p1,p2,cells)
    if not base[0]: return 'no disagreement', {}
    out = {}
    for side,(pre,m,faults,other) in enumerate([(pre1,m1,faults1,p2),(pre2,m2,faults2,p1)]):
        own = p1 if side==0 else p2
        for f in faults:
            q = counterfactual(pre, a, m, f, other)
            eff = (q[1]!=own[1]) or any(contents(q[0],c)!=contents(own[0],c) for c in cells)
            newd = differ(q, other, cells) if side==0 else differ(other, q, cells)
            out[(side,f[0],f[1])] = ('effective' if eff else 'not effective', 'causal' if newd!=base else 'not causal')
    # joint: restore all faults of both
    q1,q2=pre1.copy(),pre2.copy(); M1,M2=m1,m2
    for (pre,fs) in ((q1,faults1),(q2,faults2)):
        for k,c,v in fs:
            if k=='wall': pre.walls[c[1]][c[0]]=v
            if k=='object': pre.obj=v
    if any(f[0]=='model' for f in faults1): M1=apply_action
    if any(f[0]=='model' for f in faults2): M2=apply_action
    j1,j2=pred(q1,a,M1),pred(q2,a,M2)
    out['all restored removes d']= not differ(j1,j2,cells)[0]
    return base, out
# Case 1: single causal fault: R1 believes wall at c1=(4,3)
b=D.copy(); b.walls[3][4]=True
print("1 causal:", classify(b,apply_action,[('wall',(4,3),False)], D.copy(),apply_action,[], 'East'))
# Case 2: overdetermined: R1 wall at c1 wrongly blocked AND object wrongly at c1 with ... use c1 blocked + object believed at (4,3)? object on wall invalid -> use model fault too
b=D.copy(); b.walls[3][4]=True; f3=make_f3('East','left')
print("2 overdetermined (wall c1 + irrelevant model):", classify(b,f3,[('wall',(4,3),False),('model','East',None)], D.copy(),apply_action,[], 'East'))
# Case 3: overdetermination with two sufficient faults: agent (4,3) pushing object (5,3) east; R1 believes wall at c2=(6,3) AND has F3 rotating the push into a believed wall
t=D.copy(); t.agent=(4,3); b=t.copy(); b.walls[3][6]=True; b.walls[2][5]=True
print("3 two sufficient faults:", classify(b,make_f3('East','left'),[('wall',(6,3),False),('wall',(5,2),False),('model','East',None)], t.copy(),apply_action,[], 'East'))
