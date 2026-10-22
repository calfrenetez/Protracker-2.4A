"""Full normal/failed restoration and original public release AFTER verified stop.

No target command, signal, reconnect, service change or transaction retry.
Every mutation uses the original current owner and bounded preserved evidence.
"""
import hashlib
import json
import os
from pathlib import Path
import sys
import time

HARNESS = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
if __name__ == '__main__':
    info = time.get_clock_info('monotonic')
    if not (sys.executable == str(HARNESS / '.venv/bin/python')
            and tuple(sys.version_info[:3]) == (3, 14, 0) and sys.platform == 'darwin'
            and sys.implementation.name == 'cpython' and info.monotonic and not info.adjustable):
        raise RuntimeError('Original harness clock required before settlement imports')

import editor_pointer_parent as parent
import editor_pointer_binding as binding_source
import editor_probe_stopped as stopped
import editor_capture_namespace as app_namespace
from editor_probe_namespace import save, directory_ref

STATE = 'ROOT_ACCEPTED_FULL_STOPPED_PASSIVE_V2_RESTORATION_AND_RELEASE'
READONLY_STATE = 'ROOT_ACCEPTED_FULL_STOPPED_READONLY_PASSIVE_V2_RESTORATION_AND_RELEASE'
APP_ROOT = Path('/Users/james1/Documents/Amiberry')
ORIGINAL_HELPER = (10249, '8c1fbc9f12b31fd846e37916d2178ab153f34477608c77378438ee43d9e8f15b')
THEMES = {'Visuals/Themes/Dark.theme', 'Visuals/Themes/Default.theme'}
ALLOWED = {'probeSettlements': 1, 'appNamespaceSettlements': 1, 'hostRoleRestorations': 3,
           'themeMtimeRestorationsMaximum': 2, 'guardReleases': 1,
           'targetCommands': 0, 'signals': 0, 'serviceChanges': 0}


def large_pin(ref):
    body, actual = parent.read(ref['path'], 16777216)
    parent.need(actual == ref, 'Pinned bounded app-data body/metadata changed')
    return body


def validate(binding):
    b = binding
    readonly = b.get('state') == READONLY_STATE
    parent.need(b['state'] == (READONLY_STATE if readonly else STATE) and b['acceptedByRoot'] is True
                and b['allowedActions'] == ALLOWED and b['limits'] == {'whole': 180},
                'Distinct full stopped restoration/release scope required')
    sb = json.loads(parent.pinned(b['stoppedBindingRef'])); verified = stopped.validate(sb)
    p = verified[0]; window = Path(sb['windowDirectory']); directory_ref(window)
    parent.need((sb['state'] == stopped.READONLY_STATE) == readonly, 'Full settlement and stopped input scopes differ')
    parent.need(b['ownerRunID'] == sb['ownerRunID'] == p['ownerRunID']
                and b['mode'] == sb['mode'] and b['windowDirectory'] == str(window)
                and b['stoppedBindingRef']['path'] == str(window / 'probe-stopped-binding.json')
                and str(Path(__file__)) in p['sourcePins']
                and str(Path(app_namespace.__file__)) in p['sourcePins'],
                'Same exact stopped binding and selected full settlement source required')
    for key, name in {'appDataBeforeRef': 'current-app-data-before-and-backups.json',
                      'preservationBeforeRef': 'fresh-preservation-before.json',
                      'roleBaselineRef': 'before-owned-lifecycle-files.json'}.items():
        parent.need(b[key]['path'] == str(window / name), 'Actual new restoration baseline path differs')
        parent.pinned(b[key])
    return sb, verified, window


