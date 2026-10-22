"""Finite one-shot passive V2 carrier, inside the caller's original guarded FRAME.

No executable entry point, staging, lease, service lifecycle, cleanup or input.
The future parent must pin these sources and provide the original owner/file APIs.
"""
import asyncio
import copy
import hashlib
import json
import math
from pathlib import Path
import re

from editor_pointer_snapshot import parse_editor_pointer_snapshot

NATIVE = (15232, '344b2f009f7674a4cb0ffcbaa6a480c442dd0988acacb3f4a04d4b4f209804bb')
SOURCE = (4461, '059067346eb818a5bcb220163131aded2a84172182483293086b6a8d397bc938')
PROBE_SECONDS = 20
SCRIPT_SECONDS = 10
HARNESS = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
EDITOR = (346404, '7cb674a6f8fd0353690ff177972593be45fb96fba31d54bfd0935449d9e75a9b')


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def digest(body):
    return len(body), hashlib.sha256(body).hexdigest()


class OriginalFrameProbeIO:
    """Uses the installed guarded routes; caller installs their original hooks.

    original.owner checks actual FRAME, target, VM argv and socket. No new
    operation or connection-loss recovery is created here. The one-shot carrier
    bounds the complete readback and script exchanges, including initialization.
    """
    def __init__(self, original, owner):
        self.original, self.owner = original, owner

    async def readback(self, guest, path):
        self.original.owner(self.owner)
        await self.original.readback(guest, path)
        self.original.owner(self.owner)

    async def snapshot(self, guest, out):
        from mcp import ClientSession
        from mcp.client.streamable_http import streamable_http_client
        from bridge_checks import require_reply

        self.original.owner(self.owner)
        async with streamable_http_client('http://127.0.0.1:3000/mcp') as (rd, wr, _):
            async with ClientSession(rd, wr) as session:
                await session.initialize()
                self.original.owner(self.owner)
                reply = await session.call_tool('amiga_run_script',
                                                {'script': guest, 'timeout': SCRIPT_SECONDS})
                self.original.owner(self.owner)
                self.original.save(out / 'passive-snapshot-reply.json',
                                   {'reply': reply.model_dump(mode='json')})
                text = require_reply(reply)
        self.original.owner(self.owner)
        return text.encode('ascii')


class EditorPointerProbe:
    """Consumes one attempt even on refusal/cancellation; never retries."""
    def __init__(self, binding, original, *, io=None):
        self.binding, self.original = copy.deepcopy(binding), original
        self.io = io if io is not None else OriginalFrameProbeIO(original, binding['ownerRunID'])
        self.started = False

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
        directory = 'pt24g-editor-pointer-' + b['ownerRunID']
        need(b['guestCandidate'] == 'Dev:Tests/' + directory + '/PTEditorPointerV2'
             and b['stagedCandidate']['path']
             == str(HARNESS / 'runtime/Dev/Tests' / directory / 'PTEditorPointerV2'),
             'Separate owner-bound probe namespace required')
        out = Path(out)
        need(out.is_absolute() and '..' not in out.parts and not out.exists()
             and not out.is_symlink(), 'Fresh separate probe evidence directory required')

        def current():
            self.original.owner(b['ownerRunID'])
            need(self.original.clock() < deadline, 'Passive probe deadline exhausted; no replay')
            for ref in b['sourcePins'].values():
                self.original.pinned(ref)
            source = self.original.pinned(b['nativeSource'])
            native = self.original.pinned(b['nativeCandidate'])
            staged = self.original.pinned(b['stagedCandidate'])
            need(digest(source) == SOURCE and digest(native) == NATIVE and staged == native,
                 'Exact compiled V2 source/native/staged bytes required')
            receipt = json.loads(self.original.pinned(b['compileReceipt']))
            need(receipt['state'] == 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED'
                 and receipt['binary'] == {'bytes': NATIVE[0], 'sha256': NATIVE[1]}
                 and receipt['sourcePinsEqualBeforeAfter']['tools/diagnostics/native_editor_pointer_snapshot.c']
                 == {'bytes': SOURCE[0], 'sha256': SOURCE[1]},
                 'Actual private V2 compile receipt association required')
            need(digest(self.original.pinned(b['editorCandidate'])) == EDITOR,
                 'Exact current editor candidate required')
            return native

        async def bounded(call):
            current()
            remaining = deadline - self.original.clock()
            need(remaining > 0, 'Probe budget exhausted before dispatch')
            async with asyncio.timeout(remaining):
                value = await call()
            current()
            return value

        native = current()
        need(bool(b['sourcePins']), 'Actual parent source pins required')
        out.mkdir(mode=0o700)
        result = {'state': 'PASSIVE_EDITOR_PROBE_STARTED', 'readbackAttempts': 0,
                  'executionAttempts': 0, 'retryAllowed': False, 'inputAuthorized': False,
                  'hotspotMappingAccepted': False, 'lifecyclePending': True}
        try:
            path = out / 'probe-guest-readback.bin'
            self.original.save(out / 'readback-intent.json',
                               {'guestCandidate': b['guestCandidate'], 'localPath': str(path),
                                'bytes': NATIVE[0], 'sha256': NATIVE[1], 'retryAllowed': False})
            result['readbackAttempts'] = 1
            await bounded(lambda: self.io.readback(b['guestCandidate'], path))
            actual, ref = self.original.read(path)
            need(actual == native, 'Complete guest V2 bytes differ; never execute')
            current()
            result['guestReadback'] = ref
            self.original.save(out / 'snapshot-intent.json',
                               {'script': b['guestCandidate'], 'timeoutSeconds': SCRIPT_SECONDS,
                                'maximumExecutions': 1, 'retryAllowed': False})
            self.original.pinned(ref)
            result['executionAttempts'] = 1
            body = await bounded(lambda: self.io.snapshot(b['guestCandidate'], out))
            self.original.pinned(ref)
            snapshot = parse_editor_pointer_snapshot(body)
            current()
            result.update(state='PASSIVE_EDITOR_V2_DATA_RECORDED', screen=snapshot.screen,
                          window=snapshot.window)
            return result
        except BaseException as error:
            result.update(state='PASSIVE_EDITOR_PROBE_FAILED_RETAINED',
                          errorType=type(error).__name__, error=str(error)[:2048])
            raise
        finally:
            self.original.save(out / 'result.json', result)
