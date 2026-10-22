"""Prospective distinct same-FRAME mouse case with one passive V2 observation.

Derived from the preserved original first-mouse phase. Original Root/focus/
buttons/geometry/click rules and240-second whole budget stay unchanged.
No executable parent, target admission, staging, lifecycle or cleanup here.
"""
import hashlib
from pathlib import Path

from editor_blank_capture_protocol import require_unchanged_blank_snapshots
from editor_mouse_capture_protocol import first_selection_point, inspect_first_selection

ALLOWED = {'buttonFreeMoves': 1, 'leftClicks': 1, 'captures': 2,
           'explicitStateQueries': 3, 'keys': 0, 'playback': 0, 'exitAttempts': 0,
           'probeReadbacks': 1, 'passiveNativeExecutions': 1}
LIMITS = {'whole': 240, 'rootObservation': 60, 'action': 30, 'selectedLog': 30,
          'passiveProbe': 20, 'passiveScript': 10}


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def clean_action(action, expected, capture, point, request_id):
    need(action['ok'] is True and action['code'] == 'ok'
         and action['execution_state'] == 'completed'
         and action['cleanup_state'] in ('confirmed', 'not_required')
         and action['failure_phase'] is None and action['next_action'] == 'none'
         and action['deduplicated'] is False and action['action'] == expected
         and action['capture_id'] == capture['captureID']
         and action['controller_id'] == capture['controllerID']
         and action['request_id'] == request_id
         and action['requested_coordinates'] == [{'x': point[0], 'y': point[1]}],
         'Original GUI action did not complete exactly once; never retry')


def same_mapping(before, after):
    need(after['captureID'] != before['captureID']
         and after['controllerID'] == before['controllerID']
         and after['captureNonce'] != before['captureNonce']
         and {k:v for k,v in before['geometry'].items() if k not in ('path','capture_nonce')}
             == {k:v for k,v in after['geometry'].items() if k not in ('path','capture_nonce')},
         'Fresh registered capture with unchanged mapping required')


def state_matches(state, capture):
    # This is an OUTSIDE-transaction observation. MIDDLE can legitimately report
    # focus_ready=False before/after the original API's temporary BOTH mode.
    # The unchanged original GuiAutomationService checks actual focus after its
    # settings transaction and the native guarded callback checks it again.
    geometry = capture['geometry']
    need(state['geometry_valid'] is True
         and state['runtime_id'] == geometry['runtime_id']
         and state['geometry_revision'] == geometry['geometry_revision']
         and state['monitor_id'] == geometry['monitor_id']
         and state['button_mask'] == 0 and type(state['focus_ready']) is bool
         and state['pending_effective_diverged'] is False,
         'Actual same-geometry released-button settled-state observation required')


def settings_restored(before, after):
    keys = ('pending_tablet_mode','effective_tablet_mode',
            'pending_mouse_untrap','effective_mouse_untrap')
    need(all(before[k] == after[k] for k in keys)
         and after['pending_effective_diverged'] is False,
         'Original bounded GUI settings restoration differs')


