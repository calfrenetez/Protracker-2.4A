"""One owner-bound passive-probe file: stage, preserve, remove and release facts.

No executable entry point or guard/lifecycle mutation. The caller must hold the
original command lock. stage_owner checks the actual original FRAME/VM/target;
stopped_owner independently checks the original lease/recovery mode and actual
VM/group/socket absence. Neither callback may reconnect or manufacture a FRAME.
All functions are single-attempt and leave partial failure evidence in place.
"""
import fcntl
import json
import os
from pathlib import Path
import re
import stat

from editor_pointer_parent import read, pinned, need, HARNESS, FACTS
from editor_pointer_probe import NATIVE, SOURCE, digest


def checked_command_lock(fd):
    path = HARNESS / 'runtime/test.lock'
    before = path.lstat()
    identity = lambda st: tuple(getattr(st, k) for k in
                               ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_nlink', 'st_size'))
    need(type(fd) is int and fd >= 3 and stat.S_ISREG(before.st_mode)
         and before.st_uid == os.getuid() and before.st_nlink == 1 and before.st_size == 0,
         'Actual existing original command-lock descriptor required')
    need(identity(os.fstat(fd)) == identity(before)
         and fcntl.fcntl(fd, fcntl.F_GETFL) & os.O_ACCMODE == os.O_RDWR,
         'Supplied descriptor is not the exact original command lock')
    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    need(identity(before) == identity(os.fstat(fd)) == identity(path.lstat()),
         'Original command lock changed')


def current_stage(api, owner, fd):
    checked_command_lock(fd); api.stage_owner(owner); checked_command_lock(fd)


def current_stopped(api, owner, mode, fd):
    checked_command_lock(fd); api.stopped_owner(owner, mode); checked_command_lock(fd)


def paths(owner):
    need(type(owner) is str and re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,95}', owner),
         'Exact safe original probe owner required')
    root = HARNESS / 'runtime/Dev/Tests'
    name = 'pt24g-editor-pointer-' + owner
    return root, root / name, 'Dev:Tests/' + name + '/PTEditorPointerV2'


def directory_ref(path):
    st = Path(path).lstat()
    need(stat.S_ISDIR(st.st_mode) and st.st_uid == os.getuid()
         and not getattr(st, 'st_flags', 0) & 1073741824, 'Ordinary own resident directory required')
    return {'path': str(path), 'facts': {k: getattr(st, k, 0) for k in FACTS}}


def same_directory(ref):
    actual = directory_ref(ref['path'])
    need(actual == ref, 'Exact probe directory changed')


def put(path, body, mode=0o600):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, mode)
    with os.fdopen(fd, 'wb') as stream:
        stream.write(body); stream.flush(); os.fsync(stream.fileno())
    actual, ref = read(path)
    need(actual == body, 'New own evidence body differs')
    return ref


def save(path, value):
    return put(path, (json.dumps(value, sort_keys=True, allow_nan=False) + '\n').encode())


def input_bytes(binding):
    for ref in binding['sourcePins'].values(): pinned(ref)
    need(bool(binding['sourcePins']), 'Frozen source pins required')
    source = pinned(binding['nativeSource']); native = pinned(binding['nativeCandidate'])
    need(digest(source) == SOURCE and digest(native) == NATIVE, 'Exact compiled V2 source and body required')
    receipt = json.loads(pinned(binding['compileReceipt']))
    need(receipt['state'] == 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED'
         and receipt['binary'] == {'bytes': NATIVE[0], 'sha256': NATIVE[1]}
         and receipt['sourcePinsEqualBeforeAfter']['tools/diagnostics/native_editor_pointer_snapshot.c']
         == {'bytes': SOURCE[0], 'sha256': SOURCE[1]}, 'Actual V2 compile association required')
    return native


def evidence_path(binding, out, leaf):
    window = Path(binding['windowDirectory'])
    need(window.is_absolute() and '..' not in window.parts
         and str(window).startswith('/private/tmp/pt24g-')
         and Path(out) == window / leaf and not os.path.lexists(out),
         'Fresh exact private probe evidence path required; never replay')
    directory_ref(window)
    return Path(out)


