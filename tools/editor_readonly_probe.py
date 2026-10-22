"""Distinct read-only V2 carrier and stopped input-retention facts.

No reservation, FRAME creation, service change, staging, signal or release.
The existing executable parent does not admit this new carrier yet. Runtime
use requires the caller's original current owner/FRAME and a distinct freeze.
"""
import json
from pathlib import Path

import editor_pointer_parent as parent
import editor_pointer_probe as original_probe
import editor_probe_volume as volume
from editor_probe_namespace import checked_command_lock, directory_ref, put, save

FROZEN_STATE = 'ROOT_FROZEN_EDITOR_READONLY_PASSIVE_V2_SESSION_SOURCE_NOT_EXECUTED'
INPUT_STATE = 'ROOT_ADMITTED_PASSIVE_V2_READONLY_PROBE_INPUT_ONLY'
MODULES = parent.MODULES + ('editor_probe_volume', 'editor_readonly_probe')
ALLOWED = {'guestReadbacks': 1, 'passiveNativeExecutions': 1, 'input': 0,
           'hostInputWrites': 0, 'serviceChanges': 0, 'guardReleases': 0}
LIMITS = {'whole': original_probe.PROBE_SECONDS, 'script': original_probe.SCRIPT_SECONDS}


def source_shape(record, *, pin=parent.pinned):
    """Canonical complete selected source mapping only; grants no authority."""
    modules, sources = record['modulePaths'], record['sources']
    parent.need(set(modules) == set(MODULES) and type(sources) is dict and sources
                and all(k == v['path'] for k, v in sources.items()), 'Exact read-only source mapping required')
    session = Path(modules['capture_runner']).parent; window = Path(record['newWindow'])
    for path in (session, window):
        parent.need(path.parent == Path('/private/tmp') and path.name.startswith('pt24g-')
                    and '..' not in path.parts, 'Separate direct-private source/window required')
    parent.need(session != window, 'Source session cannot be the runtime window')
    directory_ref(session)
    local = {'capture_runner', 'capture_case_mounted', 'first_mouse_api', 'capture_support', 'root_observation'}
    shared = {'launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'}
    for name, path in modules.items():
        if name in local: expected = session / (name + '.py')
        elif name in shared: expected = parent.HARNESS / 'scripts' / (name + '.py')
        elif name.startswith('amiberry_mcp.'):
            expected = parent.HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name.split('.')[-1] + '.py')
        else: expected = parent.TOOLS / (name + '.py')
        parent.need(path == str(expected) and path in sources, 'Canonical selected read-only module path differs')
    extras = {n: str(session / (n + '.py')) for n in
              ('fresh_preservation', 'canonical_profile_session_files', 'service_readiness')}
    parent.need(record['extraModulePaths'] == extras
                and set(extras.values()) <= set(sources)
                and str(parent.TOOLS / 'editor_pointer_parent.py') in sources,
                'Original private helpers and actual parent must be selected')
    for ref in sources.values():
        body = pin(ref)
        if Path(ref['path']).suffix == '.py': compile(body, ref['path'], 'exec')
    return session, window


def prepared_input(binding, *, pin=parent.pinned):
    """Exact retained input evidence; does not assert a current mounted profile.

    This separate check is also usable after genuine stopped profile restoration.
    Runtime uses require_current_mounted_input, including actual owner and profile.
    """
    b = binding
    parent.need(b['state'] == INPUT_STATE and b['acceptedByRoot'] is True
                and b['target'] == 'amiberry-030' and b['allowedActions'] == ALLOWED
                and b['limits'] == LIMITS and 'namespaceStageRef' not in b,
                'Distinct read-only passive-input admission required')
    freeze = json.loads(pin(b['sourceFreezeRef']))
    parent.need(freeze['state'] == FROZEN_STATE and freeze['sources'] == b['sourcePins']
                and freeze['readonlyPreparationRef'] == b['readonlyPreparationRef'],
                'Actual distinct read-only source freeze required')
    _, window = source_shape(freeze, pin=pin); directory_ref(window)
    parent.need(b['windowDirectory'] == str(window)
                and b['sourceFreezeRef']['path'] == str(Path(freeze['modulePaths']['capture_runner']).parent / 'SOURCE-FREEZE.json'),
                'Exact read-only source/window association required')
    owner = b['ownerRunID']
    parent.need(type(owner) is str and 1 <= len(owner) <= 96 and owner[0].isascii() and owner[0].isalnum()
                and all(c.isascii() and (c.isalnum() or c in '_-') for c in owner), 'Safe original read-only owner required')
    parent.need(b['guestCandidate'] == 'PTProbe:PTEditorPointerV2', 'Exact read-only guest input required')
    parent.recent_line(b['runLabel'])
    prepared = volume.verify(b['readonlyPreparationRef'])
    refs = prepared['inputs']; capture = json.loads(pin(refs['capturePreparation']))
    parent.need(b['nativeSource'] == refs['nativeSource'] and b['nativeCandidate'] == refs['nativeCandidate']
                and b['compileReceipt'] == refs['compileReceipt']
                and b['stagedCandidate'] == prepared['candidate']
                and b['editorCandidate'] == capture['privateCandidate']
                and b['capturePreparationRef'] == refs['capturePreparation']
                and pin(b['originalProfileBeforeimage']) == pin(refs['originalProfile']),
                'Exact prepared source/native/editor/profile associations required')
    parent.need(str(Path(__file__).resolve()) == freeze['modulePaths']['editor_readonly_probe']
                and str(Path(volume.__file__).resolve()) == freeze['modulePaths']['editor_probe_volume'],
                'Actual loaded read-only carrier/preparation sources differ')
    return prepared


