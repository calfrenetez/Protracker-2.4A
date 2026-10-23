"""Independent stopped restoration for a V3 parent refused before any app operation.

This never changes the failed runtime freeze/admission. Exact current stopped
ownership, retained failure and namespace absence are prerequisites. No guest,
signal, service change, application namespace deletion or transaction replay.
"""
import json
import os
from pathlib import Path
import time

import editor_context_settlement as full
from editor_context_settlement import (parent, stopped, app_namespace, save,
    directory_ref, load_dependencies, app_data_rows, audit_app_data,
    restore_theme_mtimes, verify_app_data)
from editor_probe_namespace import checked_command_lock

HARNESS = full.HARNESS
STATE = 'ROOT_ACCEPTED_STOPPED_V3_PREAPPLICATION_RESTORATION_AND_RELEASE'
ALLOWED = {'probeSettlements': 1, 'appNamespaceDeletions': 0,
           'hostRoleRestorations': 3, 'themeMtimeRestorationsMaximum': 2,
           'guardReleases': 1, 'targetCommands': 0, 'signals': 0, 'serviceChanges': 0}
FAILURE = 'CURRENT_PROFILE_CTIME_CHANGED_PARENT_REFUSED_BEFORE_OPERATION'


def failure_evidence(b, p):
    window = Path(b['windowDirectory'])
    refs = {'failureRef': 'parent-pre-operation-failure.json',
            'dispatchResultRef': 'capture-dispatch-result.json',
            'stderrRef': 'capture.stderr', 'stdoutRef': 'capture.stdout'}
    bodies = {}
    for key, name in refs.items():
        parent.need(b[key]['path'] == str(window / name), 'Exact failed run evidence path required')
        bodies[key] = parent.pinned(b[key])
    failure = json.loads(bodies['failureRef']); dispatch = json.loads(bodies['dispatchResultRef'])
    parent.need(failure['state'] == FAILURE and failure['ownerRunID'] == b['ownerRunID']
                and failure['actualOwnedProcess'] == p['blankBinding']['ownedProcess']
                and all(type(failure[k]) is int and failure[k] == 0 for k in
                        ('applicationLaunches', 'passiveNativeExecutions', 'guiInputEvents'))
                and failure['retryAllowed'] is False,
                'Actual own pre-operation failure with zero application/probe/input required')
    parent.need(dispatch['state'] == 'CONTEXT_V3_PARENT_RETURNED_LIFECYCLE_PENDING'
                and type(dispatch['returncode']) is int and dispatch['returncode'] == 1
                and dispatch['signalsSent'] == 0 and dispatch['guardReleaseClaimed'] is False
                and dispatch['retryAllowed'] is False and bodies['stdoutRef'] == b''
                and b"original.pinned(p['activeProfile'])" in bodies['stderrRef']
                and bodies['stderrRef'].endswith(b'RuntimeError: Actual pinned input changed\n'),
                'Preserved actual parent profile-pin refusal required; never infer app absence from exit alone')


def validate(b):
    parent.need(b['state'] == STATE and b['acceptedByRoot'] is True
                and b['allowedActions'] == ALLOWED and b['limits'] == {'whole': 180}
                and b['mode'] == 'failed', 'Distinct independent preapplication failed-stop scope required')
    sb = json.loads(parent.pinned(b['stoppedBindingRef'])); verified = stopped.validate(sb)
    p = verified[0]; window = Path(sb['windowDirectory']); directory_ref(window)
    parent.need(b['ownerRunID'] == sb['ownerRunID'] == p['ownerRunID']
                and b['mode'] == sb['mode'] and b['windowDirectory'] == str(window)
                and b['stoppedBindingRef']['path'] == str(window / 'probe-stopped-binding.json'),
                'Same original stopped run required')
    source = b['independentCleanupSourceRef']
    parent.need(source['path'] == str(Path(__file__)), 'Exact independent cleanup source required')
    parent.pinned(source)
    qualification = json.loads(parent.pinned(b['independentHostQualificationRef']))
    parent.need(qualification['state'] == 'PREAPPLICATION_RESTORATION_HOST_PASSED_NOT_RUNTIME'
                and qualification['sourceRef'] == source and qualification['returncode'] == 0
                and type(qualification['testsPassed']) is int and qualification['testsPassed'] > 0,
                'Separate source-bound HOST qualification required; failed runtime freeze is immutable')
    for key, name in {'appDataBeforeRef': 'current-app-data-before-and-backups.json',
                      'preservationBeforeRef': 'fresh-preservation-before.json',
                      'roleBaselineRef': 'before-owned-lifecycle-files.json'}.items():
        parent.need(b[key]['path'] == str(window / name), 'Original saved restoration baseline required')
        parent.pinned(b[key])
    failure_evidence(b, p)
    return sb, verified, window


