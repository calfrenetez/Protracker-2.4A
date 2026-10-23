"""Distinct V3 diagnostic source/binding checks; no admission or target action.

Actual-start evidence and a new approved service epoch must be established by
the caller. These checks never synthesize either from a historical transaction.
"""
import json
from pathlib import Path
import sys

import editor_pointer_parent as original
import editor_pointer_binding as saved
import editor_context_probe as probe
import editor_context_blank as blank
from editor_probe_namespace import directory_ref

STATE = 'ROOT_ADMITTED_EDITOR_CONTEXT_V3_DIAGNOSTIC_ONLY'
ALLOWED = dict(blank.ALLOWED, passiveNativeExecutions=1, nativeGuestReadbacks=1,
               hostRaises=0, serviceChanges=0, guardReleases=0)
LIMITS = {'whole': 360, 'blank': 300, 'probe': 20, 'script': 10}
OWN = ('editor_context_binding', 'editor_context_parent', 'editor_context_blank',
       'editor_context_probe', 'editor_context_volume', 'editor_pointer_context',
       'editor_context_retention', 'editor_context_stopped', 'editor_context_settlement')
# Original imported transport/utility dependencies remain selected, even where
# their mouse entry points are unused. No V2 admission is invoked or converted.
MODULES = original.MODULES + tuple(n for n in OWN if n not in original.MODULES)
EXTRA = saved.EXTRA


def source_shape(record, *, pin=None):
    pin = pin or original.pinned
    modules, sources = record['modulePaths'], record['sources']
    original.need(set(modules) == set(MODULES) and type(sources) is dict and sources
                  and all(k == v['path'] for k, v in sources.items()),
                  'Exact distinct V3 selected source map required')
    session = Path(modules['capture_runner']).parent; window = Path(record['newWindow'])
    for path in (session, window):
        original.need(path.parent == Path('/private/tmp') and path.name.startswith('pt24g-')
                      and '..' not in path.parts, 'Separate direct-private V3 endpoints required')
    original.need(session != window, 'V3 source and runtime window must differ')
    directory_ref(session); directory_ref(window)
    local = {'capture_runner', 'capture_case_mounted', 'first_mouse_api', 'capture_support', 'root_observation'}
    shared = {'launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'}
    for name, path in modules.items():
        if name in local: expected = session / (name + '.py')
        elif name in shared: expected = original.HARNESS / 'scripts' / (name + '.py')
        elif name.startswith('amiberry_mcp.'):
            expected = original.HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name.split('.')[-1] + '.py')
        else: expected = original.TOOLS / (name + '.py')
        original.need(path == str(expected) and path in sources, 'Canonical V3 selected source differs')
    extras = {n: str(session / (n + '.py')) for n in EXTRA}
    required = set(extras.values()) | {str(original.TOOLS / (n + '.py')) for n in
        ('editor_pointer_parent', 'editor_pointer_binding', 'editor_probe_stopped',
         'editor_capture_settlement', 'editor_capture_namespace')}
    original.need(record['extraModulePaths'] == extras and required <= sources.keys()
                  and record['contextModulePaths'] == probe.MODULES,
                  'Original saved-file helpers and distinct V3 lifecycle must be selected')
    for ref in sources.values():
        body = pin(ref)
        if Path(ref['path']).suffix == '.py': compile(body, ref['path'], 'exec')
    original.need(pin(sources[str(session / 'capture_case_mounted.py')])
                  == pin(sources[str(original.TOOLS / 'editor_context_blank.py')]),
                  'Selected private blank phase must match reviewed V3 template')
    original.need(original.digest(pin(sources[extras['canonical_profile_session_files']]))
                  == saved.SAVED_FILE_HELPER, 'Unchanged qualified three-file helper required')
    return session, window


