"""Stopped-owner probe cleanup entry point; no target, signal, reconnect or release.

Only after a new explicit stopped-case binding and actual stop evidence exist.
Uses the original guard owner API without creating a FRAME, including HOLD mode.
"""
import http.client
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
    clock = time.get_clock_info('monotonic')
    if not (sys.executable == str(HARNESS / '.venv/bin/python')
            and tuple(sys.version_info[:3]) == (3, 14, 0) and sys.platform == 'darwin'
            and sys.implementation.name == 'cpython' and clock.monotonic and not clock.adjustable):
        raise RuntimeError('Original harness clock required before cleanup imports')

from editor_pointer_parent import pinned, need, command_lock, FROZEN_STATE
from editor_probe_namespace import settle_stopped, release_cleanup_facts, save

STATE = 'ROOT_ADMITTED_STOPPED_EDITOR_PROBE_NAMESPACE_ONLY'


class HostObservations:
    """Finite read-only process and cached health observations; no guest connection."""
    def command(self, argv, expected):
        result = subprocess.run(argv, capture_output=True, text=True, timeout=10)
        need(result.returncode in expected and not result.stderr
             and len(result.stdout.encode()) <= 1048576, 'Bounded host absence observation refused')
        return result.stdout

    def health(self):
        connection = http.client.HTTPConnection('127.0.0.1', 3000, timeout=3)
        try:
            connection.request('GET', '/health', headers={'Connection': 'close'})
            response = connection.getresponse(); body = response.read(65537)
            need(response.status == 200 and len(body) <= 65536, 'Bounded cached bridge health required')
            value = json.loads(body)
        finally:
            connection.close()
        need(value['serial']['host'] == '127.0.0.1' and value['serial']['port'] == 2345
             and value['serial']['connected'] is False, 'Original emulator bridge must remain disconnected')
        return value['serial']

    def socket_absent(self):
        return not os.path.lexists('/tmp/amiberry.sock')


def validate(binding):
    b = binding
    need(b['state'] == STATE and b['acceptedByRoot'] is True and b['target'] == 'amiberry-030'
         and b['mode'] in ('normal', 'failed') and b['limits'] == {'whole': 60}
         and b['allowedActions'] == {'probeFilesRemoved': 1, 'probeDirectoriesRemoved': 1,
              'targetCommands': 0, 'signals': 0, 'serviceChanges': 0, 'guardReleases': 0},
         'Distinct stopped-probe-only admission required')
    parent = json.loads(pinned(b['parentBindingRef']))
    probe = parent['passiveProbe']; window = Path(probe['windowDirectory'])
    need(window.is_absolute() and '..' not in window.parts
         and str(window).startswith('/private/tmp/pt24g-')
         and b['parentBindingRef']['path'] == str(window / 'mouse-parent-staged-binding.json')
         and parent['state'] == 'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2'
         and parent['ownerRunID'] == probe['ownerRunID'] == b['ownerRunID']
         and b['windowDirectory'] == str(window), 'Actual same-owner staged-parent record required')
    freeze = json.loads(pinned(parent['sourceFreezeRef']))
    need(freeze['state'] == FROZEN_STATE and freeze['sources'] == parent['sourcePins']
         and str(Path(__file__)) in parent['sourcePins'], 'Current cleanup entry point must be frozen')
    for ref in parent['sourcePins'].values(): pinned(ref)
    need(b['guardRef']['path'] == str(HARNESS / 'runtime/launcher-guard/state.json'),
         'Exact current original guard endpoint required')
    pinned(b['guardRef'])
    stop = json.loads(pinned(b['stopReceiptRef']))
    owned = json.loads(pinned(b['ownedProcessRef']))
    service = json.loads(pinned(b['serviceProcessRef']))
    need(b['stopReceiptRef']['path'] == str(window / 'ROOT_ACTUAL_OWNED_STOP.json')
         and b['ownedProcessRef']['path'] == str(window / 'ROOT_CURRENT_STARTED_PROCESS.json')
         and b['serviceProcessRef']['path'] == str(window / 'ROOT_CURRENT_DEVBENCH_PROCESS.json')
         and stop['ownerRunID'] == b['ownerRunID'] and stop['processAndGroupAbsent'] is True
         and type(owned['pid']) is int and owned['pid'] > 1 and stop['pid'] == owned['pid']
         and type(owned['argv']) is list and len(owned['argv']) == 3
         and owned['argv'][:2] == [str(owned['pid']), str(owned['pid'])]
         and parent['blankBinding']['ownedProcess'] == owned
         and type(service['pid']) is int and service['pid'] > 1
         and type(service['argv']) is list and len(service['argv']) == 3
         and service['argv'][0] == str(service['pid']), 'New own stop/process/service association required')
    return parent, probe, stop, owned, service