def load_dependencies(binding, verified):
    p = verified[0]; freeze = json.loads(parent.pinned(p['sourceFreezeRef']))
    readonly = binding['state'] == READONLY_STATE
    extras = binding_source.load_readonly_extras(freeze) if readonly else binding_source.load_extras(freeze)
    f = extras['canonical_profile_session_files']
    if readonly:
        binding_source.configure_readonly_probe_saved_files(f, p['sourcePins'],
            p['blankBinding']['localVolumePreparation'], p['passiveProbe']['readonlyPreparationRef'],
            p['blankBinding']['originalProfileBeforeimage'])
    else:
        binding_source.configure_saved_files(f, p['sourcePins'], p['blankBinding']['localVolumePreparation'],
                                             p['blankBinding']['originalProfileBeforeimage'])
    d = stopped.load_dependencies(p)
    d.files = f; d.preservation = extras['fresh_preservation']
    return d


def inventory():
    directory_ref(APP_ROOT); names = set()
    for base, dirs, files in os.walk(APP_ROOT, followlinks=False):
        directory_ref(base)
        for name in dirs: directory_ref(Path(base) / name)
        for name in files:
            if name == '.DS_Store': continue
            path = Path(base) / name
            parent.need(not path.is_symlink(), 'Unexpected app-data symlink; retain without restoration')
            names.add(str(path.relative_to(APP_ROOT)))
    return names


def app_data_rows(binding, p):
    before = json.loads(parent.pinned(binding['appDataBeforeRef']))
    rows = before['files']; session = Path(p['blankBinding']['modulePaths']['capture_runner']).parent
    parent.need(len(rows) == 31 and inventory() == set(rows), 'Exact complete original31 app-data inventory required')
    for name, row in rows.items():
        path = Path(name); backup = Path(row['backup'])
        parent.need(not path.is_absolute() and '..' not in path.parts
                    and row['original']['path'] == str(APP_ROOT / path)
                    and not backup.is_absolute() and '..' not in backup.parts,
                    'Original app-data/backup paths differ')
        body, _ = parent.read(session / backup, 16777216)
        parent.need(parent.digest(body) == (row['original']['bytes'], row['original']['sha256']),
                    'Saved original app-data body changed')
    return rows


def audit_app_data(rows):
    result = {'state': 'FULL_STOPPED_APP_DATA_AUDIT', 'files': {}, 'changes': {}}
    parent.need(inventory() == set(rows), 'Complete app-data inventory changed')
    for name, row in rows.items():
        body, current = parent.read(APP_ROOT / name, 16777216); original = row['original']
        parent.need(parent.digest(body) == (original['bytes'], original['sha256']),
                    'App-data body changed; independent recovery required')
        result['files'][name] = current
        if current != original:
            changed = {k for k in parent.FACTS if current['fullFacts'][k] != original['fullFacts'][k]}
            parent.need(name in THEMES and changed == {'st_mtime_ns', 'st_ctime_ns'},
                        'Unapproved app-data metadata change; independent recovery required')
            result['changes'][name] = {'before': original, 'after': current}
    return result


def verify_app_data(rows, themes):
    parent.need(inventory() == set(rows), 'App-data inventory changed before release')
    actual = {}
    for name, row in rows.items():
        body, now = parent.read(APP_ROOT / name, 16777216); original = row['original']
        parent.need(parent.digest(body) == (original['bytes'], original['sha256'])
                    and all(now['fullFacts'][k] == original['fullFacts'][k] for k in parent.FACTS
                            if k != 'st_ctime_ns'), 'Original app-data body/identity/mtime not restored')
        if name not in themes:
            parent.need(now == original, 'Unrelated app-data metadata changed')
        actual[name] = now
    return actual