def stage(binding, out, api, *, lock_fd):
    """One ordinary host write to the separately owned Dev namespace; no guest RPC."""
    owner = binding['ownerRunID']
    root, directory, guest = paths(owner)
    current_stage(api, owner, lock_fd)
    native = input_bytes(binding)
    root_ref = directory_ref(root)
    out = evidence_path(binding, out, 'passive-probe-stage')
    need(not os.path.lexists(directory), 'Fresh probe namespace required; never replay')
    out.mkdir(mode=0o700)
    result = {'state': 'OWNED_PROBE_STAGE_STARTED', 'ownerRunID': owner,
              'guestCandidate': guest, 'nativeCandidate': binding['nativeCandidate'],
              'windowDirectory': binding['windowDirectory'],
              'fileWriteAttempts': 0, 'retryAllowed': False, 'lifecyclePending': True}
    try:
        save(out / 'stage-intent.json', {'ownerRunID': owner, 'directory': str(directory),
                                      'bytes': NATIVE[0], 'sha256': NATIVE[1], 'retryAllowed': False})
        current_stage(api, owner, lock_fd); input_bytes(binding)
        # Root identity must be stable; adding the new child intentionally changes
        # its size/link count/times. It is never removed or restored by this helper.
        current = directory_ref(root)
        for key in ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_flags'):
            need(current['facts'][key] == root_ref['facts'][key], 'Shared Tests directory identity changed')
        directory.mkdir(mode=0o700)
        result['directoryBeforeFile'] = directory_ref(directory)
        current_stage(api, owner, lock_fd)
        result['fileWriteAttempts'] = 1
        file_ref = put(directory / 'PTEditorPointerV2', native, 0o700)
        current_stage(api, owner, lock_fd); input_bytes(binding)
        need(pinned(file_ref) == native and sorted(p.name for p in directory.iterdir()) == ['PTEditorPointerV2'],
             'Exact sole staged probe file required')
        result.update(state='OWNED_PASSIVE_PROBE_STAGED', stagedCandidate=file_ref,
                      directory=directory_ref(directory))
        return save(out / 'stage-result.json', result)
    except BaseException as error:
        result.update(state='OWNED_PROBE_STAGE_FAILED_RETAINED', errorType=type(error).__name__,
                      error=str(error)[:2048])
        save(out / 'stage-failed.json', result)
        raise


def validate_stage(binding, stage_ref):
    """Same exact file/directory before launch and before preservation."""
    value = json.loads(pinned(stage_ref))
    _, directory, guest = paths(binding['ownerRunID'])
    need(value['state'] == 'OWNED_PASSIVE_PROBE_STAGED'
         and value['ownerRunID'] == binding['ownerRunID'] and value['guestCandidate'] == guest
         and value['nativeCandidate'] == binding['nativeCandidate'] and value['fileWriteAttempts'] == 1
         and value['retryAllowed'] is False and value['lifecyclePending'] is True
         and value['windowDirectory'] == binding['windowDirectory']
         and stage_ref['path'] == str(Path(binding['windowDirectory']) / 'passive-probe-stage/stage-result.json')
         and value['stagedCandidate']['path'] == str(directory / 'PTEditorPointerV2')
         and value['directory']['path'] == str(directory), 'Actual own stage association required')
    same_directory(value['directory'])
    need(sorted(p.name for p in directory.iterdir()) == ['PTEditorPointerV2'],
         'Unexpected probe namespace content; preserve without removal')
    need(pinned(value['stagedCandidate']) == input_bytes(binding), 'Staged probe differs from compiled bytes')
    return value


