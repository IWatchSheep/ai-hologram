import sys; sys.path.insert(0,'.')
import ref_two
from ref_two import *
D=parse(ref_c1.DEMO); D2=parse(ref_c1.DEMO2)
def go(M, enforce, title):
    ref_two.ENFORCE_MMAX=enforce
    lines=[]; s=TwoSystem('C3',D2,lines.append,M_max=M,r1={'belief':D.copy()})
    r=s.run(title)
    print(title, "->", r, "| largest messages in one step:", s.max_step_msgs, "| limit hits:", s.mmax_hits)
    print("   ", [l for l in lines if 'message limit' in l or 'cautious' in l][:3])
    return s
go(16, True,  "M_max=16, enforced")
s=go(3,  True,  "M_max=3, enforced ")
assert s.max_step_msgs <= 3
s=go(3,  False, "M_max=3, enforcement deliberately disabled")
print("deliberate failure check: a step exceeded M_max:", s.max_step_msgs > 3)
ref_two.ENFORCE_MMAX=True
