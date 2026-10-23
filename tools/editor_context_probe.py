"""Distinct one-readback/one-execution V3 carrier in an ORIGINAL guarded FRAME.

No command entry point, reservation, FRAME creation, staging, service change,
input, click acceptance, restoration or release. A distinct reviewed parent and
full lifecycle integration are still required before actual runtime admission.
"""
import asyncio
import copy
import json
import math
from pathlib import Path
import re

import editor_context_volume as volume
from editor_context_volume import NATIVE, SOURCE
from editor_pointer_probe import OriginalFrameProbeIO, PROBE_SECONDS, SCRIPT_SECONDS, HARNESS, EDITOR, digest, need
from editor_pointer_context import parse_editor_pointer_context

INPUT_STATE = 'ROOT_ADMITTED_PASSIVE_V3_READONLY_CONTEXT_INPUT_ONLY'
FROZEN_STATE = 'ROOT_FROZEN_EDITOR_CONTEXT_V3_DIAGNOSTIC_SOURCE_NOT_EXECUTED'
ALLOWED = {'guestReadbacks': 1, 'passiveNativeExecutions': 1, 'input': 0,
           'hostInputWrites': 0, 'serviceChanges': 0, 'guardReleases': 0}
LIMITS = {'whole': PROBE_SECONDS, 'script': SCRIPT_SECONDS}
TOOLS = Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A/tools')
MODULES = {name: str(TOOLS / (name + '.py')) for name in
           ('editor_context_probe', 'editor_context_volume', 'editor_pointer_context',
            'editor_pointer_snapshot', 'editor_pointer_probe')}