async def run_selection(binding, blank, out, api):
    """All api observations, capture registry and bounded Root gates are mandatory.

    api.root_observation must verify an actual current immutable request/image,
    capture ID/controller/nonce, owner, operation nonce, stage and deadline.
    It must not auto-publish an observation or infer a pointer position.
    The prospective concrete adapter must separately be reviewed and tested.
    """
    need(binding['state'] == 'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2'
         and binding['acceptedByRoot'] is True
         and binding['allowedActions'] == ALLOWED and binding['limits'] == LIMITS
         and binding['nativeSize'] == [640,512],
         'Distinct passive V2 and first-mouse admission required')
    need(blank['state'] == 'BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED'
         and blank['ownerRunID'] == binding['ownerRunID']
         and blank['attempts'] == {'candidate-readback':1,'environment-setup':1,
                                  'application-launch':1,'capture':1},
         'Actual same-session completed blank phase required')
    out = Path(out)
    need(not out.exists(), 'Never replay a mouse phase')
    out.mkdir(mode=0o700)
    deadline = api.clock() + LIMITS['whole']
    result = {'state':'FIRST_MOUSE_PHASE_STARTED','attempts':{'move':0,'click':0,'capture':0,'state':0,'probeInvocation':0},
              'selectionAccepted':False,'wholeApplicationAcceptance':False,
              'cleanExitClaimed':False,'environmentRestored':False,'retryAllowed':False}

    def current():
        need(api.clock() < deadline, 'Mouse phase deadline exhausted; preserve')
        api.owner(binding['ownerRunID'])
        for source in binding['sourcePins'].values():
            api.pinned(source)
        api.pinned(binding['mountedCandidate'])

    async def observe(stage, capture, point=None):
        current()
        value = await api.root_observation(out, stage, capture, point,
                                          min(deadline, api.clock()+LIMITS['rootObservation']))
        current()
        return value

    async def get_state(label, capture):
        current()
        result['attempts']['state'] += 1
        need(result['attempts']['state'] <= ALLOWED['explicitStateQueries'], 'State query budget exceeded')
        value = await api.state_query(out, label, min(deadline,api.clock()+LIMITS['action']))
        current()
        state_matches(value, capture)
        return value

    async def capture(label, before):
        current()
        result['attempts']['capture'] += 1
        need(result['attempts']['capture'] <= ALLOWED['captures'], 'Capture budget exceeded')
        value = await api.capture(out/(label+'.png'), min(deadline,api.clock()+LIMITS['action']))
        current()
        same_mapping(before,value)
        return value

    try:
        current()
        baseline = api.blank_log()
        label = binding['runLabel']
        require_unchanged_blank_snapshots(baseline,baseline,label,expected_history_bytes=182388)
        before = blank['capture']
        chosen = await observe('before-move', before)
        need(chosen['editorBlankAndUnmodal'] is True, 'Actual blank editor observation required')
        metadata = before['metadata']
        point = first_selection_point(chosen['screenRectangle'],
                    [metadata['image_width'],metadata['image_height']],
                    native_size=binding['nativeSize'])
        bounds = metadata['actionable_bounds']
        need(metadata['actionable'] is True and metadata['coordinate_space'] == 'screenshot_pixels'
             and bounds['x'] <= point[0] < bounds['x']+bounds['width']
             and bounds['y'] <= point[1] < bounds['y']+bounds['height'],
             'Source-derived point outside the actual actionable image')
        require_unchanged_blank_snapshots(baseline,api.blank_log(),label,expected_history_bytes=182388)
        first = await get_state('before-move', before)
        current()
        api.save(out/'move-intent.json',{'point':list(point),'captureID':before['captureID'],'retryAllowed':False})
        result['attempts']['move'] = 1
        moved = await api.move(before, point, 'editor-firstmouse-move',
                               min(deadline,api.clock()+LIMITS['action']))
        clean_action(moved,'move',before,point,'editor-firstmouse-move')
        current()
        # New explicit action scope. The original move must be clean first.
        require_unchanged_blank_snapshots(baseline,api.blank_log(),label,expected_history_bytes=182388)
        result['attempts']['probeInvocation'] = 1
        result['passiveProbeEvidenceDirectory'] = str(out/'passive-pointer')
        probe = await api.passive_pointer(out/'passive-pointer',
                                         min(deadline,api.clock()+LIMITS['passiveProbe']))
        need(probe['state'] == 'PASSIVE_EDITOR_V2_DATA_RECORDED'
             and probe['readbackAttempts'] == 1 and probe['executionAttempts'] == 1
             and probe['inputAuthorized'] is False and probe['hotspotMappingAccepted'] is False,
             'Exact one-shot passive data required; never infer click authority')
        result['passiveProbeResultPath'] = str(out/'passive-pointer'/'result.json')
        native_point = [probe['screen']['mouse_x'],probe['screen']['mouse_y']]
        expected_native = [point[0]-chosen['screenRectangle'][0],
                           point[1]-chosen['screenRectangle'][1]]
        result.update(nativePointer=native_point,sourceNativePoint=expected_native)
        current()
        after_move = await capture('after-move',before)
        second = await get_state('after-move',after_move)
        settings_restored(first,second)
        require_unchanged_blank_snapshots(baseline,api.blank_log(),label,expected_history_bytes=182388)
        seen = await observe('after-move',after_move,list(point))
        tip = seen['observedPointerTip']
        need(type(seen['pointerTipIdentifiable']) is bool
             and ((seen['pointerTipIdentifiable'] is False and tip is None)
                  or (type(tip) is list and len(tip) == 2 and all(type(v) is int for v in tip))),
             'Root must explicitly identify an integer pointer tip or report none')
        result.update(requestedPoint=list(point),observedPointerTip=tip)
        if native_point != expected_native:
            result['state'] = 'NATIVE_POINTER_MISMATCH_RECORDED_NO_CLICK'
            return result
        if seen['pointerTipIdentifiable'] is not True or seen['observedPointerTip'] != list(point):
            result['state'] = 'POINTER_MISMATCH_RECORDED_NO_CLICK'
            return result
        # A clean acknowledgement alone is never click permission.
        current()
        require_unchanged_blank_snapshots(baseline,api.blank_log(),label,expected_history_bytes=182388)
        api.save(out/'click-intent.json',{'point':list(point),'captureID':after_move['captureID'],
                                       'button':'left','count':1,'retryAllowed':False})
        result['attempts']['click'] = 1
        clicked = await api.click(after_move,point,'editor-firstmouse-click',
                                 min(deadline,api.clock()+LIMITS['action']))
        clean_action(clicked,'click',after_move,point,'editor-firstmouse-click')
        until = min(deadline,api.clock()+LIMITS['selectedLog'])
        while True:
            current()
            body = api.blank_log()
            api.log_snapshot(out,body)
            parsed = inspect_first_selection(body,baseline,label,expected_history_bytes=182388)
            if parsed.state == 'SELECTED_ROW4_CHANNEL1':
                break
            need(api.clock() < until, 'Planned selection log did not complete; never repeat click')
            await api.sleep(.1)
        selected_before = api.blank_log()
        need(inspect_first_selection(selected_before,baseline,label,expected_history_bytes=182388).state
             == 'SELECTED_ROW4_CHANNEL1','Complete selected baseline required before capture')
        after_click = await capture('after-click',after_move)
        third = await get_state('after-click',after_click)
        settings_restored(first,third)
        selected_after = api.blank_log()
        need(selected_after == selected_before
             and inspect_first_selection(selected_after,baseline,label,expected_history_bytes=182388).state
                 == 'SELECTED_ROW4_CHANNEL1','Selection log changed across final capture')
        seen = await observe('after-click',after_click,list(point))
        need(seen['row4Channel1CursorVisible'] is True
             and seen['noUnexpectedModalOrContentChange'] is True,
             'Root final selection image differs from planned result')
        result.update(state='ROW4_CHANNEL1_MOUSE_SELECTION_RECORDED',
                      selectionAccepted=True,logSHA256=hashlib.sha256(selected_after).hexdigest(),
                      originalGUISettingsRestored=True)
        return result
    except BaseException as error:
        result.update(state='FIRST_MOUSE_PHASE_FAILED_RETAINED',errorType=type(error).__name__,error=str(error))
        raise
    finally:
        api.save(out/'result.json',result)


