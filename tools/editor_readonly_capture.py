"""Distinct paired-volume blank capture through original harness APIs; no input or exit.

This different case does not adopt or weaken the old full-lifetime adapter's
independent review gates. It grants no playback, clean exit or release result.
"""
from pathlib import Path
import json
from editor_readonly_probe import require_current_mounted_input, BLANK_STATE
import asyncio
import hashlib
from editor_blank_capture_protocol import inspect_blank_log, require_unchanged_blank_snapshots, recent_line

INFRA = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
CANDIDATE_SHA = '7cb674a6f8fd0353690ff177972593be45fb96fba31d54bfd0935449d9e75a9b'
CANDIDATE_BYTES = 346404
HISTORY_BYTES = 182388
LIMITS = {'whole': 300, 'readback': 150, 'setup': 20, 'ready': 30, 'capture': 30}
ALLOWED = {'applicationLaunches': 1, 'environmentSetups': 1, 'candidateGuestReadbacks': 1,
           'captures': 1, 'mouseEvents': 0, 'keyEvents': 0, 'playbackActions': 0,
           'exitAttempts': 0, 'restoreAttempts': 0}

def need(ok, message):
    if not ok:
        raise RuntimeError(message)

def environment_script(device):
    settings = {'PT24G_RECENT_PREFIX': device+'/recent', 'PT24G_RECOVERY_DIR': device+'/copies',
                'PT24G_RECOVERY_MEDIA': 'fixed', 'PT24G_RECOVERY_SECONDS': '300',
                'PT24G_RECOVERY_REMOVABLE': '0'}
    lines = ['FailAt 1', 'CD '+device]
    for index, key in enumerate(settings):
        lines += ['If EXISTS ENV:'+key, 'Copy ENV:'+key+' before-'+str(index), 'EndIf']
    lines += ['Echo ready >backup-ready']
    for index, (key, value) in enumerate(settings.items()):
        lines += ['SetEnv '+key+' '+value, 'Copy ENV:'+key+' active-'+str(index)]
    return settings, ('\n'.join(lines+['Echo ready >setup-done'])+'\n').encode()

