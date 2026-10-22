"""One finite supervisor for a newly admitted stage-plan parent.

No reservation, target exchange, service change, signal, retry or release.
An expired supervisor retains its child and files for independent inspection.
"""
import importlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from types import SimpleNamespace

HARNESS = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')

if __name__ == '__main__':
    info = time.get_clock_info('monotonic')
    if not (sys.executable == str(HARNESS / '.venv/bin/python')
            and tuple(sys.version_info[:3]) == (3, 14, 0) and sys.platform == 'darwin'
            and sys.implementation.name == 'cpython' and info.monotonic and not info.adjustable):
        raise RuntimeError('Original harness clock required before dispatcher imports')

import editor_pointer_parent as parent
from editor_probe_namespace import save, directory_ref

# Original blank300 + original mouse240 + explicit stage60, plus120 overhead.
# No subphase or actual Root observation budget is extended.
SUPERVISOR_SECONDS = 720
INHERITED_AUTHORITY = ('AMIGA_LAUNCH_LEASE', 'AMIGA_LAUNCH_INFLIGHT')


def validate(ref):
    binding = json.loads(parent.pinned(ref))
    readonly = binding.get('state') == parent.READONLY_STATE
    freeze = parent.validate_binding(binding, readonly=True) if readonly else parent.validate_binding(binding, stage_plan=True)
    window = Path(binding['outputDirectory']).parent
    parent.need(ref['path'] == str(window / 'mouse-parent-binding.json')
                and str(Path(__file__)) in binding['sourcePins'],
                'Exact new parent binding and frozen dispatcher required')
    directory_ref(window)
    parent.need(SUPERVISOR_SECONDS >= (0 if readonly else binding['limits']['probeStage'])
                + binding['limits']['whole'] + binding['blankBinding']['limits']['whole'] + 120,
                'Supervisor must include every unchanged finite subphase plus overhead')
    parent.need(not any(name in os.environ for name in INHERITED_AUTHORITY),
                'Unexpected inherited target authority; no dispatch')
    for name in ('capture-dispatch-intent.json', 'capture-child.json', 'capture.stdout',
                 'capture.stderr', 'capture-supervisor-timeout.json',
                 'capture-dispatch-result.json', 'capture-supervisor-failure.json'):
        parent.need(not os.path.lexists(window / name), 'Never replay an original parent dispatch')
    return binding, freeze, window


def load_dependencies(binding):
    path = str(HARNESS / 'scripts/launcher_guard.py')
    parent.need(binding['blankBinding']['modulePaths']['launcher_guard'] == path,
                'Original supervisor guard module path differs')
    parent.pinned(binding['sourcePins'][path])
    sys.path.insert(0, str(HARNESS / 'scripts'))
    spec = importlib.util.find_spec('launcher_guard')
    parent.need(spec and spec.origin == path, 'Original guard resolution differs before import')
    module = importlib.import_module('launcher_guard')
    parent.need(str(Path(module.__file__)) == path, 'Actual original guard module differs')
    parent.pinned(binding['sourcePins'][path])
    return SimpleNamespace(guard=module.LauncherGuard, decode=module._decode,
                           popen=subprocess.Popen, clock=time.monotonic)


def execute(ref, dependencies=None):
    """Delegate one actual lease to one child; never inherit another inflight frame."""
    binding, _, window = validate(ref)
    d = dependencies if dependencies is not None else load_dependencies(binding)
    lease_body, lease_ref = parent.read(window / 'private-lease.json')
    lease = d.decode(lease_body)['lease']
    parent.need(lease['run_id'] == binding['ownerRunID'], 'Actual new private lease owner differs')
    guard = d.guard(HARNESS)

    def owner():
        parent.pinned(ref); parent.pinned(lease_ref)
        for source in binding['sourcePins'].values(): parent.pinned(source)
        status = guard.status()
        parent.need(status['phase'] == 'READY' and not status['holds'] and status['inflight'] is None,
                    'Fresh original READY reservation required before dispatch')
        with guard._locked() as (_, state): guard._owner(state, lease, 'amiberry-030')

    # This lock is released before spawning: the child must acquire the same
    # original command lock itself and independently recheck owner and FRAME.
    with parent.command_lock(HARNESS / 'runtime/test.lock'):
        owner()
        save(window / 'capture-dispatch-intent.json', {
            'state': ('ONE_READONLY_PASSIVE_V2_PARENT_DISPATCH_INTENT' if binding['state'] == parent.READONLY_STATE
                      else 'ONE_PASSIVE_V2_STAGE_PLAN_PARENT_DISPATCH_INTENT'),
            'bindingRef': ref, 'supervisorSeconds': SUPERVISOR_SECONDS,
            'probeStagesMaximum': 0 if binding['state'] == parent.READONLY_STATE else 1, 'applicationLaunchesMaximum': 1,
            'buttonFreeMovesMaximum': 1, 'leftClicksMaximum': 1,
            'keysPlaybackSignalsServiceChangesAndRelease': 0, 'retryAllowed': False})
    env = dict(os.environ); env['AMIGA_LAUNCH_LEASE'] = str(window / 'private-lease.json')
    argv = [str(HARNESS / '.venv/bin/python'), '-B', str(Path(parent.__file__)),
            '--binding-ref', json.dumps(ref, sort_keys=True)]
    started = d.clock(); child = None
    try:
        with (window / 'capture.stdout').open('xb') as out, (window / 'capture.stderr').open('xb') as err:
            child = d.popen(argv, cwd=HARNESS, env=env, stdout=out, stderr=err)
            save(window / 'capture-child.json', {'pid': child.pid, 'argv': argv})
            try:
                # Popen.wait does not kill on timeout. Do not use run(timeout=)
                # or a Popen context manager, which would change this lifetime.
                child.wait(timeout=SUPERVISOR_SECONDS)
            except subprocess.TimeoutExpired:
                save(window / 'capture-supervisor-timeout.json', {
                    'state': 'CHILD_OR_TARGET_UNCERTAIN_RETAINED_NO_SIGNAL',
                    'pid': child.pid, 'retryAllowed': False, 'signalsSent': 0})
                raise
        result = {'state': 'PASSIVE_V2_PARENT_RETURNED_LIFECYCLE_PENDING',
                  'returncode': child.returncode, 'seconds': d.clock() - started,
                  'retryAllowed': False, 'signalsSent': 0, 'guardReleaseClaimed': False}
        save(window / 'capture-dispatch-result.json', result)
        return result
    except BaseException as error:
        save(window / 'capture-supervisor-failure.json', {
            'state': 'SUPERVISOR_FAILED_CHILD_OR_TARGET_RETAINED',
            'pid': child.pid if child is not None else None,
            'errorType': type(error).__name__, 'error': str(error)[:2048],
            'signalsSent': 0, 'retryAllowed': False, 'guardReleaseClaimed': False})
        raise


def main():
    os.umask(0o077); sys.dont_write_bytecode = True
    parent.need(len(sys.argv) == 3 and sys.argv[1] == '--binding-ref',
                'Actual newly admitted stage-plan parent binding reference required')
    result = execute(json.loads(sys.argv[2]))
    print(json.dumps(result)); raise SystemExit(result['returncode'])


if __name__ == '__main__': main()
