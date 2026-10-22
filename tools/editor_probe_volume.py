"""Host-only preparation of a separate read-only passive-probe input volume.

No target imports, shared writes, source freeze, admission, lease or lifecycle.
The existing parent does not consume this prospective preparation yet. A fresh
reviewed integration must bind both paired mounts and retain strict full facts.
"""
import json
import os
from pathlib import Path
import re
import stat

from editor_capture_volume import profile_append as capture_append
from editor_pointer_parent import read, pinned, need, FACTS
from editor_pointer_probe import NATIVE, SOURCE, EDITOR, digest

STATE = 'PRIVATE_READONLY_PROBE_VOLUME_PREPARED_NOT_MOUNTED_OR_ADMITTED'
LABEL = r'[a-z0-9][a-z0-9-]{0,47}'
VOLUME = re.compile(r'/private/tmp/pt24g-editor-probe-volume-(' + LABEL + r')')
PREPARATION = re.compile(r'/private/tmp/pt24g-editor-probe-preparation-(' + LABEL + r')')


def profile_append(original, capture_directory, probe_directory):
    """Retain the exact existing editor suffix; add paired DH4/uaehf3 ro lines."""
    editor = capture_append(original, capture_directory)
    if (not isinstance(probe_directory, str) or VOLUME.fullmatch(probe_directory) is None
            or any(slot in original for slot in (b'PTProbe', b'DH4:', b'uaehf3='))):
        raise ValueError('Fresh bounded probe volume and unoccupied paired slots required')
    mount = ('DH4:PTProbe:' + probe_directory + ',-128\n').encode('ascii')
    return editor + b'filesystem2=ro,' + mount + b'uaehf3=dir,ro,' + mount


def directory_ref(path):
    path = Path(path); st = path.lstat()
    need(stat.S_ISDIR(st.st_mode) and st.st_uid == os.getuid()
         and not getattr(st, 'st_flags', 0) & 1073741824, 'Own ordinary resident volume required')
    return {'path': str(path), 'fullFacts': {k: getattr(st, k, 0) for k in FACTS}}


def put(path, body, mode=0o600):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, mode)
    with os.fdopen(fd, 'wb') as stream:
        stream.write(body); stream.flush(); os.fsync(stream.fileno())
    actual, ref = read(path)
    need(actual == body, 'Own prepared input copy differs')
    return ref


def save(path, value):
    return put(path, (json.dumps(value, sort_keys=True, allow_nan=False) + '\n').encode())


def inputs(refs, probe_directory):
    need(set(refs) == {'originalProfile', 'capturePreparation', 'nativeSource',
                       'nativeCandidate', 'compileReceipt'}, 'Exact prospective input references required')
    original = pinned(refs['originalProfile'])
    capture = json.loads(pinned(refs['capturePreparation']))
    need(capture['state'] == 'PRIVATE_LOCAL_CANDIDATE_VOLUME_AND_ADDITIVE_PROFILE_SUFFIX_PREPARED_NOT_ADMITTED_OR_MOUNTED'
         and capture['targetOperations'] == capture['canonicalProfileWrites'] == 0
         and capture['guestCandidate'] == 'PTCapture:PT24GEdit', 'Original host-only editor preparation required')
    candidate = capture['privateCandidate']; path = Path(candidate['path'])
    directory_ref(path.parent)
    need(path.name == 'PT24GEdit' and sorted(p.name for p in path.parent.iterdir()) == ['PT24GEdit']
         and pinned(candidate) == pinned(capture['originalCandidate'])
         and digest(pinned(candidate)) == EDITOR, 'Exact sole-file editor input volume required')
    editor_suffix = capture_append(original, str(path.parent))
    need(capture['appendASCII'] == editor_suffix.decode('ascii')
         and capture['proposedProfile'] == {'bytes': len(original + editor_suffix),
             'sha256': digest(original + editor_suffix)[1]}, 'Editor profile preparation differs')
    native = pinned(refs['nativeCandidate']); source = pinned(refs['nativeSource'])
    receipt = json.loads(pinned(refs['compileReceipt']))
    need(digest(native) == NATIVE and digest(source) == SOURCE
         and receipt['state'] == 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED'
         and receipt['binary'] == {'bytes': NATIVE[0], 'sha256': NATIVE[1]}
         and receipt['sourcePinsEqualBeforeAfter']['tools/diagnostics/native_editor_pointer_snapshot.c']
             == {'bytes': SOURCE[0], 'sha256': SOURCE[1]}, 'Exact original V2 compile association required')
    suffix = profile_append(original, str(path.parent), probe_directory)
    return original, native, suffix