def require_current_mounted_input(binding, api):
    """Original current owner plus all strict pins before/after runtime I/O."""
    api.owner(binding['ownerRunID'])
    prepared = prepared_input(binding, pin=api.pinned)
    before = api.pinned(binding['originalProfileBeforeimage'])
    active = api.pinned(binding['activeProfile'])
    parent.need(binding['activeProfile']['path']
                == str(parent.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae')
                and active == before + prepared['appendASCII'].encode('ascii')
                and prepared['proposedProfile'] == {'bytes': len(active), 'sha256': parent.digest(active)[1]},
                'Actual exact complete paired read-only profile required')
    api.owner(binding['ownerRunID'])
    return prepared


class ReadonlyEditorPointerProbe(original_probe.EditorPointerProbe):
    """Same single readback/execution, parser and budgets in the original FRAME."""
    def validate_endpoint(self):
        b = self.binding
        parent.need(b['state'] == INPUT_STATE and b['acceptedByRoot'] is True
                    and b['guestCandidate'] == 'PTProbe:PTEditorPointerV2'
                    and 'namespaceStageRef' not in b, 'Distinct read-only probe endpoint required')

    def validate_input_namespace(self):
        require_current_mounted_input(self.binding, self.original)

    def validate_output(self, out):
        super().validate_output(out)
        b = self.binding; parent.recent_line(b['runLabel'])
        parent.need(out == Path(b['windowDirectory']) / b['runLabel'] / 'mouse-phase/passive-pointer',
                    'Exact same-window read-only probe evidence endpoint required')
        directory_ref(out.parent); directory_ref(out.parent.parent)


def current_stopped(binding, api, mode, fd):
    checked_command_lock(fd)
    api.stopped_owner(binding['ownerRunID'], mode)
    checked_command_lock(fd)


def preserve_stopped(binding, out, api, *, mode, lock_fd):
    """Copy input evidence after original stopped-owner proof; retain the volume."""
    parent.need(mode in ('normal', 'failed'), 'Explicit original stopped mode required')
    current_stopped(binding, api, mode, lock_fd)
    prepared = prepared_input(binding); window = Path(binding['windowDirectory']); out = Path(out)
    parent.need(out == window / 'preserved-readonly-probe-input' and not out.exists() and not out.is_symlink(),
                'New exact stopped input evidence only; never replay')
    out.mkdir(mode=0o700)
    result = {'state': 'READONLY_INPUT_PRESERVATION_STARTED', 'ownerRunID': binding['ownerRunID'],
              'mode': mode, 'preparationRef': binding['readonlyPreparationRef'],
              'retainedInput': prepared['candidate'], 'retainedDirectory': prepared['directory'],
              'probeFilesRemoved': 0, 'probeDirectoriesRemoved': 0,
              'targetOperations': 0, 'runtimeAdmission': False, 'retryAllowed': False}
    try:
        save(out / 'intent.json', result)
        result['preservedProbe'] = put(out / 'PTEditorPointerV2', parent.pinned(prepared['candidate']))
        result['preservedPreparation'] = put(out / 'PREPARATION.json', parent.pinned(binding['readonlyPreparationRef']))
        current_stopped(binding, api, mode, lock_fd); prepared_input(binding)
        parent.need(parent.pinned(result['preservedProbe']) == parent.pinned(prepared['candidate'])
                    and parent.pinned(result['preservedPreparation']) == parent.pinned(binding['readonlyPreparationRef']),
                    'Preserved read-only input differs')
        result['state'] = 'READONLY_PROBE_INPUT_PRESERVED_AND_RETAINED'
        return save(out / 'result.json', result)
    except BaseException as error:
        result.update(state='READONLY_INPUT_PRESERVATION_FAILED_RETAINED', errorType=type(error).__name__, error=str(error)[:2048])
        save(out / 'failed.json', result); raise


def release_facts(binding, result_ref, api, *, mode, lock_fd):
    """Input-retention supplement only; no runtime admission or guard release."""
    parent.need(mode in ('normal', 'failed'), 'Explicit original stopped mode required')
    current_stopped(binding, api, mode, lock_fd)
    prepared = prepared_input(binding); value = json.loads(parent.pinned(result_ref))
    saved = Path(binding['windowDirectory']) / 'preserved-readonly-probe-input'
    parent.need(result_ref['path'] == str(saved / 'result.json')
                and value['state'] == 'READONLY_PROBE_INPUT_PRESERVED_AND_RETAINED'
                and value['ownerRunID'] == binding['ownerRunID'] and value['mode'] == mode
                and value['preparationRef'] == binding['readonlyPreparationRef']
                and value['retainedInput'] == prepared['candidate'] and value['retainedDirectory'] == prepared['directory']
                and value['preservedProbe']['path'] == str(saved / 'PTEditorPointerV2')
                and value['preservedPreparation']['path'] == str(saved / 'PREPARATION.json')
                and value['probeFilesRemoved'] == value['probeDirectoriesRemoved'] == value['targetOperations'] == 0
                and value['runtimeAdmission'] is value['retryAllowed'] is False,
                'Complete same-owner stopped input-retention evidence required')
    parent.need(parent.pinned(value['preservedProbe']) == parent.pinned(prepared['candidate'])
                and parent.pinned(value['preservedPreparation']) == parent.pinned(binding['readonlyPreparationRef']),
                'Retained input evidence changed')
    current_stopped(binding, api, mode, lock_fd)
    return {'readonlyProbeInput': {'resultRef': result_ref, 'facts': value,
                                 'runtimeAdmission': False, 'guardReleaseClaimed': False}}
