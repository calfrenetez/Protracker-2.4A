"""Preserve/remove only the actual stopped editor's original capture namespace.

No target, stop, restoration or guard release. Original stopped owner and actual
command-lock FD are checked throughout. Partial failures are never replayed.
"""
import json
import os
from pathlib import Path
import re
import stat

from editor_pointer_parent import HARNESS, read, pinned, need
from editor_blank_capture_protocol import recent_line
from editor_probe_namespace import save, put, directory_ref, checked_command_lock


def endpoints(parent):
    blank = parent['blankBinding']; out = Path(blank['outputDirectory']); window = out.parent
    recent_line(out.name)
    need(parent['runLabel'] == out.name and parent['passiveProbe']['windowDirectory'] == str(window),
         'Same exact stopped capture namespace required')
    root = HARNESS / 'runtime/Dev/Tests'
    return window, root, root / out.name, root / ('launch-' + out.name)


def current(parent, api, mode, fd):
    checked_command_lock(fd); api.stopped_owner(parent['ownerRunID'], mode); checked_command_lock(fd)


def preserve_stopped(parent, api, *, mode, lock_fd):
    need(mode in ('normal', 'failed'), 'Explicit original stopped mode required')
    window, root, run, launch = endpoints(parent)
    current(parent, api, mode, lock_fd); root_ref = directory_ref(root); directory_ref(run)
    def same_root():
        now = directory_ref(root)
        for key in ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_flags'):
            need(now['facts'][key] == root_ref['facts'][key], 'Shared Tests directory identity changed')
    saved = window / 'preserved-guest-namespace'
    need(not os.path.lexists(saved) and not os.path.lexists(window / 'namespace-cleanup-result.json'),
         'Never replay app namespace preservation or removal')
    saved.mkdir(mode=0o700)
    result = {'state': 'OWNED_CAPTURE_NAMESPACE_PRESERVATION_STARTED', 'ownerRunID': parent['ownerRunID'],
              'mode': mode, 'files': [], 'directories': [], 'filesRemoved': 0, 'directoriesRemoved': 0,
              'retryAllowed': False, 'nativeCleanExitClaimed': False, 'ENVRestorationClaimed': False}
    try:
        total = 0
        for base, dirs, leaves in os.walk(run, followlinks=False):
            current(parent, api, mode, lock_fd); same_root()
            base = Path(base); ref = directory_ref(base)
            need(len(base.relative_to(run).parts) <= 4 and len(result['directories']) < 64,
                 'Capture namespace nesting/count exceeds original finite scope')
            result['directories'].append(ref)
            for name in dirs + leaves:
                need(re.fullmatch(r'[A-Za-z0-9._-]{1,100}', name) and not (base / name).is_symlink(),
                     'Unexpected own namespace name/type; retain without removal')
            for name in leaves:
                source = base / name; body, pin = read(source)
                total += len(body)
                need(len(result['files']) < 128 and total <= 16777216, 'Finite own namespace size exceeded')
                dest = saved / 'run' / source.relative_to(run)
                dest.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
                copy_ref = put(dest, body)
                need(pinned(copy_ref) == pinned(pin), 'Preserved own app body differs')
                result['files'].append({'source': pin, 'copy': copy_ref})
        body, pin = read(launch); copy_ref = put(saved / 'launcher', body)
        need(pinned(copy_ref) == pinned(pin), 'Preserved own launcher differs')
        result['files'].append({'source': pin, 'copy': copy_ref})
        mounted = parent['blankBinding']['mountedCandidate']; volume = Path(mounted['path']).parent
        directory_ref(volume)
        need(Path(mounted['path']).name == 'PT24GEdit'
             and sorted(p.name for p in volume.iterdir()) == ['PT24GEdit'],
             'Immutable input volume gained content; retain and inspect')
        result['retainedVolume'] = {'source': mounted, 'copy': put(saved / 'PT24GEdit', pinned(mounted))}
        save(window / 'namespace-cleanup-intent.json', result)
        # Every byte is preserved before the first unlink. Check exact source
        # endpoints and original absence immediately before each mutation.
        for row in result['files']:
            current(parent, api, mode, lock_fd); same_root(); pinned(row['source']); pinned(row['copy'])
            Path(row['source']['path']).unlink(); result['filesRemoved'] += 1
        for ref in reversed(result['directories']):
            current(parent, api, mode, lock_fd); same_root(); actual = directory_ref(ref['path'])
            for key in ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_flags'):
                need(actual['facts'][key] == ref['facts'][key], 'Own app directory identity changed')
            need(not list(Path(ref['path']).iterdir()), 'Own app directory gained content')
            Path(ref['path']).rmdir(); result['directoriesRemoved'] += 1
        current(parent, api, mode, lock_fd)
        need(not os.path.lexists(run) and not os.path.lexists(launch), 'Own app namespace reappeared')
        result.update(state='OWNED_CAPTURE_NAMESPACE_PRESERVED_AND_ABSENT', absenceVerified=True)
        return save(window / 'namespace-cleanup-result.json', result)
    except BaseException as error:
        result.update(state='OWNED_CAPTURE_NAMESPACE_FAILED_RETAINED', errorType=type(error).__name__, error=str(error)[:2048])
        save(window / 'namespace-cleanup-failed.json', result)
        raise


def release_facts(parent, result_ref, api, *, mode, lock_fd):
    window, _, run, launch = endpoints(parent)
    current(parent, api, mode, lock_fd); value = json.loads(pinned(result_ref))
    need(result_ref['path'] == str(window / 'namespace-cleanup-result.json')
         and value['state'] == 'OWNED_CAPTURE_NAMESPACE_PRESERVED_AND_ABSENT'
         and value['ownerRunID'] == parent['ownerRunID'] and value['mode'] == mode
         and value['absenceVerified'] is True and value['retryAllowed'] is False
         and value['nativeCleanExitClaimed'] is False and value['ENVRestorationClaimed'] is False
         and value['filesRemoved'] == len(value['files'])
         and value['directoriesRemoved'] == len(value['directories']), 'Actual complete own app settlement required')
    for row in value['files']:
        body = pinned(row['copy']); source = Path(row['source']['path'])
        need(source == launch or run in source.parents, 'Unexpected removed source path')
        need(len(body) == row['source']['bytes'] and read(row['copy']['path'])[1]['sha256'] == row['source']['sha256']
             and not os.path.lexists(source), 'Preserved app body changed or source reappeared')
    volume = value['retainedVolume']
    need(volume['source'] == parent['blankBinding']['mountedCandidate']
         and pinned(volume['source']) == pinned(volume['copy']), 'Retained immutable candidate changed')
    current(parent, api, mode, lock_fd)
    need(not os.path.lexists(run) and not os.path.lexists(launch), 'App namespace must remain absent before release')
    return {'appNamespaceCleanupRef': result_ref, 'appNamespacePreservedAndAbsent': True,
            'nativeCleanExitClaimed': False, 'ENVRestorationClaimed': False}
