import sys; sys.path.insert(0,'.')
from ref_two import *
LF=parse(["##########","#........#","#........#","#A...O...#","#........#","#........#","#....D...#","#........#","#........#","##########"])
print("valid",valid(LF))
def run(title, cond, true, r1=None, r2=None, show=40, **kw):
    lines=[]; s=TwoSystem(cond,true,lines.append,r1=r1,r2=r2,**kw); r=s.run(title)
    print("\n".join(lines[:show]) + ("\n..." if len(lines)>show else "")); print("\n".join(lines[-2:]))
    return s
f=make_f3('East','right')
for c in ('C3','C4','C2'):
    run(f"{c}: R1's East push wrongly predicted to move the object south","%s"%c,LF,r1={'base_model':f},show=30)