class EditorPointerContextProbe:
    """Consumes one attempt even on refusal/cancellation; never retries."""
    def __init__(self, binding, original, *, io=None):
        self.binding, self.original = copy.deepcopy(binding), original
        self.io = io if io is not None else OriginalFrameProbeIO(original, binding['ownerRunID'])
        self.started = False

    def validate_endpoint(self):
        b = self.binding
        need(str(Path(__file__).resolve()) == MODULES['editor_context_probe']
             and str(Path(volume.__file__).resolve()) == MODULES['editor_context_volume']
             and b['state'] == INPUT_STATE and b['acceptedByRoot'] is True
             and b['target'] == 'amiberry-030' and b['allowedActions'] == ALLOWED
             and b['limits'] == LIMITS
             and b['guestCandidate'] == 'PTContext:PTEditorPointerContextV3'
             and Path(b['stagedCandidate']['path']).name == 'PTEditorPointerContextV3'
             and volume.VOLUME.fullmatch(str(Path(b['stagedCandidate']['path']).parent)) is not None
             and 'namespaceStageRef' not in b, 'Distinct read-only V3 diagnostic input required')

    def validate_input_namespace(self):
        b = self.binding
        self.original.owner(b['ownerRunID'])
        freeze = json.loads(self.original.pinned(b['sourceFreezeRef']))
        need(freeze['state'] == FROZEN_STATE and freeze['sources'] == b['sourcePins']
             and freeze['newWindow'] == b['windowDirectory']
             and freeze['contextPreparationRef'] == b['contextPreparationRef'],
             'Distinct V3 diagnostic source freeze required')
        for name, path in MODULES.items():
            need(freeze['contextModulePaths'][name] == path and path in b['sourcePins'],
                 'Actual selected V3/unchanged V2 transport source required')
            self.original.pinned(b['sourcePins'][path])
        prepared = volume.verify(b['contextPreparationRef'])
        refs = prepared['inputs']
        need(all(b[k] == refs[k] for k in ('nativeSource', 'nativeCandidate', 'compileReceipt'))
             and b['stagedCandidate'] == prepared['candidate']
             and b['capturePreparationRef'] == refs['capturePreparation'],
             'Actual same prepared V3 source/native/editor association required')
        capture = json.loads(self.original.pinned(refs['capturePreparation']))
        need(b['editorCandidate'] == capture['privateCandidate'], 'Exact V3 diagnostic editor required')
        before = self.original.pinned(b['originalProfileBeforeimage'])
        need(before == self.original.pinned(refs['originalProfile'])
             and b['activeProfile']['path'] == str(HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae')
             and self.original.pinned(b['activeProfile']) == before + prepared['appendASCII'].encode('ascii'),
             'Exact original profile plus paired readonly V3 mount required')
        self.original.owner(b['ownerRunID'])

    def validate_output(self, out):
        b = self.binding
        from editor_blank_capture_protocol import recent_line
        recent_line(b['runLabel'])
        window = Path(b['windowDirectory'])
        need(window.parent == Path('/private/tmp') and window.name.startswith('pt24g-')
             and out == window / b['runLabel'] / 'native-context-v3'
             and not out.exists() and not out.is_symlink(),
             'Fresh separate same-window V3 evidence directory required')
        volume.directory_ref(window)
        volume.directory_ref(out.parent)

    async def run(self, out, parent_deadline):
        need(not self.started, 'Never replay a passive pointer probe')
        self.started = True
        need(type(parent_deadline) in (int, float) and math.isfinite(parent_deadline),
             'Finite original parent deadline required')
        deadline = min(parent_deadline, self.original.clock() + PROBE_SECONDS)
        b = self.binding
        need(type(b['ownerRunID']) is str
             and re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,95}', b['ownerRunID']),
             'Explicit safe original owner required')
        self.validate_endpoint()
        out = Path(out)
        self.validate_output(out)

        def current():
            self.original.owner(b['ownerRunID'])
            need(self.original.clock() < deadline, 'Passive probe deadline exhausted; no replay')
            for ref in b['sourcePins'].values():
                self.original.pinned(ref)
            source = self.original.pinned(b['nativeSource'])
            native = self.original.pinned(b['nativeCandidate'])
            staged = self.original.pinned(b['stagedCandidate'])
            need(digest(source) == SOURCE and digest(native) == NATIVE and staged == native,
                 'Exact compiled V3 source/native/staged bytes required')
            receipt = json.loads(self.original.pinned(b['compileReceipt']))
            need(receipt['state'] == 'PASSIVE_EDITOR_CONTEXT_V3_SOURCE_PARSER_AND_NATIVE_HOST_CHECKED_NOT_EXECUTED'
                 and receipt['binary'] == {'bytes': NATIVE[0], 'sha256': NATIVE[1]}
                 and receipt['sourcePinsEqualBeforeAfter']['tools/diagnostics/native_editor_pointer_context.c']
                 == {'bytes': SOURCE[0], 'sha256': SOURCE[1]},
                 'Actual private V3 compile receipt association required')
            need(digest(self.original.pinned(b['editorCandidate'])) == EDITOR,
                 'Exact current editor candidate required')
            self.validate_input_namespace()
            need(self.original.clock() < deadline, 'Passive probe deadline exhausted during input validation; no replay')
            return native

        async def bounded(call):
            current()
            remaining = deadline - self.original.clock()
            need(remaining > 0, 'Probe budget exhausted before dispatch')
            async with asyncio.timeout(remaining):
                value = await call()
            current()
            return value

        out.mkdir(mode=0o700)
        result = {'state': 'PASSIVE_EDITOR_PROBE_STARTED', 'readbackAttempts': 0,
                  'executionAttempts': 0, 'retryAllowed': False, 'inputAuthorized': False,
                  'hotspotMappingAccepted': False, 'lifecyclePending': True}
        try:
            # Retain a zero-dispatch receipt when the initial exact input or
            # owner check refuses. The same strict checks precede target I/O.
            native = current()
            need(bool(b['sourcePins']), 'Actual parent source pins required')
            path = out / 'probe-guest-readback.bin'
            self.original.save(out / 'readback-intent.json',
                               {'guestCandidate': b['guestCandidate'], 'localPath': str(path),
                                'bytes': NATIVE[0], 'sha256': NATIVE[1], 'retryAllowed': False})
            result['readbackAttempts'] = 1
            await bounded(lambda: self.io.readback(b['guestCandidate'], path))
            actual, ref = self.original.read(path)
            need(actual == native, 'Complete guest V3 bytes differ; never execute')
            current()
            result['guestReadback'] = ref
            self.original.save(out / 'snapshot-intent.json',
                               {'script': b['guestCandidate'], 'timeoutSeconds': SCRIPT_SECONDS,
                                'maximumExecutions': 1, 'retryAllowed': False})
            self.original.pinned(ref)
            result['executionAttempts'] = 1
            body = await bounded(lambda: self.io.snapshot(b['guestCandidate'], out))
            self.original.pinned(ref)
            snapshot = parse_editor_pointer_context(body)
            current()
            result.update(state='PASSIVE_EDITOR_V3_CONTEXT_DATA_RECORDED', screen=snapshot.screen,
                          window=snapshot.window, context=snapshot.context)
            return result
        except BaseException as error:
            result.update(state='PASSIVE_EDITOR_PROBE_FAILED_RETAINED',
                          errorType=type(error).__name__, error=str(error)[:2048])
            raise
        finally:
            self.original.save(out / 'result.json', result)