def prepare(refs, volume, evidence):
    """Create one private input copy and evidence; retain partial failures, no retry.

    No original file is rewritten, including the existing sole-file PTCapture.
    Host read/execute permissions plus ro mount syntax are prospective only;
    neither is evidence of a guest mount or runtime immutability.
    """
    volume, evidence = Path(volume), Path(evidence)
    vm = VOLUME.fullmatch(str(volume)); em = PREPARATION.fullmatch(str(evidence))
    need(vm is not None and em is not None and vm.group(1) == em.group(1),
         'Matching new direct-private volume and evidence paths required')
    need(not os.path.lexists(volume) and not os.path.lexists(evidence), 'Never replay private preparation')
    original, native, suffix = inputs(refs, str(volume))
    source_ref = read(Path(__file__).resolve())[1]
    evidence.mkdir(mode=0o700)
    result = {'state': 'PRIVATE_READONLY_PROBE_PREPARATION_STARTED', 'inputs': refs,
              'preparationSource': source_ref, 'volumePath': str(volume),
              'guestCandidate': 'PTProbe:PTEditorPointerV2', 'targetOperations': 0,
              'canonicalProfileWrites': 0, 'runtimeAdmission': False,
              'sourceFreezeCreated': False, 'retryAllowed': False, 'candidateCopies': 0}
    try:
        save(evidence / 'intent.json', result)
        inputs(refs, str(volume)); pinned(source_ref)
        volume.mkdir(mode=0o700)
        result['candidateCopies'] = 1
        candidate = put(volume / 'PTEditorPointerV2', native, 0o500)
        volume.chmod(0o500)
        need(pinned(candidate) == native and stat.S_IMODE(candidate['fullFacts']['st_mode']) == 0o500
             and sorted(p.name for p in volume.iterdir()) == ['PTEditorPointerV2'],
             'Exact sole-file prospective probe volume required')
        inputs(refs, str(volume)); pinned(source_ref)
        result.update(state=STATE, candidate=candidate, directory=directory_ref(volume),
                      appendASCII=suffix.decode('ascii'), proposedProfile={
                          'bytes': len(original + suffix), 'sha256': digest(original + suffix)[1]})
        return save(evidence / 'PREPARATION.json', result)
    except BaseException as error:
        result.update(state='PRIVATE_READONLY_PROBE_PREPARATION_FAILED_RETAINED',
                      errorType=type(error).__name__, error=str(error)[:2048])
        save(evidence / 'FAILED.json', result)
        raise


def verify(ref):
    """Strict prospective input check only; never a runtime or ownership check."""
    value = json.loads(pinned(ref)); volume = Path(value['volumePath'])
    vm = VOLUME.fullmatch(str(volume)); em = PREPARATION.fullmatch(str(Path(ref['path']).parent))
    need(vm is not None and em is not None and vm.group(1) == em.group(1)
         and Path(ref['path']).name == 'PREPARATION.json' and value['state'] == STATE
         and value['targetOperations'] == value['canonicalProfileWrites'] == 0
         and value['runtimeAdmission'] is value['sourceFreezeCreated'] is value['retryAllowed'] is False
         and value['candidateCopies'] == 1 and value['guestCandidate'] == 'PTProbe:PTEditorPointerV2'
         and value['candidate']['path'] == str(volume / 'PTEditorPointerV2'),
         'Exact completed prospective preparation required')
    need(value['preparationSource']['path'] == str(Path(__file__).resolve()),
         'Actual prospective preparation source required')
    pinned(value['preparationSource'])
    original, native, suffix = inputs(value['inputs'], str(volume))
    need(value['appendASCII'] == suffix.decode('ascii') and value['proposedProfile'] == {
             'bytes': len(original + suffix), 'sha256': digest(original + suffix)[1]}
         and directory_ref(volume) == value['directory'] and pinned(value['candidate']) == native
         and stat.S_IMODE(value['directory']['fullFacts']['st_mode']) == 0o500
         and stat.S_IMODE(value['candidate']['fullFacts']['st_mode']) == 0o500
         and sorted(p.name for p in volume.iterdir()) == ['PTEditorPointerV2'],
         'Prospective volume/profile body, identity or metadata changed')
    return value
