import sys; sys.argv += ['--wait','--trapcheck']
sys.path.insert(0,'.')
import pilot, ref_two
from pilot import *
layouts=[make_layout(sd)[0] for sd in range(900000,900040)]
ls=[(s,rs) for s in layouts for rs in (1,2)]; runs=make_events(ls)
for i,((s,rs),evs) in enumerate(zip(ls,runs)):
    c=Counters(); res,dec,sysm=run_two('C3',s,evs,c)
    if sum(1 for r in sysm.records if r['kind']=='immediate-consequence')>=5:
        print("run",i,"events:",[(e['step'],e['type'],e['target'],e.get('cell'),e.get('dir')) for e in evs])
        print("R1 dead==R2 dead?", sysm.R1.dead==sysm.R2.dead)
        diff=[c for c in sysm.R1.dead if sysm.R1.dead[c]!=sysm.R2.dead[c]]
        print("cells where R1/R2 dead status differ:",diff[:10])
        print("R1 not_dead:",sysm.R1.not_dead,"R2 not_dead:",sysm.R2.not_dead)
        tw=[(x,y) for y in range(10) for x in range(10) if sysm.R2.belief.walls[y][x]!=s.walls[y][x]]
        tw1=[(x,y) for y in range(10) for x in range(10) if sysm.R1.belief.walls[y][x]!=s.walls[y][x]]
        print("R1 wrong walls:",tw1," R2 wrong walls:",tw)