class EditorMouseProbeAPI:
    """Preserves the original mouse API; adds only the finite probe carrier."""
    def __init__(self, mouse, probe):
        self.mouse, self.probe = mouse, probe

    def __getattr__(self, name):
        return getattr(self.mouse, name)

    async def passive_pointer(self, out, deadline):
        return await self.probe.run(out, deadline)


async def run_editor_mouse_probe(binding, blank, out, mouse_api, original_api):
    """Future parent calls this inside its existing original operation/lock.

    This composition cannot reserve, restart, stage, release or manufacture a
    FRAME. The parent must bind both original APIs to the same frozen session.
    """
    from editor_pointer_probe import EditorPointerProbe, PROBE_SECONDS, SCRIPT_SECONDS

    need(LIMITS['passiveProbe'] == PROBE_SECONDS and LIMITS['passiveScript'] == SCRIPT_SECONDS
         and mouse_api.original is original_api and mouse_api.frame is original_api.frame
         and mouse_api.binding is binding
         and original_api.binding['ownerRunID'] == binding['ownerRunID'],
         'Same original API/FRAME/owner binding required')
    probe_binding = dict(binding['passiveProbe'])
    need(probe_binding['ownerRunID'] == binding['ownerRunID']
         and probe_binding['sourcePins'] == binding['sourcePins']
         and probe_binding['editorCandidate'] == binding['mountedCandidate'],
         'Passive probe must share exact editor/source/owner pins')
    probe = EditorPointerProbe(probe_binding, original_api)
    return await run_selection(binding, blank, out, EditorMouseProbeAPI(mouse_api, probe))
