"""Stopped V3 input preservation only; no target, deletion or guard release."""
import json
from pathlib import Path
import editor_pointer_parent as parent
from editor_context_probe import prepared_input
from editor_probe_namespace import checked_command_lock, put, save

def current_stopped(binding, api, mode, fd):
    checked_command_lock(fd)
    api.stopped_owner(binding['ownerRunID'], mode)
    checked_command_lock(fd)


def preserve_stopped(binding, out, api, *, mode, lock_fd):
    """Copy input evidence after original stopped-owner proof; retain the volume."""
    parent.need(mode in ('normal', 'failed'), 'Explicit original stopped mode required')
    current_stopped(binding, api, mode, lock_fd)
    prepared = prepared_input(binding); window = Path(binding['windowDirectory']); out = Path(out)
    parent.need(out == window / 'preserved-readonly-context-input' and not out.exists() and not out.is_symlink(),
                'New exact stopped input evidence only; never replay')
    out.mkdir(mode=0o700)
    result = {'state': 'CONTEXT_V3_INPUT_PRESERVATION_STARTED', 'ownerRunID': binding['ownerRunID'],
              'mode': mode, 'preparationRef': binding['contextPreparationRef'],
              'retainedInput': prepared['candidate'], 'retainedDirectory': prepared['directory'],
              'probeFilesRemoved': 0, 'probeDirectoriesRemoved': 0,
              'targetOperations': 0, 'runtimeAdmission': False, 'retryAllowed': False}
    try:
        save(out / 'intent.json', result)
        result['preservedProbe'] = put(out / 'PTEditorPointerContextV3', parent.pinned(prepared['candidate']))
        result['preservedPreparation'] = put(out / 'PREPARATION.json', parent.pinned(binding['contextPreparationRef']))
        current_stopped(binding, api, mode, lock_fd); prepared_input(binding)
        parent.need(parent.pinned(result['preservedProbe']) == parent.pinned(prepared['candidate'])
                    and parent.pinned(result['preservedPreparation']) == parent.pinned(binding['contextPreparationRef']),
                    'Preserved read-only input differs')
        result['state'] = 'CONTEXT_V3_PROBE_INPUT_PRESERVED_AND_RETAINED'
        return save(out / 'result.json', result)
    except BaseException as error:
        result.update(state='CONTEXT_V3_INPUT_PRESERVATION_FAILED_RETAINED', errorType=type(error).__name__, error=str(error)[:2048])
        save(out / 'failed.json', result); raise


def release_facts(binding, result_ref, api, *, mode, lock_fd):
    """Input-retention supplement only; no runtime admission or guard release."""
    parent.need(mode in ('normal', 'failed'), 'Explicit original stopped mode required')
    current_stopped(binding, api, mode, lock_fd)
    prepared = prepared_input(binding); value = json.loads(parent.pinned(result_ref))
    saved = Path(binding['windowDirectory']) / 'preserved-readonly-context-input'
    parent.need(result_ref['path'] == str(saved / 'result.json')
                and value['state'] == 'CONTEXT_V3_PROBE_INPUT_PRESERVED_AND_RETAINED'
                and value['ownerRunID'] == binding['ownerRunID'] and value['mode'] == mode
                and value['preparationRef'] == binding['contextPreparationRef']
                and value['retainedInput'] == prepared['candidate'] and value['retainedDirectory'] == prepared['directory']
                and value['preservedProbe']['path'] == str(saved / 'PTEditorPointerContextV3')
                and value['preservedPreparation']['path'] == str(saved / 'PREPARATION.json')
                and value['probeFilesRemoved'] == value['probeDirectoriesRemoved'] == value['targetOperations'] == 0
                and value['runtimeAdmission'] is value['retryAllowed'] is False,
                'Complete same-owner stopped input-retention evidence required')
    parent.need(parent.pinned(value['preservedProbe']) == parent.pinned(prepared['candidate'])
                and parent.pinned(value['preservedPreparation']) == parent.pinned(binding['contextPreparationRef']),
                'Retained input evidence changed')
    current_stopped(binding, api, mode, lock_fd)
    return {'contextInput': {'resultRef': result_ref, 'facts': value,
                                 'runtimeAdmission': False, 'guardReleaseClaimed': False}}
