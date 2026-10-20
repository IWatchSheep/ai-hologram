import sys; sys.path.insert(0,'.')
from ref_two import *
D=parse(ref_c1.DEMO); D2=parse(ref_c1.DEMO2)
LF=parse(["##########","#........#","#........#","#A...O...#","#........#","#........#","#....D...#","#........#","#........#","##########"])
def run(title, cond, true, r1=None, r2=None, show=12, **kw):
    lines=[]; s=TwoSystem(cond,true,lines.append,r1=r1,r2=r2,**kw); r=s.run(title)
    print("\n".join(lines[:show])); print("\n".join(lines[-2:])); return s,lines
s,l=run("16/18 C3 again (warning retained, not repeated)","C3",LF,r1={'base_model':make_f3('East','right')},show=14)
s,l=run("17 C4: R1 lacks wall (4,3): no immediate comparison","C4",D2,r1={'belief':D.copy()},show=8)
print("immediate comparisons recorded in C4:", sum(1 for r in s.records if r['kind']=='immediate'))
# 13: after R1's correction, stored later states are not announced
s,l=run("13 announce after correction (C3)","C3",D2,r1={'belief':D.copy()},show=0)