async def run_case(binding, out, api):
    """api checks the actual owner/frame at each target boundary; failures retain.

    Snapshot reads are bounded but allow the guest's own append between reads.
    Before and after capture must still be complete and byte-identical.
    """
    out = Path(out)
    recent_line(out.name)  # Refuse bad namespace before any Guest construction.
    need(binding['state']==BLANK_STATE and
         binding['allowedActions']==ALLOWED and binding['limits']==LIMITS and
         binding['expectedHistoryBytes']==HISTORY_BYTES, 'Narrow capture binding differs')
    probe = binding['readonlyProbe']
    need(binding['acceptedByRoot'] is True and binding['target']=='amiberry-030'
         and binding['ownerRunID']==probe['ownerRunID'] and binding['sourcePins']==probe['sourcePins']
         and binding['mountedCandidate']==probe['editorCandidate']
         and binding['localVolumePreparation']==probe['capturePreparationRef']
         and binding['activeProfile']==probe['activeProfile']
         and binding['originalProfileBeforeimage']==probe['originalProfileBeforeimage']
         and out==Path(probe['windowDirectory'])/probe['runLabel'], 'Exact same paired-input blank subphase required')
    candidate = api.pinned(binding['candidate'])
    need(len(candidate)==CANDIDATE_BYTES and hashlib.sha256(candidate).hexdigest()==CANDIDATE_SHA,
         'Exact current candidate required')
    # The alias already exists in a separately prepared local volume. This
    # actor never creates or rewrites the immutable candidate. Actual mapped
    # guest bytes still must be read back before the only app launch.
    mounted = binding['mountedCandidate']
    path = Path(mounted['path'])
    need(path.name=='PT24GEdit' and binding['guestCandidate']=='PTCapture:PT24GEdit',
         'Exact private guest executable alias required')
    preparation = json.loads(api.pinned(binding['localVolumePreparation']))
    need(preparation['state']=='PRIVATE_LOCAL_CANDIDATE_VOLUME_AND_ADDITIVE_PROFILE_SUFFIX_PREPARED_NOT_ADMITTED_OR_MOUNTED' and
         preparation['targetOperations']==0 and preparation['canonicalProfileWrites']==0 and
         preparation['guestCandidate']==binding['guestCandidate'] and
         preparation['privateCandidate']==mounted and preparation['originalCandidate']==binding['candidate'],
         'Actual original local candidate preparation required; no substituted alias')
    need(api.pinned(mounted)==candidate, 'Local candidate differs before Guest admission')
    require_current_mounted_input(binding['readonlyProbe'], api)
    for ref in binding['sourcePins'].values():
        api.pinned(ref)
    api.owner(binding['ownerRunID'])
    need(not out.exists(), 'Capture evidence namespace exists; never replay')
    out.mkdir(mode=0o700)
    deadline = api.clock()+LIMITS['whole']
    result = {'state':'CAPTURE_CASE_STARTED', 'allowedActions':ALLOWED, 'attempts':{},
              'ownerRunID':binding['ownerRunID'], 'retryAllowed':False,
              'wholeApplicationAcceptance':False, 'cleanExitClaimed':False,
              'environmentRestored':False, 'lifecyclePending':True}
    def current():
        need(api.clock()<deadline, 'Whole capture deadline exhausted; retain state')
        require_current_mounted_input(binding['readonlyProbe'], api)
        need(api.clock()<deadline, 'Whole capture deadline exhausted during input validation')
    async def call(label, seconds, action):
        current();result['attempts'][label]=result['attempts'].get(label,0)+1
        need(result['attempts'][label]==1, 'Target action replay refused')
        api.save(out/(label+'-intent.json'), {'action':label, 'maximumSeconds':seconds, 'retryAllowed':False})
        async with asyncio.timeout(min(seconds, deadline-api.clock())):
            value = await action()
        current();return value
    async def wait_for(predicate, seconds):
        until = min(deadline, api.clock()+seconds)
        while True:
            current()
            value = predicate()
            if value:
                return value
            need(api.clock()<until, 'Finite startup observation timed out; retain state')
            await api.sleep(.1)
    try:
        current();guest=api.Guest(INFRA,out)
        current();run=api.prepare_run(guest,out)
        need(run==guest.share/out.name and guest.device=='Dev:Tests/' and
             guest.launch==guest.share/('launch-'+out.name), 'Original Guest paths differ')
        device=guest.device+out.name
        (run/'copies').mkdir()
        staged=mounted
        need(api.pinned(staged)==candidate,'Local candidate changed before full guest readback')
        result['mountedCandidate']=staged
        current()
        await call('candidate-readback',LIMITS['readback'],
                   lambda:api.readback(binding['guestCandidate'],out/'candidate-readback.bin'))
        actual,ref=api.read(out/'candidate-readback.bin',2097152)
        need(actual==candidate, 'Actual guest readback differs; do not execute')
        result['guestCandidateReadback']=ref
        settings,setup=environment_script(device)
        api.put(run/'setup-env',setup)
        setup_launch=('FailAt 1\nExecute '+device+'/setup-env\n').encode()
        api.put(guest.launch,setup_launch)
        api.put(out/'setup-launch.script',setup_launch)
        current();api.require_running_guest(guest,out,'before-environment')
        async def start():
            guest.start()
        await call('environment-setup',30,start)
        def exact(name,body):
            return api.optional(run/name,65536)==body
        await wait_for(lambda:exact('setup-done',b'ready\n'),LIMITS['setup'])
        need(exact('backup-ready',b'ready\n'), 'Original ENV backup incomplete')
        for index,value in enumerate(settings.values()):
            need(exact('active-'+str(index),value.encode()), 'Private ENV override differs')
        api.put(out/'environment-setup-launch.log',api.read(out/'launch.log',1048576)[0])
        app=('FailAt 21\nStack 65536\nCD '+device+'\n'+binding['guestCandidate']+' >editor.log\n'
             'Echo $RC >editor.rc\nEcho done >editor-done\n').encode()
        api.replace(guest.launch,setup_launch,app)
        api.put(out/'application-launch.script',app)
        need(api.pinned(staged)==candidate, 'Mounted candidate changed before execution')
        current();api.require_running_guest(guest,out,'before-blank-application')
        await call('application-launch',30,start)
        def log_ready():
            body=api.optional(run/'editor.log',16384)
            if body is None:
                return False
            api.put_unique_snapshot(out,body)
            observed=inspect_blank_log(body,out.name,expected_history_bytes=HISTORY_BYTES)
            return body if observed.state=='CLEAN_BLANK_PREFIX' else False
        await wait_for(log_ready,LIMITS['ready'])
        current();api.require_running_guest(guest,out,'before-capture')
        before=api.read(run/'editor.log',16384)[0]
        api.put(out/'editor-before-capture.log',before)
        require_unchanged_blank_snapshots(before,before,out.name,expected_history_bytes=HISTORY_BYTES)
        result['capture']=await call('capture',LIMITS['capture'],lambda:api.capture(out/'editor.png'))
        after=api.read(run/'editor.log',16384)[0]
        api.put(out/'editor-after-capture.log',after)
        observed=require_unchanged_blank_snapshots(before,after,out.name,expected_history_bytes=HISTORY_BYTES)
        current();api.require_running_guest(guest,out,'after-capture')
        need(api.pinned(staged)==candidate,'Mounted candidate changed through capture')
        result.update(state='BLANK_LOG_AND_REGISTERED_CAPTURE_RECORDED',logSHA256=observed.sha256,
                      nativeWindowVisualComparisonPending=True)
        return result
    except BaseException as error:
        result.update(state='CAPTURE_CASE_FAILED_RETAINED',errorType=type(error).__name__,error=str(error))
        raise
    finally:
        api.save(out/'result.json',result)
