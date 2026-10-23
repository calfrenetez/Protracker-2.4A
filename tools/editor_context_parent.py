"""One original FRAME: blank capture then passive V3 data, with zero GUI input.

No runtime CLI, reservation, startup, restart, stop or release. Actual-start
binding remains a separate prerequisite; HOST qualification is not admission.
"""
import asyncio
import os
from pathlib import Path

import editor_pointer_parent as original
import editor_context_binding as binding_source
import editor_context_probe as probe
from editor_blank_capture_protocol import require_unchanged_blank_snapshots

STATE = binding_source.STATE


async def run_same_frame(binding, captured, api, *, deadline, carrier=None):
    """Diagnostic data only; a mismatch never dispatches any corrective input."""
    b = binding; out = Path(b['outputDirectory'])
    original.need(not os.path.lexists(out / 'context-parent-result.json')
                  and not os.path.lexists(out / 'native-context-v3'),
                  'Never replay a V3 context diagnostic')
    original.need(api.binding == b['blankBinding'] and api.frame is not None,
                  'Same actual original blank API and FRAME required')
    original.need(captured['state'] == 'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED'
                  and captured['ownerRunID'] == b['ownerRunID']
                  and captured['attempts'] == {'candidate-readback': 1, 'environment-setup': 1,
                                               'application-launch': 1, 'capture': 1},
                  'Completed one original blank capture required before context data')

    def current():
        original.need(api.clock() < deadline, 'V3 parent deadline exhausted; no replay')
        probe.require_current_mounted_input(b['passiveProbe'], api)
        for ref in b['sourcePins'].values(): api.pinned(ref)
        original.need(api.clock() < deadline, 'V3 parent deadline exhausted during current check')

    current()
    guest_log = original.HARNESS / 'runtime/Dev/Tests' / b['runLabel'] / 'editor.log'
    baseline = api.read(out / 'editor-after-capture.log', 16384)[0]
    before = api.read(guest_log, 16384)[0]
    require_unchanged_blank_snapshots(baseline, before, b['runLabel'],
                                    expected_history_bytes=binding_source.blank.HISTORY_BYTES)
    api.put(out / 'editor-before-context.log', before)
    observer = carrier if carrier is not None else probe.EditorPointerContextProbe(b['passiveProbe'], api)
    original.need(observer.original is api and observer.binding == b['passiveProbe'],
                  'V3 carrier must retain the same original API and exact binding')
    result = {'state': 'EDITOR_CONTEXT_V3_DIAGNOSTIC_STARTED', 'inputAuthorized': False,
              'hotspotMappingAccepted': False, 'selectionAccepted': False, 'focusAccepted': False,
              'guiInputEvents': 0, 'hostRaises': 0, 'retryAllowed': False, 'lifecyclePending': True,
              'nativeCleanExitClaimed': False, 'wholeApplicationAcceptance': False}
    try:
        async with asyncio.timeout(deadline - api.clock()):
            observed = await observer.run(out / 'native-context-v3', deadline)
        current()
        original.need(observed['state'] == 'PASSIVE_EDITOR_V3_CONTEXT_DATA_RECORDED'
                      and observed['inputAuthorized'] is False and observed['hotspotMappingAccepted'] is False,
                      'Only passive V3 data may complete this diagnostic')
        after = api.read(guest_log, 16384)[0]
        api.put(out / 'editor-after-context.log', after)
        require_unchanged_blank_snapshots(before, after, b['runLabel'],
                                        expected_history_bytes=binding_source.blank.HISTORY_BYTES)
        current()
        result.update(state='EDITOR_CONTEXT_V3_DIAGNOSTIC_DATA_RECORDED_NO_INPUT', contextObservation=observed)
        return result
    except BaseException as error:
        result.update(state='EDITOR_CONTEXT_V3_DIAGNOSTIC_FAILED_RETAINED',
                      errorType=type(error).__name__, error=str(error)[:2048])
        raise
    finally:
        api.save(out / 'context-parent-result.json', result)


def execute(binding, dependencies):
    """Finite composition under the ORIGINAL command lock, reservation and FRAME."""
    b, d = binding, dependencies
    with original.command_lock(original.HARNESS / 'runtime/test.lock'):
        original.need('AMIGA_LAUNCH_INFLIGHT' not in os.environ, 'Cannot inherit another FRAME')
        binding_source.validate(b)
        out = Path(b['outputDirectory'])
        original.need(not os.path.lexists(out), 'Never replay a V3 diagnostic parent')
        prepared = probe.prepared_input(b['passiveProbe'])
        p = b['passiveProbe']
        original.need(p['activeProfile']['path']
                      == str(original.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae')
                      and original.pinned(p['activeProfile'])
                      == original.pinned(p['originalProfileBeforeimage']) + prepared['appendASCII'].encode('ascii'),
                      'Exact mounted V3 profile required before original operation')
        guard = d.guard(original.HARNESS); lease = d.lease_input(original.HARNESS, 'amiberry-030')
        status = guard.status()
        original.need(status['phase'] == 'READY' and not status['holds'] and status['inflight'] is None
                      and lease['run_id'] == b['ownerRunID'], 'Current original READY owner required')
        with guard._locked() as (_, state): guard._owner(state, lease, 'amiberry-030')
        d.current_service(b)
        d.install_client(); d.install_ipc(original.HARNESS)
        with d.operation(original.HARNESS, 'amiberry-030',
                         'One blank capture and passive V3 context; zero GUI input or Raise',
                         may_leave_running=True) as frame:
            api = d.original_api(b['blankBinding'], frame)
            api.owner(b['ownerRunID'])
            deadline = api.clock() + binding_source.LIMITS['whole']
            probe.require_current_mounted_input(b['passiveProbe'], api)

            async def run():
                async with asyncio.timeout(binding_source.LIMITS['whole']):
                    d.current_service(b)
                    captured = await d.blank_case(b['blankBinding'], out, api)
                    api.owner(b['ownerRunID']); d.current_service(b)
                    result = await run_same_frame(b, captured, api, deadline=deadline)
                    api.owner(b['ownerRunID']); d.current_service(b)
                    return result
            return asyncio.run(run())
