"""Finite Root-observed host window Raise gate; no GUI input implementation.

The production adapter must hold the original two locks and verify the current
lease, READY state, source freeze, exact process/argv and socket on every current
check. Root performs at most one native window Raise through computer control.
This module never focuses by clicks/keys, changes capture settings, retries an
action or treats a raised window as native input-readiness acceptance.
"""
from pathlib import Path
import json
import math

STATE = 'ROOT_ADMITTED_ONE_HOST_WINDOW_RAISE_NO_GUEST_INPUT'
ALLOWED = {'hostWindowRaises': 1, 'guestMoves': 0, 'guestClicks': 0, 'keys': 0,
           'serviceChanges': 0, 'captureSettingWrites': 0}


def need(ok, message):
    if not ok: raise RuntimeError(message)


def validate_observation(value, request, request_ref, now, pin):
    need(value['state'] == 'ROOT_ACTUALLY_OBSERVED_ONE_NATIVE_WINDOW_RAISE'
         and value['actuallyPerformedNativeRaise'] is True
         and value['requestSHA256'] == request_ref['sha256']
         and value['ownerRunID'] == request['ownerRunID']
         and value['process'] == request['process']
         and value['appPath'] == request['appPath']
         and value['windowTitle'] == request['windowTitle']
         and value['action'] == 'Raise' and value['actionCount'] == 1
         and type(value['actionCount']) is int
         and value['guestMoves'] == value['guestClicks'] == value['keys'] == 0
         and all(type(value[k]) is int for k in ('guestMoves','guestClicks','keys'))
         and request['requestedMonotonic'] <= value['observedMonotonic'] <= now < request['deadline'],
         'Actual current one-Raise Root observation required; no replay')
    refs = [value[k] for k in ('beforeSnapshot', 'actionResult', 'afterSnapshot')]
    need(len({ref['path'] for ref in refs}) == 3, 'Separate actual before/action/after evidence required')
    for ref in refs: pin(ref)


async def run(binding, out, api):
    """Wait for actual Root evidence under original locks, never issue UI input.

    api is a separately reviewed adapter, not arbitrary runtime authority.
    current() must refuse HOLD, inflight work, changed identity or source pins;
    fail() retains an original guard HOLD. Root's observation is an assertion of
    what Root actually did, as with the existing actual-image observation gate.
    """
    need(binding['state'] == STATE and binding['acceptedByRoot'] is True
         and binding['allowedActions'] == ALLOWED and binding['wholeSeconds'] == 60,
         'Distinct bounded host Raise admission required')
    out = Path(out)
    need(not out.exists(), 'Never replay a host Raise gate')
    out.mkdir(mode=0o700)
    result = {'state':'HOST_WINDOW_RAISE_GATE_STARTED', 'retryAllowed':False,
              'nativeInputReadinessAccepted':False, 'guestInputAuthorized':False}
    with api.original_locks():
        try:
            process = api.current(binding)
            started = api.clock()
            request = {'state':'ROOT_ONE_HOST_WINDOW_RAISE_REQUEST',
                'ownerRunID':binding['ownerRunID'], 'process':process,
                'sourceFreezeRef':binding['sourceFreezeRef'],
                'appPath':binding['appPath'], 'windowTitle':binding['windowTitle'],
                'requestedMonotonic':started, 'deadline':started+60,
                'observationPath':str(out/'root-observation.json'),
                'allowedActions':ALLOWED, 'retryAllowed':False}
            ref = api.save(out/'request.json', request)
            value = await api.root_observation(request, ref)
            need(api.current(binding) == process, 'Original owner/process changed across host Raise')
            validate_observation(value, request, ref, api.clock(), api.pinned)
            api.pinned(ref)
            need(api.current(binding) == process and api.clock() < request['deadline'],
                 'Owner/process/deadline changed while validating actual evidence')
            result.update(state='ONE_ROOT_OBSERVED_HOST_WINDOW_RAISE_RECORDED',
                process=process, rootObservation=value, requestRef=ref,
                sourceFreezeRef=binding['sourceFreezeRef'], ownerRunID=binding['ownerRunID'],
                completedMonotonic=api.clock(),
                freshRegisteredCaptureBeforeGuestInputStillRequired=True,
                originalGUIReadinessAndNativeCallbackStillRequired=True)
        except BaseException as error:
            result.update(state='HOST_WINDOW_RAISE_GATE_FAILED_RETAINED',
                errorType=type(error).__name__, error=str(error)[:2048])
            api.save(out/'result.json', result)
            api.fail(binding, 'Host Raise gate failed; preserve without replay')
            raise
        api.save(out/'result.json', result)
    return result


def verify_completed(ref, freeze_ref, owner, process, window, pin, now):
    """Fresh binder prerequisite only, never permission to skip native guards."""
    directory=Path(window)/'host-window-raise'
    need(ref['path']==str(directory/'result.json'),'Exact same-window Raise result required')
    result=json.loads(pin(ref));completed=result['completedMonotonic']
    need(result['state']=='ONE_ROOT_OBSERVED_HOST_WINDOW_RAISE_RECORDED'
         and result['sourceFreezeRef']==freeze_ref and result['ownerRunID']==owner
         and result['process']==process and result['retryAllowed'] is False
         and result['nativeInputReadinessAccepted'] is False
         and result['guestInputAuthorized'] is False
         and result['freshRegisteredCaptureBeforeGuestInputStillRequired'] is True
         and result['originalGUIReadinessAndNativeCallbackStillRequired'] is True
         and type(completed) in (int,float) and math.isfinite(completed)
         and completed<=now<completed+60,
         'Fresh completed same-owner Raise evidence required; no focus acceptance')
    request_ref=result['requestRef']
    need(request_ref['path']==str(directory/'request.json'),'Exact current Raise request required')
    request=json.loads(pin(request_ref))
    need(request['state']=='ROOT_ONE_HOST_WINDOW_RAISE_REQUEST'
         and request['sourceFreezeRef']==freeze_ref and request['ownerRunID']==owner
         and request['process']==process and request['allowedActions']==ALLOWED
         and request['retryAllowed'] is False
         and request['deadline']==request['requestedMonotonic']+60,
         'Same original bounded Raise request required')
    validate_observation(result['rootObservation'],request,request_ref,completed,pin)
    pin(ref);pin(request_ref)
    return result