def validate(binding, *, pin=None):
    """Also valid after stop; does not read the restored active-profile endpoint."""
    pin = pin or original.pinned
    b = binding; p = b['passiveProbe']; c = b['blankBinding']
    original.need(b['state'] == STATE and b['acceptedByRoot'] is True
                  and b['target'] == 'amiberry-030' and b['allowedActions'] == ALLOWED
                  and b['limits'] == LIMITS, 'Distinct zero-input V3 parent scope required')
    freeze = json.loads(pin(b['sourceFreezeRef']))
    original.need(freeze['state'] == probe.FROZEN_STATE and freeze['sources'] == b['sourcePins']
                  and freeze['modulePaths'] == c['modulePaths'], 'Distinct same-source V3 freeze required')
    session, window = source_shape(freeze, pin=pin)
    original.need(b['sourceFreezeRef']['path'] == str(session / 'SOURCE-FREEZE.json')
                  and b['sourceFreezeRef'] == p['sourceFreezeRef']
                  and b['sourcePins'] == p['sourcePins'] == c['sourcePins']
                  and b['ownerRunID'] == p['ownerRunID'] == c['ownerRunID']
                  and p['windowDirectory'] == str(window)
                  and b['runLabel'] == p['runLabel']
                  and b['outputDirectory'] == c['outputDirectory'] == str(window / b['runLabel'])
                  and b['mountedCandidate'] == p['editorCandidate'] == c['mountedCandidate']
                  and c['state'] == blank.BLANK_STATE and c['readonlyProbe'] == p
                  and c['acceptedByRoot'] is True and c['target'] == 'amiberry-030'
                  and c['allowedActions'] == blank.ALLOWED and c['limits'] == blank.LIMITS
                  and c['expectedHistoryBytes'] == blank.HISTORY_BYTES
                  and c['guestCandidate'] == 'PTCapture:PT24GEdit'
                  and c['localVolumePreparation'] == p['capturePreparationRef']
                  and c['originalProfileBeforeimage'] == p['originalProfileBeforeimage']
                  and c['activeProfile'] == p['activeProfile'], 'Exact V3 parent/blank/input association required')
    original.recent_line(b['runLabel'])
    owner = b['ownerRunID']
    original.need(type(owner) is str and 1 <= len(owner) <= 96 and owner[0].isascii()
                  and owner[0].isalnum() and all(ch.isascii() and (ch.isalnum() or ch in '_-') for ch in owner),
                  'Safe original V3 owner required')
    probe.prepared_input(p, pin=pin)
    capture = json.loads(pin(c['localVolumePreparation']))
    original.need(capture['originalCandidate'] == c['candidate']
                  and original.digest(pin(c['candidate'])) == probe.EDITOR,
                  'Actual same original editor candidate required')
    return freeze


def load_extras(record):
    session, _ = source_shape(record)
    sys.path[:0] = [str(session), str(original.HARNESS / 'scripts'), str(original.TOOLS)]
    return {n: saved.checked_module(n, record['extraModulePaths'][n], record['sources']) for n in EXTRA}


def configure_saved_files(f, sources, capture_ref, context_ref, before_ref):
    """Same complete paired V3 suffix for original patch AND restoration."""
    path = str(original.TOOLS / 'editor_context_volume.py')
    volume = saved.checked_module('editor_context_volume', path, sources)
    prepared = volume.verify(context_ref)
    original.need(prepared['inputs']['capturePreparation'] == capture_ref
                  and original.pinned(prepared['inputs']['originalProfile']) == original.pinned(before_ref),
                  'Exact V3 editor preparation and original profile required')
    capture = json.loads(original.pinned(capture_ref)); before = original.pinned(before_ref)
    suffix = volume.profile_append(before, str(Path(capture['privateCandidate']['path']).parent), prepared['volumePath'])
    original.need(prepared['appendASCII'] == suffix.decode('ascii') and prepared['proposedProfile']
                  == {'bytes': len(before + suffix), 'sha256': original.digest(before + suffix)[1]},
                  'Complete prepared V3 paired suffix differs')
    saved.configure_saved_files(f, sources, capture_ref, before_ref)
    f.APPEND = suffix; f.SESSION = original.digest(before + suffix)
    return f
