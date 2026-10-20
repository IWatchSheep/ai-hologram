import sys, random, collections; sys.path.insert(0,'.')
from ref_two import *
from pilot import make_layout
def a2_resolve(belief_wrong, true, p, radius):
    """A2 investigation by the receiver: check present walls within `radius` of p against the current
    (true) observation, recompute, then the local check. Returns how p ends up classified."""
    b=belief_wrong.copy()
    cells=[(x,y) for y in range(10) for x in range(10) if abs(x-p[0])+abs(y-p[1])<=radius]
    fixed_walls=[c for c in cells if b.wall(c)!=true.wall(c)]
    for c in fixed_walls: b.walls[c[1]][c[0]]=true.wall(c)
    dead,_=dead_map(b)
    if dead[p]: return 'resolved by wall correction'
    if not ref_c1.local_check_not_dead(b,dead,p): return 'resolved by trap correction (wrong element)'
    return 'NOT resolved (loop risk)'
res={r:collections.Counter() for r in (1,2,3,10)}
cases=0
for seed in range(901000,901120):
    try: s,_,_=make_layout(seed)
    except Exception: continue
    dead,_=dead_map(s)
    for p in [c for c in dead if dead[c]]:
        for dist in (1,2,3):
            for c in [(x,y) for y in range(1,9) for x in range(1,9) if abs(x-p[0])+abs(y-p[1])==dist and s.wall((x,y)) and (x,y)!=s.dest]:
                b=s.copy(); b.walls[c[1]][c[0]]=False          # F1 blocked->free near p
                if not valid(b): continue
                d2,_=dead_map(b)
                if d2[p]: continue                              # p still dead: no disagreement
                cases+=1
                for r in res: res[r][(dist, a2_resolve(b,s,p,r))]+=1
print("disagreements about p caused by one wrong wall near p:",cases)
for r in res:
    print(f"wall check radius {r}:",dict(sorted(res[r].items())))
