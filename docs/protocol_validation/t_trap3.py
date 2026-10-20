import sys; sys.path.insert(0,'.')
import ref_two
from pilot import make_layout
from ref_two import *
s,_,_=make_layout(900104)
for flag in (False, True):
    ref_two.TRAP_CHECK_ON_OBJECTION=flag
    lines=[]; sy=TwoSystem('C3',s,lines.append,B=200,r1={'not_dead_overrides':[(1,3)]})
    r=sy.run()
    print(f"partner objection triggers the trap check = {flag}: {r}")
    print("\n".join(l for l in lines if 'OBJECTION' in l or 'trap' in l.lower() or 'dead square' in l or 'correction' in l)[:900])
    print(lines[-2])
