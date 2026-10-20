import random, collections, sys
sys.path.insert(0,'.')
from ref_two import *
from pilot import bfs_len
rows=collections.Counter(); n_solv=n_uns=0; dis=bad=0
for t in range(150):
    random.seed(5000+t)
    walls=[[x in(0,9) or y in(0,9) or random.random()<0.12 for x in range(10)] for y in range(10)]
    free=[(x,y) for y in range(10) for x in range(10) if not walls[y][x]]
    a,o,d=random.sample(free,3); s=W(walls,a,o,d)
    if not valid(s): continue
    L=bfs_len(s); solv = L is not None
    n_solv+=solv; n_uns+=not solv
    for cond in ('C1@200','C1@100','C2','C3','C4'):
        if cond.startswith('C1'):
            ag,r,_=ref_c1.run_c1(s,"t",lambda *_:None,B=int(cond[3:]),T_max=100)
        else:
            sy=TwoSystem(cond,s,lambda *_:None); r=sy.run()
            dis+=len(sy.records)+len(sy.objections)
            bad+=sum(1 for X in (sy.R1,sy.R2) if X.corrections or X.mismatches)
        rows[(cond,'solvable' if solv else 'unsolvable',r)]+=1
print(f"layouts: {n_solv+n_uns} valid; independently solvable (breadth-first search over true states): {n_solv}; unsolvable: {n_uns}")
for cond in ('C1@200','C1@100','C2','C3','C4'):
    print(cond, {k[1]+': '+k[2]:v for k,v in sorted(rows.items()) if k[0]==cond})
print("two-reasoner disagreements/objections:",dis," reasoners with corrections or mismatches:",bad)
