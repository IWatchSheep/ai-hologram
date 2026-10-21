"""Stage 4 CSV rows and demonstration text (plan 3.11 and 5), from the scoring reference."""
import sys
sys.path.insert(0, '.')
from ref_score import *

RUN_HEADER = ("run_id,condition,layout_seed,run_seed,outcome,decision_steps,world_steps,safe_stops,"
              "justified_safe_stops,mismatches,mismatches_without_fault_cause,source_labels_correct,"
              "source_labels_scored,claims,false_alarms,wrong_accepted_corrections,evaluations,"
              "events_scheduled,events_activated")
EVENT_HEADER = ("event_id,condition,reasoner,layout_seed,run_seed,type,target,common_mode,cell,direction,"
                "order,action,rotation,scheduled_step,reported_as,activation_step,activation_delay,"
                "not_activated_reason,opportunity,opportunity_step,revealing_opportunity,mismatch_detected,"
                "mismatch_step,strictly_detected,strict_step,detection_delay,end_state,end_step,"
                "correction_status,verified_correction")


def b(x): return '1' if x else '0'
def opt(x): return '' if x is None else str(x)


def run_row(R, cond, ls, rs):
    sc = R['scorer']
    activated = sum(1 for _, cls, _ in R['records'] if cls.startswith('Activated'))
    f = ['%d-%d' % (ls, rs), cond, str(ls), str(rs), R['outcome'], str(R['decisions']), str(R['world_steps']),
         str(R['safe_stops']), str(sc.justified), str(len(sc.mism)), str(sc.no_cause), str(sc.labels_correct),
         str(sc.labels_scored), str(len(sc.claims)), str(sc.false_alarms), str(sc.wrong_accepted),
         str(R['evals']), str(len(R['records'])), str(activated)]
    return ','.join(f)


def event_row(R, cond, e, rec):
    ev, cls, r = rec
    t = ev['type']
    cell = direction = order = action = rot = ''
    if t in ('F1', 'F4') and ev['cell'] is not None: cell = '%d:%d' % ev['cell']
    if t == 'F1': direction = ev['dir']
    if t == 'F2': order = ' '.join(ev['order'])
    if t == 'F3': action = ev['action']; rot = ev['rot']
    ls, rs, _ = ev['id'].split('-')
    activated = cls.startswith('Activated')
    reason = r['reason'] if cls in ('Not activated: not applicable at the scheduled step',
                                    'Not activated: deferred, never applicable') else ''
    f = [ev['id'], cond, 'R', ls, rs, t, ev['target'], b(ev['common']), cell, direction, order, action, rot,
         str(ev['step']), cls]
    if not activated:
        f += ['', '', reason] + [''] * 12
    else:
        f += [str(r['step']), str(r['step'] - ev['step']), '']
        end = e.ended
        if end is None: es, est = 'Active at end of run', ''
        else: es, est = end[0], str(end[1])
        cs = e.correcting['status'] if (end is not None and end[0] == 'Corrected' and e.correcting) else ''
        verified = cs == 'VERIFIED'
        f += [b(e.opp_step is not None), opt(e.opp_step), b(e.revealing),
              b(e.mismatch_step is not None), opt(e.mismatch_step),
              b(e.strict_step is not None), opt(e.strict_step),
              opt(None if e.strict_step is None else e.strict_step - e.act_step),
              es, est, cs, b(verified)]
    return ','.join(f)


def activation_line(rec):
    ev, cls, r = rec
    s = '  %s %s: %s' % (ev['id'], ev['type'], cls)
    if cls == 'Activated at its scheduled step': s += ' (step %d)' % r['step']
    elif cls == 'Activated after deferral': s += ' (step %d, delay %d)' % (r['step'], r['step'] - ev['step'])
    elif r['reason']: s += ' (%s)' % r['reason']
    return s


def result_line(R, ls, rs, cond):
    return ('Run %d-%d, %s: %s after %d world steps, %d decision steps, %d safe stops, %d model evaluations'
            % (ls, rs, cond, R['outcome'], R['world_steps'], R['decisions'], R['safe_stops'], R['evals']))


def demo_lines():
    L = []
    lays = {}
    for seed in range(500000, 500004):
        lays[seed] = make_layout(seed)
    runs = [(seed, rs, lays[seed][0]) for seed in range(500000, 500004) for rs in (1, 2)]
    EV = make_events(runs)
    s0, att, sh = lays[500000]
    L += ['', 'Stage 4: the measuring instrument, on demonstration material (not an experiment)',
          'Faults are injected into a reasoner\'s beliefs or reasoning, never into the world.',
          'Demonstration seeds 500000-500003 only; no pilot or confirmatory layout is used.',
          '', 'Layout 500000 (found at attempt %d, shortest solution %d world steps):' % (att, sh)]
    L += layout_text(s0)
    L += ['', 'Fault events of runs 500000-1 and 500003-1 (fault master seed 4242):']
    idx = {(ls, rs): i for i, (ls, rs, _) in enumerate(runs)}
    for key in ((500000, 1), (500003, 1)):
        L += [ev_text(e) for e in EV[idx[key]]]
    run_rows, ev_rows = [], []
    for key in ((500000, 2), (500003, 1)):
        i = idx[key]; s = runs[i][2]
        for cond in ('C1', 'C0'):
            R = score_run(cond, s, EV[i])
            L += ['', result_line(R, key[0], key[1], cond)]
            L += [activation_line(rec) for rec in R['records']]
            run_rows.append(run_row(R, cond, *key))
            ev_rows += [event_row(R, cond, e, rec) for e, rec in zip(R['scorer'].ev, R['records'])]
    L += ['', 'runs.csv:', RUN_HEADER] + run_rows
    L += ['', 'events.csv:', EVENT_HEADER] + ev_rows
    L += ['', 'These lines show the instrument working; they are not results and are never compared.']
    return L


if __name__ == '__main__':
    print('\n'.join(demo_lines()))
