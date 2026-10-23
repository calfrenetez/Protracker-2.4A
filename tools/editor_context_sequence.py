"""One fresh actual-start binder followed immediately by the existing V3 dispatch.

Eliminates a manual host gap, without refreshing a pin or extending either actor.
No reservation, restart, profile mutation, signal, retry, cleanup or release.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import time

HARNESS = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
if __name__ == '__main__':
    clock = time.get_clock_info('monotonic')
    if not (sys.executable == str(HARNESS / '.venv/bin/python')
            and tuple(sys.version_info[:3]) == (3, 14, 0) and sys.platform == 'darwin'
            and sys.implementation.name == 'cpython' and clock.monotonic and not clock.adjustable):
        raise RuntimeError('Original harness clock required before sequence imports')

import editor_pointer_parent as parent
import editor_context_start as start
import editor_context_dispatch as dispatch
from editor_probe_namespace import save

BINDER_SUPERVISOR_SECONDS = 180  # Original binder60 plus120 overhead; no extension.
DISPATCH_GAP_MAXIMUM = 30
AUTHORITY = ('AMIGA_LAUNCH_LEASE', 'AMIGA_LAUNCH_INFLIGHT')


def validate(ref):
    parent.need(not any(k in os.environ for k in AUTHORITY), 'Unexpected inherited sequence authority')
    record = json.loads(parent.pinned(ref)); freeze, _, _, _, owner = start.validate_actual(record)
    window = Path(record['windowDirectory'])
    parent.need(ref['path'] == str(window / 'actual-start-binding.json')
                and str(Path(__file__)) in freeze['sources'],
                'Actual fresh start binding and independently selected sequence source required')
    parent.pinned(freeze['sources'][str(Path(__file__))])
    for name in ('bind-dispatch-intent.json', 'binder-child.json', 'binder.stdout', 'binder.stderr',
                 'bind-dispatch-failed.json', 'bind-dispatch-result.json', 'binder-supervisor-timeout.json'):
        parent.need(not os.path.lexists(window / name), 'Never replay a binding/dispatch sequence')
    return record, freeze, owner, window


def load_dependencies(freeze):
    # Same original pinned guard loader as the existing finite dispatcher.
    selected = {'blankBinding': {'modulePaths': freeze['modulePaths']}, 'sourcePins': freeze['sources']}
    d = dispatch.load_dependencies(selected)
    d.dispatch = dispatch.execute
    return d


def execute(ref, dependencies=None):
    record, freeze, owner_id, window = validate(ref)
    d = dependencies if dependencies is not None else load_dependencies(freeze)
    lease_body, lease_ref = parent.read(window / 'private-lease.json')
    lease = d.decode(lease_body)['lease']
    parent.need(lease['run_id'] == owner_id, 'Exact original fresh sequence owner required')
    guard = d.guard(HARNESS)
    def owner():
        parent.pinned(ref); parent.pinned(lease_ref)
        for source in freeze['sources'].values(): parent.pinned(source)
        parent.pinned(record['activeProfileRef'])
        state = guard.status()
        parent.need(state['phase'] == 'READY' and not state['holds'] and state['inflight'] is None,
                    'Original READY owner with no operation required')
        with guard._locked() as (_, raw): guard._owner(raw, lease, 'amiberry-030')
    with parent.command_lock(HARNESS / 'runtime/test.lock'):
        owner()
        save(window / 'bind-dispatch-intent.json', {
            'state': 'ONE_FRESH_V3_BINDER_THEN_IMMEDIATE_ORIGINAL_DISPATCH', 'sourceBindingRef': ref,
            'binderSupervisorSeconds': BINDER_SUPERVISOR_SECONDS, 'dispatchGapMaximum': DISPATCH_GAP_MAXIMUM,
            'parentSupervisorSeconds': dispatch.SUPERVISOR_SECONDS, 'profilePinRefreshes': 0,
            'targetAuthorityExpansions': 0, 'signals': 0, 'retryAllowed': False})
    env = dict(os.environ); env['AMIGA_LAUNCH_LEASE'] = str(window / 'private-lease.json')
    argv = [str(HARNESS / '.venv/bin/python'), '-B', str(Path(start.__file__)),
            '--actual-binding-ref', json.dumps(ref, sort_keys=True)]
    child = None
    try:
        with (window / 'binder.stdout').open('xb') as out, (window / 'binder.stderr').open('xb') as err:
            child = d.popen(argv, cwd=HARNESS, env=env, stdout=out, stderr=err)
            save(window / 'binder-child.json', {'pid': child.pid, 'argv': argv})
            try: child.wait(timeout=BINDER_SUPERVISOR_SECONDS)
            except subprocess.TimeoutExpired:
                save(window / 'binder-supervisor-timeout.json', {'pid': child.pid,
                    'childOrTargetUncertainRetained': True, 'signals': 0, 'retryAllowed': False})
                raise
        gap_started = d.clock()
        parent.need(child.returncode == 0, 'Binder failed; retain without parent dispatch or replay')
        output, output_ref = parent.read(window / 'binder.stdout', 65536)
        value = json.loads(output); binding_ref = value['ref']
        parent.need(value['applicationLaunches'] == 0 and value['lifecyclePending'] is True
                    and binding_ref['path'] == str(window / 'context-parent-binding.json'),
                    'Actual successful fresh binder output required')
        b = json.loads(parent.pinned(binding_ref))
        parent.need(b['sourceFreezeRef'] == record['sourceFreezeRef'] and b['ownerRunID'] == owner_id
                    and b['passiveProbe']['activeProfile'] == record['activeProfileRef'],
                    'Generated binding must retain exact original owner/freeze/profile pin')
        # No metadata refresh. The original full-facts pin must still hold.
        with parent.command_lock(HARNESS / 'runtime/test.lock'): owner()
        parent.pinned(output_ref)
        parent.need(d.clock() - gap_started <= DISPATCH_GAP_MAXIMUM,
                    'Finite automatic bind/dispatch gap exhausted; do not dispatch')
        result = d.dispatch(binding_ref)
        result_ref = save(window / 'bind-dispatch-result.json', {
            'state': 'FRESH_V3_BINDER_AND_ORIGINAL_DISPATCH_RETURNED_LIFECYCLE_PENDING',
            'binderOutputRef': output_ref, 'parentBindingRef': binding_ref,
            'dispatch': result, 'signals': 0, 'profilePinRefreshes': 0,
            'retryAllowed': False, 'guardReleaseClaimed': False})
        return {'ref': result_ref, 'returncode': result['returncode'], 'lifecyclePending': True}
    except BaseException as error:
        save(window / 'bind-dispatch-failed.json', {
            'state': 'FRESH_V3_SEQUENCE_FAILED_RETAINED_NO_REPLAY',
            'binderPID': child.pid if child is not None else None,
            'errorType': type(error).__name__, 'error': str(error)[:2048],
            'signals': 0, 'profilePinRefreshes': 0, 'retryAllowed': False,
            'guardReleaseClaimed': False})
        raise


def main():
    os.umask(0o077); sys.dont_write_bytecode = True
    parent.need(len(sys.argv) == 3 and sys.argv[1] == '--actual-binding-ref',
                'Exact new actual-start binding reference required')
    result = execute(json.loads(sys.argv[2])); print(json.dumps(result))
    raise SystemExit(result['returncode'])


if __name__ == '__main__': main()
