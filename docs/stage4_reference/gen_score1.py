import sys; sys.path.insert(0,'.')
from ref_score import *
from ref_c1 import parse, DEMO2
ev=[{'id':'demo-1','step':1,'type':'F1','target':'R1','common':False,'dir':'blocked->free','cell':(4,3)}]
R=score_run('C1', parse(DEMO2), ev)
sc=R['scorer']; e=sc.ev[0]
print(R['outcome'], R['evals'], len(R['lines']))
print('act',e.act_step,'opp',e.opp_step,'rev',e.revealing,'mism',e.mismatch_step,'strict',e.strict_step,'ended',e.ended, 'corr', e.correcting and (e.correcting['id'], e.correcting['status']))
print('claims', [(c['id'],c['kind'],c['el'],c['value'],c['_true'],c['_step'],c['status']) for c in sc.claims])
print('FA',sc.false_alarms,'wrong',sc.wrong_accepted,'mism',sc.mism,'labels',sc.labels_correct,sc.labels_scored,'nocause',sc.no_cause,'just',sc.justified, 'evals seen', getattr(sc,'n_evals_seen',0))
