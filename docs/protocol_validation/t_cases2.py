import sys; sys.path.insert(0,'.')
from ref_two import *
D=parse(ref_c1.DEMO); D2=parse(ref_c1.DEMO2)
def run(title, cond, true, r1=None, r2=None, show=30, **kw):
    lines=[]; s=TwoSystem(cond,true,lines.append,r1=r1,r2=r2,**kw); r=s.run(title)
    print("\n".join(lines[:show]) + ("\n..." if len(lines)>show else "")); print("\n".join(lines[-2:]))
    return s
Dw=D.copy(); Dw.walls[3][6]=True   # R2 wrongly believes wall at (6,3)
run("R C3: reciprocal - R2 wrongly believes wall (6,3)","C3",D,r2={'belief':Dw},show=16)
run("F C3: R1 faulty East-push model (rotate left)","C3",D,r1={'base_model':make_f3('East','left')},show=40)
