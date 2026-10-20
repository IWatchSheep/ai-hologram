import sys, itertools; sys.path.insert(0,'.')
from ref_two import *
D=parse(ref_c1.DEMO)
def restore(pre, model, faults):
    p=pre.copy(); m=model
    for k,c,v in faults:
        if k=='wall': p.walls[c[1]][c[0]]=v
        elif k=='object': p.obj=v
        elif k=='model': m=apply_action
    return p,m
def minimal_sets(pre1,m1,f1,p2,a):
    p1=m1(pre1,a); cells=fp2_cells(pre1,a,p1,p2)
    def removed(S):
        q,m=restore(pre1,m1,S); r=m(q,a); return not differ(r,p2,cells)[0]
    mins=[]
    for k in range(1,len(f1)+1):
        for S in itertools.combinations(f1,k):
            if removed(S) and not any(set(M)<=set(S) for M in mins): mins.append(S)
    return mins
t=D.copy(); t.agent=(4,3); b=t.copy(); b.walls[3][6]=True; b.walls[2][5]=True
faults=[('wall',(6,3),False),('wall',(5,2),False),('model','East',None)]
p2=apply_action(t,'East')
print("case 3 minimal restoration sets:", minimal_sets(b,make_f3('East','left'),faults,p2,'East'))
