import sys; sys.path.insert(0,'.')
from ref_two import *
D=parse(ref_c1.DEMO); D2=parse(ref_c1.DEMO2)
def run(title, cond, true, r1=None, r2=None, show=40, **kw):
    lines=[]; s=TwoSystem(cond,true,lines.append,r1=r1,r2=r2,**kw); r=s.run(title)
    print("\n".join(lines[:show]) + ("\n..." if len(lines)>show else "")); print(lines[-2] if len(lines)>show else "")
    return s
# A: R1 wrong (no wall at (4,3)), R2 right
s=run("A C3: R1 lacks wall (4,3), R2 right","C3",D2,r1={'belief':D.copy()})
# B: R2 wrong, R1 right
s=run("B C3: R2 lacks wall (4,3), R1 right","C3",D2,r2={'belief':D.copy()})
# C: common mode
s=run("C C3: both lack wall (4,3)","C3",D2,r1={'belief':D.copy()},r2={'belief':D.copy()},show=12)