def namespace_absence(b, p, api, *, mode, lock_fd):
    checked_command_lock(lock_fd); api.stopped_owner(b['ownerRunID'], mode)
    failure_evidence(b, p)
    window, root, run, launch = app_namespace.endpoints(p)
    root_ref = directory_ref(root)
    endpoints = [Path(p['blankBinding']['outputDirectory']), run, launch]
    parent.need(window == Path(b['windowDirectory']) and all(not os.path.lexists(x) for x in endpoints),
                'Preapplication output/run/launcher must all remain absent; no removal is authorized')
    api.stopped_owner(b['ownerRunID'], mode); checked_command_lock(lock_fd)
    parent.need(directory_ref(root) == root_ref and all(not os.path.lexists(x) for x in endpoints),
                'Shared namespace identity or absence changed through observation')
    return {'state': 'PREAPPLICATION_NAMESPACE_ABSENT_WITHOUT_MUTATION',
            'ownerRunID': b['ownerRunID'], 'root': root_ref,
            'absentPaths': [str(x) for x in endpoints], 'filesRemoved': 0,
            'directoriesRemoved': 0, 'applicationLaunchClaimed': False,
            'sourceFailureRef': b['failureRef'], 'runtimeAcceptance': False}


def namespace_release_facts(b, p, ref, api, *, mode, lock_fd):
    parent.need(ref['path'] == str(Path(b['windowDirectory']) / 'preapplication-namespace-absence.json'),
                'Exact independent namespace absence fact required')
    actual = json.loads(parent.pinned(ref))
    parent.need(actual == namespace_absence(b, p, api, mode=mode, lock_fd=lock_fd),
                'Application namespace absence evidence changed')
    return {'source': ref, 'facts': actual}


