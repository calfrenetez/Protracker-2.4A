"""Distinct stopped V3 validation/retention through the unchanged original owner.

No target operation, signal, reconnect, service change, forced release or CLI.
"""
from contextlib import contextmanager
import json
from pathlib import Path
from editor_pointer_parent import pinned, need, command_lock
from editor_probe_namespace import save, checked_command_lock
import editor_context_binding as binding_source
import editor_context_retention as retention
from editor_probe_stopped import StoppedOwner, load_dependencies, HARNESS

STATE = 'ROOT_ADMITTED_STOPPED_EDITOR_CONTEXT_V3_RETENTION_ONLY'
ALLOWED = {'probeFilesRemoved': 0, 'probeDirectoriesRemoved': 0, 'inputPreservations': 1,
           'targetCommands': 0, 'signals': 0, 'serviceChanges': 0, 'guardReleases': 0}


def validate(binding):
    b = binding
    need(b['state'] == STATE and b['acceptedByRoot'] is True and b['target'] == 'amiberry-030'
         and b['mode'] in ('normal', 'failed') and b['limits'] == {'whole': 60}
         and b['allowedActions'] == ALLOWED, 'Distinct stopped V3 retention scope required')
    parent = json.loads(pinned(b['parentBindingRef']))
    binding_source.validate(parent)
    probe = parent['passiveProbe']; window = Path(probe['windowDirectory'])
    need(b['parentBindingRef']['path'] == str(window / 'context-parent-binding.json')
         and parent['ownerRunID'] == b['ownerRunID'] and b['windowDirectory'] == str(window)
         and str(Path(__file__)) in parent['sourcePins'], 'Same actual V3 parent and selected stopped source required')
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
         and service['argv'][0] == str(service['pid']), 'New V3 own stop/process/service association required')
    return parent, probe, stop, owned, service


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
            probe = verified[1]
            settled = retention.preserve_stopped(probe, window / 'preserved-readonly-context-input',
                                                 owner, mode=b['mode'], lock_fd=fd)
            facts = retention.release_facts(probe, settled, owner, mode=b['mode'], lock_fd=fd)
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


@contextmanager
def cleanup_lock(lock_fd):
    if lock_fd is None:
        with command_lock(HARNESS / 'runtime/test.lock') as fd:
            yield fd
    else:
        checked_command_lock(lock_fd)
        try:
            yield lock_fd
        finally:
            checked_command_lock(lock_fd)


def release_cleanup_supplement(binding, fact_ref, dependencies=None, *, lock_fd=None):
    """Required fresh probe addition to either original cleanup release record.

    The caller adds this data before hashing its original cleanup record. This
    does not write the other app facts, construct their proof, or release a guard.
    """
    b = binding
    with cleanup_lock(lock_fd) as fd:
        verified = validate(b)
        d = dependencies if dependencies is not None else load_dependencies(verified[0])
        owner = StoppedOwner(b, verified, d)
        need(fact_ref['path'] == str(Path(b['windowDirectory']) / 'probe-cleanup-fact.json'),
             'Actual original stopped-probe fact record required')
        actual = json.loads(pinned(fact_ref))
        fresh = retention.release_facts(verified[1], actual['contextInput']['resultRef'],
                                         owner, mode=b['mode'], lock_fd=fd)
        need(actual == fresh, 'V3 input fact differs from current independently checked evidence')
        return {'contextInputRetention': {'factRef': fact_ref, 'facts': fresh}}