def load_dependencies(parent):
    sys.path.insert(0, str(HARNESS / 'scripts'))
    modules = {}
    for name in ('launcher_guard', 'launcher_access'):
        path = str(HARNESS / 'scripts' / (name + '.py'))
        need(parent['blankBinding']['modulePaths'][name] == path, 'Original guard module path differs')
        pinned(parent['sourcePins'][path])
        spec = importlib.util.find_spec(name)
        need(spec and spec.origin == path, 'Original guard resolution differs before import')
        module = importlib.import_module(name)
        need(str(Path(module.__file__)) == path, 'Actual guard module import differs')
        pinned(parent['sourcePins'][path]); modules[name] = module
    return SimpleNamespace(guard=modules['launcher_guard'].LauncherGuard,
                           lease_input=modules['launcher_access'].lease_input,
                           observations=HostObservations())


class StoppedOwner:
    def __init__(self, binding, validated, dependencies):
        self.b = binding; self.parent, self.probe, _, self.owned, self.service = validated
        self.guard = dependencies.guard(HARNESS)
        self.lease = dependencies.lease_input(HARNESS, 'amiberry-030')
        need(self.lease['run_id'] == binding['ownerRunID'], 'Current original lease owner changed')
        self.host = dependencies.observations
        self.deadline = time.monotonic() + 60; self.observations = []

    def stopped_owner(self, owner, mode):
        b = self.b
        need(owner == b['ownerRunID'] and mode == b['mode'] and time.monotonic() < self.deadline,
             'Exact original stopped owner/mode/deadline required')
        for ref in (b['guardRef'], b['stopReceiptRef'], b['ownedProcessRef'], b['serviceProcessRef']): pinned(ref)
        status = self.guard.status()
        need(status['phase'] == ('HOLD' if mode == 'failed' else 'READY')
             and bool(status['holds']) == (mode == 'failed') and status['inflight'] is None,
             'Exact original normal or retained HOLD state required')
        with self.guard._locked() as (_, state): self.guard._owner(state, self.lease, 'amiberry-030')
        service = self.host.command(['/bin/ps', '-p', str(self.service['pid']), '-o', 'pid=,pgid=,command='], (0,))
        need(service.strip().split(None, 2) == self.service['argv'], 'Current shared service identity changed')
        need(not self.host.command(['/usr/bin/pgrep', '-x', 'Amiberry'], (1,)), 'Emulator process remains')
        rows = self.host.command(['/bin/ps', '-axo', 'pid=,pgid='], (0,))
        need(all(str(self.owned['pid']) not in line.split() for line in rows.splitlines()),
             'Owned emulator PID or process group remains')
        need(self.host.socket_absent(), 'Canonical emulator socket remains')
        need(not self.host.command(['/usr/sbin/lsof', '-nP', '-iTCP:2345', '-Fpcn'], (1,)),
             'Emulator bridge listener remains')
        health = self.host.health()
        need(health['host'] == '127.0.0.1' and health['port'] == 2345 and health['connected'] is False,
             'Cached emulator bridge state changed')
        pinned(b['guardRef'])
        need(time.monotonic() < self.deadline, 'Stopped-owner observation deadline exhausted')
        self.observations.append({'mode': mode, 'guardFence': status['fence'],
             'ownedPIDAndGroupAbsent': self.owned['pid'], 'servicePID': self.service['pid'],
             'canonicalSocketAndPortAbsent': True, 'bridgeDisconnected': True,
             'observedMonotonic': time.monotonic()})