def execute(binding, dependencies=None):
    b = binding; sb, verified, window = validate(b); p = verified[0]
    d = dependencies if dependencies is not None else load_dependencies(b, verified)
    f = d.files; deadline = time.monotonic() + 180
    parent.need(not any(os.path.lexists(window / n) for n in
        ('preapplication-settlement-intent.json', 'preapplication-settlement-result.json',
         'preapplication-settlement-failed.json', 'full-settlement-intent.json',
         'full-settlement-result.json', 'full-settlement-failed.json',
         'restore-admission.json', 'release-proof.json')), 'Never replay full stopped settlement')

    def owner():
        parent.need(time.monotonic() < deadline, 'Whole stopped settlement deadline exhausted; retain without replay')
        check = stopped.StoppedOwner(sb, verified, d)
        check.stopped_owner(b['ownerRunID'], b['mode'])
        return check

    owner(); rows = app_data_rows(b, p)
    audit = audit_app_data(rows)
    with parent.command_lock(HARNESS / 'runtime/test.lock') as fd:
        namespace_absence(b, p, owner(), mode=b['mode'], lock_fd=fd)
    save(window / 'preapplication-settlement-intent.json', {'state': b['state'], 'ownerRunID': b['ownerRunID'],
         'mode': b['mode'], 'allowedActions': ALLOWED, 'retryAllowed': False})
    result = {'state': 'FULL_STOPPED_SETTLEMENT_STARTED', 'mode': b['mode'],
              'targetOperations': 0, 'nativeCleanExitClaimed': False, 'runtimeAcceptance': False,
              'retryAllowed': False, 'releaseAttempted': False}
    try:
        probe_fact_ref = stopped.execute(sb, d)
        with parent.command_lock(HARNESS / 'runtime/test.lock') as fd:
            app_ref = save(window / 'preapplication-namespace-absence.json',
                namespace_absence(b, p, owner(), mode=b['mode'], lock_fd=fd))
        audit = audit_app_data(rows); save(window / 'app-data-after-stop-audit.json', audit)
        theme_ref = restore_theme_mtimes(rows, audit, window, f, owner)
        owner()
        admission = {'state': 'ROOT_ACCEPTED_CANONICAL_PROFILE_SESSION_FILE_ADMISSION',
            'acceptedByRoot': True, 'action': 'restore', 'ownerRunID': b['ownerRunID'], 'window': str(window),
            'noOwnedEmulatorOrIPCObservedByRoot': True, 'originalOwnershipAndRecoveryCheckedByRoot': True,
            'currentFull4s': {role: f.resident(path)[1] for role, path in f.ROLES.items()},
            'guardFull4': sb['guardRef'], 'ownedStopVerifiedByRoot': True, 'ownedStopFull4': sb['stopReceiptRef']}
        save(window / 'restore-admission.json', admission)
        a, ap, w = f.admissions(window / 'restore-admission.json', 'restore')
        restored_path = f.execute(a, ap, w, 'restore')
        restored_ref = f.resident(restored_path, True)[1]
        restored = json.loads(f.pinned(restored_ref, True)); role_scope = json.loads(parent.pinned(b['roleBaselineRef']))
        parent.need(restored['state'] == 'THREE_HOST_FILE_ORIGINAL_BODIES_RESTORED_AFTER_VERIFIED_OWNED_STOP'
                    and set(restored['restoredFull4s']) == set(f.ROLES) == set(role_scope), 'Complete original three-role restoration required')
        endpoints = {}
        for role, path in f.ROLES.items():
            body, now = f.resident(path); before = f.resident(window / ('before-' + role + '.bin'), True)[0]
            parent.need(body == before and all(now['fullFacts'][k] == role_scope[role]['fullFacts'][k]
                        for k in parent.FACTS if k != 'st_ctime_ns'), 'Saved role body/nine facts not restored')
            endpoints[role] = now
        theme_names = set(audit['changes']); app_after = verify_app_data(rows, theme_names)
        endpoint_ref = save(window / 'post-restoration-current-endpoints.json', {
            'state': 'THREE_RESTORED_ENDPOINTS_AND_ALL31_APPDATA_VERIFIED', 'roles': endpoints,
            'appData': app_after, 'historicalMetadataContinuityClaimed': False})
        protected = d.preservation.verify_current(json.loads(parent.pinned(b['preservationBeforeRef'])))
        with parent.command_lock(HARNESS / 'runtime/test.lock') as fd:
            check = owner()
            app_facts = namespace_release_facts(b, p, app_ref, check, mode=b['mode'], lock_fd=fd)
            probe_facts = stopped.release_cleanup_supplement(sb, probe_fact_ref, d, lock_fd=fd)
            parent.need(verify_app_data(rows, theme_names) == app_after, 'App data changed after restoration verification')
            for ref in endpoints.values(): f.pinned(ref)
            protected = d.preservation.verify_current(json.loads(parent.pinned(b['preservationBeforeRef'])))
            values = {
                'authorization': {'ownerRunID': b['ownerRunID'], 'target': 'amiberry-030',
                    'sourceBinding': b, 'userAuthorizedTestingAndVerifiedOwnRunCleanup': True},
                'restoration': {'threeSavedBodiesAndOriginalMtimesRestored': True,
                    'all31AppDataOriginalBodiesAndMtimesVerified': True, 'themeRestoration': theme_ref,
                    'sourceResult': restored_ref, 'currentEndpoints': endpoint_ref,
                    'historicalMetadataContinuityClaimed': False},
                'absence': {'ownedPIDAndGroupAbsent': verified[3]['pid'], 'canonicalSocketAndPortAbsent': True,
                    'bridgeDisconnected': True, 'sourceStop': sb['stopReceiptRef']},
                'cleanup': {'mode': b['mode'], 'preapplicationNamespaceAbsence': app_facts, **probe_facts,
                    'preservation': protected, 'failedEvidenceRetained': True,
                    'failedTransactionReplayed': False, 'nativeCleanExitClaimed': False}}
            records = {}
            for name, value in values.items():
                ref = save(window / (name + '-release-fact.json'), value); records[name] = ref['sha256']
            check = owner(); state = check.guard.status()
            proof = {'run_id': b['ownerRunID'], 'target': 'amiberry-030', 'fence': state['fence'],
                     'externally_verified': True, 'records': records, 'facts': {name: True for name in records}}
            save(window / 'release-proof.json', proof)
            # Recheck preserved app and probe facts immediately before the single
            # original public release; never clear HOLD or edit guard storage.
            namespace_release_facts(b, p, app_ref, check, mode=b['mode'], lock_fd=fd)
            stopped.release_cleanup_supplement(sb, probe_fact_ref, d, lock_fd=fd)
            parent.need(verify_app_data(rows, theme_names) == app_after, 'App data changed immediately before release')
            for ref in endpoints.values(): f.pinned(ref)
            d.preservation.verify_current(json.loads(parent.pinned(b['preservationBeforeRef'])))
            parent.need(time.monotonic() < deadline, 'Whole stopped settlement deadline exhausted before release')
            result['releaseAttempted'] = True
            released = check.guard.release(check.lease, proof)
            state = check.guard.status()
            parent.need(state['phase'] == 'EMPTY' and state['reservation'] is None
                        and not state['holds'] and state['inflight'] is None, 'Original release not independently observed')
            archive_body = parent.read(HARNESS / 'runtime/launcher-guard/history' / released['archive'])[0]
            archive = d.decode(archive_body)
            parent.need(archive['proof'] == proof and archive['reservation']['lease'] == check.lease,
                        'Original public release archive differs')
        result.update(state='PREAPPLICATION_V3_SAVED_FILES_RESTORED_AND_RELEASED', released=released,
                      archivedHolds=len(archive['holds']), restoredRoles=3, appDataFiles=31)
        save(window / 'preapplication-settlement-result.json', result)
        return result
    except BaseException as error:
        result.update(state='PREAPPLICATION_STOPPED_SETTLEMENT_FAILED_RETAINED', errorType=type(error).__name__, error=str(error)[:2048])
        save(window / 'preapplication-settlement-failed.json', result)
        raise
