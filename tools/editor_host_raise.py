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
import asyncio
from contextlib import contextmanager
import hashlib
import os
import stat
import time

STATE = 'ROOT_ADMITTED_ONE_HOST_WINDOW_RAISE_NO_GUEST_INPUT'
ALLOWED = {'hostWindowRaises': 1, 'guestMoves': 0, 'guestClicks': 0, 'keys': 0,
           'serviceChanges': 0, 'captureSettingWrites': 0, 'registeredCaptures': 1}


def need(ok, message):
    if not ok: raise RuntimeError(message)


def validate_observation(value, request, request_ref, now, pin):
    need(type(value['observedMonotonic']) in (int,float) and math.isfinite(value['observedMonotonic'])
         and value['state'] == 'ROOT_ACTUALLY_OBSERVED_ONE_NATIVE_WINDOW_RAISE'
         and value['actuallyPerformedNativeRaise'] is True
         and value['requestSHA256'] == request_ref['sha256']
         and value['ownerRunID'] == request['ownerRunID']
         and value['operationNonceSHA256'] == request['operationNonceSHA256']
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
                'operationNonceSHA256':binding['operationNonceSHA256'],
                'appPath':binding['appPath'], 'windowTitle':binding['windowTitle'],
                'requestedMonotonic':started, 'deadline':min(started+60,binding.get('wholeDeadline',started+60)),
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


class SameFrameAPI:
    """Actual existing parent FRAME/command FD, plus original transport lock.

    No new FRAME, lease, target connection, native UI action or guard mutation.
    An exception propagates through the parent's original operation to HOLD.
    """
    APP='/Users/james1/Documents/Codex/2026-08-18/open-calfrenetez-amicontinuity-read-codex-start/work/amicontinuity/tools/amiberry/local/Amiberry-8.3.0-fixed.app'
    TITLE='Amiberry - [A1200-030-DEV]'

    def __init__(self,binding,original,frame,lock_fd,baseline):
        import editor_pointer_parent as parent
        from editor_probe_namespace import checked_command_lock,save
        self.parent=parent;self.checked=checked_command_lock
        self.binding,self.original,self.frame,self.lock_fd=binding,original,frame,lock_fd
        self.baseline=baseline;self.pinned=parent.pinned;self.save=save;self.clock=time.monotonic

    def current(self,admission):
        from editor_readonly_probe import require_current_mounted_input
        from editor_blank_capture_protocol import require_unchanged_blank_snapshots
        from editor_pointer_binding import checked_module
        self.checked(self.lock_fd)
        need(self.original.binding is self.binding['blankBinding'] and self.original.frame is self.frame,
             'Same actual original blank API and FRAME required')
        self.original.owner(self.binding['ownerRunID'])
        require_current_mounted_input(self.binding['passiveProbe'],self.original)
        for source in self.binding['sourcePins'].values():self.pinned(source)
        need(admission['ownerRunID']==self.binding['ownerRunID']
             and admission['sourceFreezeRef']==self.binding['sourceFreezeRef']
             and admission['operationNonceSHA256']==hashlib.sha256(self.frame['nonce'].encode()).hexdigest(),
             'Exact current Root request owner/freeze/FRAME required')
        freeze=json.loads(self.pinned(self.binding['sourceFreezeRef']))
        service=checked_module('service_readiness',freeze['extraModulePaths']['service_readiness'],self.binding['sourcePins'])
        now=service.observe_current();ready=json.loads(self.pinned(self.binding['serviceReadinessRef']))
        need(now['job']==ready['observed']['job'] and now['processIdentity']==ready['observed']['processIdentity']
             and now['listenerPID']==int(now['job']['pid']) and now['noRecoveryActivation'] is True
             and now['serial']=={'connected':True,'host':'127.0.0.1','port':2345},
             'Original approved service identity/endpoint changed')
        run=self.parent.HARNESS/'runtime/Dev/Tests'/self.binding['runLabel']/'editor.log'
        require_unchanged_blank_snapshots(self.baseline,self.parent.read(run,16384)[0],
            self.binding['runLabel'],expected_history_bytes=182388)
        self.checked(self.lock_fd)
        return self.binding['blankBinding']['ownedProcess']

    @contextmanager
    def original_locks(self):
        self.checked(self.lock_fd)
        path=self.parent.HARNESS/'runtime/launcher-transport.lock';before=path.lstat()
        need(stat.S_IMODE(before.st_mode)==0o600,'Original private transport lock required')
        with self.parent.command_lock(path) as fd:
            facts=lambda st:{k:getattr(st,k,0) for k in self.parent.FACTS}
            need(facts(before)==facts(os.fstat(fd))==facts(path.lstat()),'Transport lock identity changed')
            try:yield
            finally:
                self.checked(self.lock_fd)
                need(facts(before)==facts(os.fstat(fd))==facts(path.lstat()),'Transport lock changed during Root observation')

    async def root_observation(self,request,ref):
        while self.clock()<request['deadline']:
            self.current(request);self.pinned(ref)
            try:raw,receipt=self.parent.read(request['observationPath'])
            except FileNotFoundError:await asyncio.sleep(.05);continue
            value=json.loads(raw)
            need(value['rootActuallyUsedComputerControl'] is True
                 and value['noAppLaunchOrGuestInputPerformed'] is True,'Actual Root computer-control evidence required')
            self.pinned(receipt);return value
        raise RuntimeError('Root Raise observation expired; preserve without retry')

    def fail(self,binding,reason):
        # The enclosing unchanged original operation latches HOLD on propagation.
        # Never manufacture a second guard operation or use another owner's lease.
        pass


async def run_same_frame(binding,blank,original,frame,lock_fd):
    """After genuine blank capture, before mouse: one Raise and fresh capture.

    All60 seconds, including recapture, fit the original720-second supervisor.
    Blank setup already exists, so every new failure uses full stopped cleanup.
    """
    import editor_pointer_parent as parent
    from editor_blank_capture_protocol import require_unchanged_blank_snapshots
    from editor_probe_namespace import save
    need(binding['state']==parent.READONLY_STATE
         and blank['state']=='BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED',
         'Actual distinct readonly blank phase required before host Raise')
    deadline=time.monotonic()+60;case=Path(binding['outputDirectory'])
    baseline=parent.read(case/'editor-after-capture.log',16384)[0]
    need(hashlib.sha256(baseline).hexdigest()==blank['logSHA256'],'Actual blank log identity differs')
    api=SameFrameAPI(binding,original,frame,lock_fd,baseline)
    admission={'state':STATE,'acceptedByRoot':True,'allowedActions':dict(ALLOWED),'wholeSeconds':60,
        'ownerRunID':binding['ownerRunID'],'sourceFreezeRef':binding['sourceFreezeRef'],
        'operationNonceSHA256':hashlib.sha256(frame['nonce'].encode()).hexdigest(),
        'appPath':api.APP,'windowTitle':api.TITLE,'wholeDeadline':deadline}
    out=case/'host-window-raise'
    await run(admission,out,api)
    # Transport lock is released; the original capture API acquires its own
    # guarded exchanges. Command FD and actual original FRAME remain unchanged.
    api.current(admission);need(time.monotonic()<deadline,'Host Raise whole deadline exhausted before recapture')
    async with asyncio.timeout(deadline-time.monotonic()):
        capture=await original.capture(out/'after-raise.png')
    api.current(admission);need(time.monotonic()<deadline,'Host Raise whole deadline exhausted after recapture')
    need(capture['captureID']!=blank['capture']['captureID']
         and capture['controllerID']==blank['capture']['controllerID']
         and capture['captureNonce']!=blank['capture']['captureNonce'],
         'Fresh original registered same-controller capture required after Raise')
    run_log=parent.HARNESS/'runtime/Dev/Tests'/binding['runLabel']/'editor.log'
    require_unchanged_blank_snapshots(baseline,parent.read(run_log,16384)[0],binding['runLabel'],expected_history_bytes=182388)
    save(out/'post-raise-capture.json',{'state':'FRESH_REGISTERED_CAPTURE_AFTER_HOST_RAISE',
        'capture':capture,'guestInputAuthorized':False,'nativeInputReadinessAccepted':False,
        'originalBlankLogSHA256':blank['logSHA256'],'registeredCaptureCount':1,
        'sameOriginalFrame':True,'retryAllowed':False})
    return dict(blank,capture=capture)