def execute(binding, dependencies=None):
    b = binding
    with command_lock(HARNESS / 'runtime/test.lock') as fd:
        verified = validate(b)
        d = dependencies if dependencies is not None else load_dependencies(verified[0])
        owner = StoppedOwner(b, verified, d)
        window = Path(b['windowDirectory'])
        need(not (window / 'probe-stopped-intent.json').exists()
             and not (window / 'probe-cleanup-fact.json').exists()
             and not (window / 'probe-stopped-observations.json').exists(), 'Never replay a stopped-probe actor')
        owner.stopped_owner(b['ownerRunID'], b['mode'])
        save(window / 'probe-stopped-intent.json', {'state': 'ONE_OWN_STOPPED_PROBE_CLEANUP_INTENT',
             'ownerRunID': b['ownerRunID'], 'mode': b['mode'], 'guardRef': b['guardRef'], 'retryAllowed': False})
        result = {'state': 'STOPPED_PROBE_CLEANUP_STARTED', 'mode': b['mode'],
                  'targetOperations': 0, 'guardReleaseClaimed': False, 'retryAllowed': False}
        try:
            probe = verified[1]; stage_ref = probe['namespaceStageRef']
            settled = settle_stopped(probe, stage_ref, window / 'passive-probe-settlement',
                                     owner, mode=b['mode'], lock_fd=fd)
            facts = release_cleanup_facts(probe, stage_ref, settled, owner, mode=b['mode'], lock_fd=fd)
            fact_ref = save(window / 'probe-cleanup-fact.json', facts)
            result.update(state='STOPPED_PROBE_CLEANUP_FACT_RECORDED_NO_RELEASE', probeCleanupFactRef=fact_ref)
            return fact_ref
        except BaseException as error:
            result.update(state='STOPPED_PROBE_CLEANUP_FAILED_RETAINED',
                          errorType=type(error).__name__, error=str(error)[:2048])
            raise
        finally:
            result['absenceObservations'] = owner.observations
            save(window / 'probe-stopped-observations.json', result)


def release_cleanup_supplement(binding, fact_ref, dependencies=None):
    """Required fresh probe addition to either original cleanup release record.

    The caller adds this data before hashing its original cleanup record. This
    does not write the other app facts, construct their proof, or release a guard.
    """
    b = binding
    with command_lock(HARNESS / 'runtime/test.lock') as fd:
        verified = validate(b)
        d = dependencies if dependencies is not None else load_dependencies(verified[0])
        owner = StoppedOwner(b, verified, d)
        need(fact_ref['path'] == str(Path(b['windowDirectory']) / 'probe-cleanup-fact.json'),
             'Actual original stopped-probe fact record required')
        actual = json.loads(pinned(fact_ref))
        fresh = release_cleanup_facts(verified[1], verified[1]['namespaceStageRef'],
            actual['probeSettlementRef'], owner, mode=b['mode'], lock_fd=fd)
        need(actual == fresh, 'Probe cleanup fact differs from current independently checked evidence')
        return {'probeNamespaceCleanup': {'factRef': fact_ref, 'facts': fresh}}


def main():
    os.umask(0o077); sys.dont_write_bytecode = True
    need(len(sys.argv) == 3 and sys.argv[1] == '--binding-ref', 'Actual new stopped-probe binding required')
    ref = json.loads(sys.argv[2]); binding = json.loads(pinned(ref))
    need(ref['path'] == str(Path(binding['windowDirectory']) / 'probe-stopped-binding.json'),
         'Exact new private stopped-case binding path required')
    fact = execute(binding)
    print(json.dumps({'state': 'PROBE_CLEANUP_FACT_RECORDED_NO_GUARD_RELEASE', 'fact': fact}))


if __name__ == '__main__': main()
