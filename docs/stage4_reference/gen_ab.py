import sys; sys.path.insert(0,'.')
from ref_stage4 import *
r = SeededRandom(0); print("seed0 first 3:", [r.next() for _ in range(3)])
r = SeededRandom(1234567); print("seed1234567 first 3:", [r.next() for _ in range(3)])
r = SeededRandom(42); print("below(10) x10 seed42:", [r.below(10) for _ in range(10)])
r = SeededRandom(42); xs=list(range(1,11)); r.shuffle(xs); print("shuffle 1..10 seed42:", xs)
r = SeededRandom(7); print("pick_distinct 1..10 k3 seed7:", r.pick_distinct(list(range(1,11)),3))
r = SeededRandom(5); print("below(1) consumes one draw:", r.below(1), r.state)
import statistics
for seed in [900000, 900001, 100000]:
    s, att, L = make_layout(seed)
    print(seed, "attempts", att, "shortest", L, "agent", s.agent, "object", s.obj, "dest", s.dest, "dead cells", len(true_dead_cells(s)), "walls interior", sum(s.walls[y][x] for y in range(1,9) for x in range(1,9)))
    print("\n".join(layout_text(s)))