def restore_theme_mtimes(rows, audit, window, f, current):
    restored = {}
    with f.locks():
        current()
        # Check every proposed endpoint before the first mtime mutation.
        for row in audit['changes'].values(): large_pin(row['after'])
        save(window / 'theme-mtime-restoration-intent.json', {
            'state': 'EXACT_OWN_RUN_THEME_MTIMES_ONLY', 'changes': audit['changes'],
            'bodyWrites': 0, 'retryAllowed': False})
        for name, row in audit['changes'].items():
            current(); body = large_pin(row['after']); path = Path(row['after']['path'])
            fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
            try:
                facts = lambda st: {k: getattr(st, k, 0) for k in parent.FACTS}
                parent.need(facts(os.fstat(fd)) == row['after']['fullFacts'] == facts(path.lstat()),
                            'Actual theme endpoint changed before mtime restoration')
                parent.need(os.read(fd, 16777217) == body, 'Theme body changed through held descriptor')
                os.utime(fd, ns=(os.fstat(fd).st_atime_ns, row['before']['fullFacts']['st_mtime_ns']))
                os.fsync(fd)
                parent.need(facts(os.fstat(fd)) == facts(path.lstat()), 'Theme endpoint replaced during restoration')
            finally: os.close(fd)
            restored[name] = parent.read(path, 16777216)[1]
        verify_app_data(rows, restored); current()
    return save(window / 'theme-mtime-restoration-result.json', {
        'state': 'ORIGINAL_THEME_BODIES_AND_MTIMES_VERIFIED', 'themes': restored,
        'bodyWrites': 0, 'historicalCtimeContinuityClaimed': False})


def execute(binding, dependencies=None):
    b = binding; sb, verified, window = validate(b); p = verified[0]
    d = dependencies if dependencies is not None else load_dependencies(b, verified)
    f = d.files; deadline = time.monotonic() + 180
    parent.need(not any(os.path.lexists(window / n) for n in
        ('full-settlement-intent.json', 'full-settlement-result.json', 'full-settlement-failed.json',
         'restore-admission.json', 'release-proof.json')), 'Never replay full stopped settlement')

    def owner():
        parent.need(time.monotonic() < deadline, 'Whole stopped settlement deadline exhausted; retain without replay')
        check = stopped.StoppedOwner(sb, verified, d)
        check.stopped_owner(b['ownerRunID'], b['mode'])
        return check

    owner(); rows = app_data_rows(b, p)
    save(window / 'full-settlement-intent.json', {'state': b['state'], 'ownerRunID': b['ownerRunID'],
         'mode': b['mode'], 'allowedActions': ALLOWED, 'retryAllowed': False})
    result = {'state': 'FULL_STOPPED_SETTLEMENT_STARTED', 'mode': b['mode'],
              'targetOperations': 0, 'nativeCleanExitClaimed': False, 'runtimeAcceptance': False,
              'retryAllowed': False, 'releaseAttempted': False}
    try:
        probe_fact_ref = stopped.execute(sb, d)
        with parent.command_lock(HARNESS / 'runtime/test.lock') as fd:
            app_ref = app_namespace.preserve_stopped(p, owner(), mode=b['mode'], lock_fd=fd)
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
            app_facts = app_namespace.release_facts(p, app_ref, check, mode=b['mode'], lock_fd=fd)
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
                'cleanup': {'mode': b['mode'], 'appNamespace': app_facts, **probe_facts,
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
            app_namespace.release_facts(p, app_ref, check, mode=b['mode'], lock_fd=fd)
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
        result.update(state='FULL_STOPPED_EDITOR_AND_PROBE_RESTORED_AND_RELEASED', released=released,
                      archivedHolds=len(archive['holds']), restoredRoles=3, appDataFiles=31)
        save(window / 'full-settlement-result.json', result)
        return result
    except BaseException as error:
        result.update(state='FULL_STOPPED_SETTLEMENT_FAILED_RETAINED', errorType=type(error).__name__, error=str(error)[:2048])
        save(window / 'full-settlement-failed.json', result)
        raise


def main():
    os.umask(0o077); sys.dont_write_bytecode = True
    parent.need(len(sys.argv) == 3 and sys.argv[1] == '--binding-ref', 'Actual complete stopped settlement binding required')
    ref = json.loads(sys.argv[2]); b = json.loads(parent.pinned(ref))
    parent.need(ref['path'] == str(Path(b['windowDirectory']) / 'full-settlement-binding.json'),
                'Exact actual new full settlement binding path required')
    print(json.dumps(execute(b)))


if __name__ == '__main__': main()