def settle_stopped(binding, stage_ref, out, api, *, mode, lock_fd):
    """Normal or failed app case, after independently verified own stop/absence.

    mode selects the caller's original guard/recovery audit. This never releases
    a HOLD, calls a target, stops a process or retries a previous settlement.
    """
    need(mode in ('normal', 'failed'), 'Explicit original settlement mode required')
    owner = binding['ownerRunID']
    current_stopped(api, owner, mode, lock_fd)
    staged = validate_stage(binding, stage_ref)
    root, directory, _ = paths(owner)
    root_ref = directory_ref(root)
    out = evidence_path(binding, out, 'passive-probe-settlement')
    out.mkdir(mode=0o700)
    result = {'state': 'OWNED_PROBE_SETTLEMENT_STARTED', 'ownerRunID': owner,
              'mode': mode, 'stageRef': stage_ref, 'filesRemoved': 0, 'directoriesRemoved': 0,
              'retryAllowed': False, 'guardReleaseClaimed': False, 'nativeCleanExitClaimed': False}
    try:
        save(out / 'preservation-intent.json', {'stageRef': stage_ref, 'mode': mode,
                                               'retryAllowed': False, 'removeBeforeCopy': False})
        current_stopped(api, owner, mode, lock_fd)
        body = pinned(staged['stagedCandidate'])
        saved_file = put(out / 'PTEditorPointerV2', body)
        saved_stage = put(out / 'stage-receipt.json', pinned(stage_ref))
        need(pinned(saved_file) == body and pinned(saved_stage) == pinned(stage_ref), 'Preserved evidence differs')
        result['preservedProbe'] = saved_file; result['preservedStage'] = saved_stage
        save(out / 'preserved-before-removal.json', result)
        current_stopped(api, owner, mode, lock_fd)
        validate_stage(binding, stage_ref)
        pinned(saved_file); pinned(saved_stage)
        root_now = directory_ref(root)
        for key in ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_flags'):
            need(root_now['facts'][key] == root_ref['facts'][key], 'Shared Tests directory identity changed')
        # The cooperating command lock and verified VM absence exclude guest and
        # other cooperating writers. Recheck exact endpoints immediately before unlink.
        pinned(staged['stagedCandidate']); same_directory(staged['directory'])
        (directory / 'PTEditorPointerV2').unlink(); result['filesRemoved'] = 1
        current_stopped(api, owner, mode, lock_fd)
        now = directory_ref(directory)
        for key in ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_flags'):
            need(now['facts'][key] == staged['directory']['facts'][key], 'Own probe directory identity changed')
        need(not list(directory.iterdir()), 'Own namespace gained content; no directory removal')
        directory.rmdir(); result['directoriesRemoved'] = 1
        current_stopped(api, owner, mode, lock_fd)
        need(not os.path.lexists(directory), 'Probe namespace reappeared; retain evidence')
        result.update(state='OWNED_PASSIVE_PROBE_PRESERVED_AND_REMOVED', absenceObserved=True)
        return save(out / 'settlement-result.json', result)
    except BaseException as error:
        result.update(state='OWNED_PROBE_SETTLEMENT_FAILED_RETAINED', errorType=type(error).__name__,
                      error=str(error)[:2048])
        save(out / 'settlement-failed.json', result)
        raise


def release_cleanup_facts(binding, stage_ref, settlement_ref, api, *, mode, lock_fd):
    """Freshly check the probe portion of the caller's normal OR failed release.

    Returns data for the existing cleanup fact record. Never constructs a guard
    proof or calls release. Whole app namespace/restoration/absence remain the
    caller's separate obligations; missing or partial probe cleanup refuses.
    """
    need(mode in ('normal', 'failed'), 'Explicit original settlement mode required')
    owner = binding['ownerRunID']; current_stopped(api, owner, mode, lock_fd)
    value = json.loads(pinned(settlement_ref)); staged = json.loads(pinned(stage_ref))
    _, directory, guest = paths(owner)
    need(value['state'] == 'OWNED_PASSIVE_PROBE_PRESERVED_AND_REMOVED'
         and value['ownerRunID'] == owner and value['mode'] == mode and value['stageRef'] == stage_ref
         and value['filesRemoved'] == 1 and value['directoriesRemoved'] == 1
         and value['absenceObserved'] is True and value['guardReleaseClaimed'] is False
         and value['nativeCleanExitClaimed'] is False and value['retryAllowed'] is False
         and staged['state'] == 'OWNED_PASSIVE_PROBE_STAGED'
         and staged['ownerRunID'] == owner and staged['guestCandidate'] == guest
         and staged['nativeCandidate'] == binding['nativeCandidate']
         and staged['windowDirectory'] == binding['windowDirectory']
         and stage_ref['path'] == str(Path(binding['windowDirectory']) / 'passive-probe-stage/stage-result.json')
         and settlement_ref['path'] == str(Path(binding['windowDirectory']) / 'passive-probe-settlement/settlement-result.json'),
         'Actual complete original-mode probe settlement required')
    need(digest(pinned(value['preservedProbe'])) == NATIVE
         and pinned(value['preservedProbe']) == input_bytes(binding)
         and pinned(value['preservedStage']) == pinned(stage_ref), 'Preserved own probe evidence changed')
    current_stopped(api, owner, mode, lock_fd)
    need(not os.path.lexists(directory), 'Probe namespace must remain absent before release')
    return {'probeStageRef': stage_ref, 'probeSettlementRef': settlement_ref,
            'probePreservedAndAbsent': True, 'mode': mode, 'guardReleaseClaimed': False,
            'wholeApplicationOrRestorationAccepted': False}
