import sys, random; sys.path.insert(0,'.')
import ref_two
from ref_two import *
from pilot import make_layout
ref_two.TRAP_CHECK_ON_OBJECTION=True
# search a case where R1's wrong wall is 3+ cells from a dead cell its plan pushes onto
shown=0; loops=0; total=0
for seed in range(901000,901200):
    try: s,_,_=make_layout(seed)
    except Exception: continue
    dead,_=dead_map(s)
    for p in [c for c in dead if dead[c] and abs(c[0]-s.obj[0])+abs(c[1]-s.obj[1])<=3]:
        for c in [(x,y) for y in range(1,9) for x in range(1,9) if s.wall((x,y)) and (x,y)!=s.dest and abs(x-p[0])+abs(y-p[1])>=3]:
            b=s.copy(); b.walls[c[1]][c[0]]=False
            if not valid(b) or dead_map(b)[0][p]: continue
            lines=[]; sy=TwoSystem('C3',s,lines.append,r1={'belief':b})
            r=sy.run()
            n=sum(1 for x in sy.records if x['kind']=='immediate-consequence')
            if n: total+=1
            if n>=5: loops+=1
            if any('not safe for both' in l for l in lines) and shown<1:
                shown+=1; print("seed",seed,"p",p,"wrong wall",c,"->",r)
                print("\n".join(l for l in lines if 'consequence' in l or 'not safe' in l or 'correction' in l)[:800])
print("cases with a consequence dispute:",total," of which looping (>=5 objections):",loops)
