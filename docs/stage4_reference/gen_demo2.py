import sys; sys.path.insert(0,'.')
from ref_stage4 import *
from ref_c1 import parse, DEMO2, run_c1
a=[]; run_c1(parse(DEMO2), "x", a.append, belief=(lambda t: (t.walls[3].__setitem__(4, False), t)[1])(parse(DEMO2).copy()))
b=[]
ev=[{'id':'demo-1','step':1,'type':'F1','target':'R1','common':False,'dir':'blocked->free','cell':(4,3)}]
R = run_with_faults('C1', parse(DEMO2), ev, log=b.append)
print("\n".join(a[2:])); print("----"); print("\n".join(b)); print(R['outcome'], R['world_steps'], R['evals'])
print("identical agent lines:", a[2:-4]==b)
