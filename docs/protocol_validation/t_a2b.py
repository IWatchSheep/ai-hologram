import sys, collections; sys.path.insert(0,'.')
from ref_two import *
from pilot import make_layout
from t_a2 import a2_resolve
dist_hist=collections.Counter(); res={r:collections.Counter() for r in (2,3,4)}
for seed in range(901000,901120):
    try: s,_,_=make_layout(seed)
    except Exception: continue
    dead,_=dead_map(s)
    for p in [c for c in dead if dead[c]]:
        for c in [(x,y) for y in range(1,9) for x in range(1,9) if (x,y)!=s.dest and (x,y)!=p]:
            b=s.copy(); b.walls[c[1]][c[0]]=not s.wall(c)
            if not valid(b): continue
            d2,_=dead_map(b)
            if d2[p]: continue
            dist=abs(c[0]-p[0])+abs(c[1]-p[1]); dist_hist[dist]+=1
            for r in res: res[r][a2_resolve(b,s,p,r)]+=1
print("distance from p of the single wrong wall that makes p look live:",dict(sorted(dist_hist.items())))
for r in res: print(f"radius {r}:",dict(res[r]))
