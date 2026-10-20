import random, collections, sys
sys.path.insert(0,'.')
from ref_two import *
res=collections.Counter(); bad=0; dis=0
for t in range(150):
    random.seed(5000+t)
    walls=[[x in(0,9) or y in(0,9) or random.random()<0.12 for x in range(10)] for y in range(10)]
    free=[(x,y) for y in range(10) for x in range(10) if not walls[y][x]]
    a,o,d=random.sample(free,3); s=W(walls,a,o,d)
    if not valid(s): continue
    for cond in ('C2','C3','C4'):
        lines=[]
        sysm=TwoSystem(cond,s,lines.append)
        r=sysm.run()
        res[(cond,r)]+=1
        dis+=len(sysm.records)+len(sysm.objections)
        for X in (sysm.R1,sysm.R2):
            if X.corrections or X.mismatches: bad+=1
print(sorted(res.items())); print("disagreements/objections:",dis,"reasoners with corrections/mismatches:",bad)
