"""One original normal-stop child with authority refusal BEFORE any intent.

No signals, retries, service change or public release in this supervisor. The
selected original lifecycle helper sets its own exact lease, as it requires.
"""
import json,os,subprocess,sys,time
from pathlib import Path
HARNESS=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
AUTHORITY=('AMIGA_LAUNCH_LEASE','AMIGA_LAUNCH_INFLIGHT')
if __name__=='__main__':
    clock=time.get_clock_info('monotonic')
    if not (sys.executable==str(HARNESS/'.venv/bin/python') and tuple(sys.version_info[:3])==(3,14,0)
            and sys.platform=='darwin' and clock.monotonic and not clock.adjustable):
        raise RuntimeError('Original harness clock required before stop supervisor imports')
    if any(k in os.environ for k in AUTHORITY):raise RuntimeError('No inherited stop authority before imports or intent')
import editor_pointer_parent as parent
import editor_context_binding as bindings
import editor_context_dispatch as dispatcher
from editor_probe_namespace import save
STATE='ROOT_ACCEPTED_ONE_FRESH_V3_NORMAL_STOP_SUPERVISOR'
ALLOWED={'originalNormalStopCalls':1,'supervisorSignals':0,'serviceChanges':0,'guardReleases':0,'retries':0}
SECONDS=180


def validate(ref):
    # Deliberately precedes even reading caller records or original guard input.
    parent.need(not any(k in os.environ for k in AUTHORITY),'No inherited stop authority before records or intent')
    b=json.loads(parent.pinned(ref))
    parent.need(b['state']==STATE and b['acceptedByRoot'] is True and b['allowedActions']==ALLOWED
                and b['limits']=={'supervisor':SECONDS},'Distinct normal-stop supervisor scope required')
    p=json.loads(parent.pinned(b['parentBindingRef']));freeze=bindings.validate(p)
    window=Path(p['outputDirectory']).parent;session=Path(p['blankBinding']['modulePaths']['capture_runner']).parent
    parent.need(b['ownerRunID']==p['ownerRunID'] and b['windowDirectory']==str(window)
                and ref['path']==str(window/'normal-stop-supervisor-binding.json')
                and b['parentBindingRef']['path']==str(window/'context-parent-binding.json')
                and b['contextRef']['path']==str(session/'ROOT-CURRENT-CONTEXT.json')
                and str(Path(__file__)) in p['sourcePins'],'Same actual new parent/window/frozen supervisor required')
    context=json.loads(parent.pinned(b['contextRef']));helper=b['lifecycleSourceRef']
    parent.need(context['state']=='ROOT_REVIEWED_CONTEXT_V3_CURRENT_LIFECYCLE_CONTEXT'
                and context['sessionDirectory']==str(session) and context['windowDirectory']==str(window)
                and context['runLabel']==p['runLabel'] and Path(helper['path']).parent==session
                and Path(helper['path']).suffix=='.py' and p['sourcePins'].get(helper['path'])==helper
                and p['sourcePins'].get(b['contextRef']['path'])==b['contextRef'],
                'Exact reviewed context and original selected lifecycle source required')
    parent.pinned(helper)
    service_ref=b['serviceProcessRef'];ready_ref=freeze['actualApprovedServiceReadinessRef']
    parent.need(service_ref['path']==str(window/'ROOT_CURRENT_DEVBENCH_PROCESS.json')
                and p['sourcePins'].get(ready_ref['path'])==ready_ref,'Exact current service and frozen actual epoch required')
    service=json.loads(parent.pinned(service_ref));ready=json.loads(parent.pinned(ready_ref))
    parent.need(ready['state']=='ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY'
                and ready['acceptedByRoot'] is True and type(service['pid']) is int
                and service['pid']==int(ready['observed']['job']['pid'])
                and service['approvedStartIdentity']==ready['observed']['processIdentity'],
                'Service record differs from actual approved epoch')
    parent.need(b['diagnosticResultRef']['path']==str(Path(p['outputDirectory'])/'context-parent-result.json'),
                'Actual completed parent result required')
    result=json.loads(parent.pinned(b['diagnosticResultRef']))
    parent.need(result['state']=='EDITOR_CONTEXT_V3_DIAGNOSTIC_DATA_RECORDED_NO_INPUT'
                and result['guiInputEvents']==result['hostRaises']==0
                and result['inputAuthorized'] is result['selectionAccepted'] is result['hotspotMappingAccepted'] is False
                and result['contextObservation']['state']=='PASSIVE_EDITOR_V3_CONTEXT_DATA_RECORDED'
                and result['contextObservation']['readbackAttempts']==result['contextObservation']['executionAttempts']==1,
                'One successful zero-input V3 diagnostic required before normal stop')
    for name in ('owned-stop-intent.json','ROOT_ACTUAL_OWNED_STOP.json','normal-disconnect-intent.json',
                 'normal-stop-supervisor-intent.json','normal-stop-supervisor-child.json',
                 'normal-stop-supervisor.stdout','normal-stop-supervisor.stderr',
                 'normal-stop-supervisor-result.json','normal-stop-supervisor-failed.json'):
        parent.need(not os.path.lexists(window/name),'Never replay a stop supervisor or original stop')
    return b,p,window


def load_dependencies(p):
    d=dispatcher.load_dependencies(p)
    path=str(Path(__file__).parent/'editor_probe_stopped.py');parent.pinned(p['sourcePins'][path])
    from editor_probe_stopped import HostObservations
    d.observations=HostObservations()
    return d


def execute(ref,dependencies=None):
    b,p,window=validate(ref);d=dependencies if dependencies is not None else load_dependencies(p)
    lease_body,lease_ref=parent.read(window/'private-lease.json');lease=d.decode(lease_body)['lease']
    parent.need(lease['run_id']==b['ownerRunID'],'Exact original stop lease required')
    guard=d.guard(HARNESS)
    with parent.command_lock(HARNESS/'runtime/test.lock'):
        for item in (ref,lease_ref,b['contextRef'],b['lifecycleSourceRef'],b['diagnosticResultRef']):parent.pinned(item)
        for source in p['sourcePins'].values():parent.pinned(source)
        state=guard.status()
        parent.need(state['phase']=='READY' and not state['holds'] and state['inflight'] is None,
                    'Current original normal owner required')
        with guard._locked() as (_,raw):guard._owner(raw,lease,'amiberry-030')
        service=json.loads(parent.pinned(b['serviceProcessRef']))
        argv=d.observations.command(['/bin/ps','-p',str(service['pid']),'-o','pid=,pgid=,command='],(0,)).strip().split(None,2)
        identity=d.observations.command(['/bin/ps','-p',str(service['pid']),'-o','pid=,lstart=,command='],(0,)).strip()
        parent.need(argv==service['argv'] and identity==service['approvedStartIdentity'].strip(),
                    'Current exact shared service changed before original stop')
        save(window/'normal-stop-supervisor-intent.json' ,{'state':STATE,'sourceBindingRef':ref,
            'supervisorSeconds':SECONDS,'authorityEnvironmentPassed':False,'signals':0,'retryAllowed':False})
    argv=[str(HARNESS/'.venv/bin/python'),'-B',b['lifecycleSourceRef']['path'],'stop-normal',
          '--context-ref',json.dumps(b['contextRef'],sort_keys=True)]
    child=None
    try:
        with (window/'normal-stop-supervisor.stdout').open('xb') as out,(window/'normal-stop-supervisor.stderr').open('xb') as err:
            child=d.popen(argv,cwd=HARNESS,env=dict(os.environ),stdout=out,stderr=err)
            save(window/'normal-stop-supervisor-child.json',{'pid':child.pid,'argv':argv})
            child.wait(timeout=SECONDS)  # Timeout never signals or kills child.
        parent.need(child.returncode==0,'Original normal stop failed; preserve without replay')
        stop_body,stop_ref=parent.read(window/'ROOT_ACTUAL_OWNED_STOP.json');stop=json.loads(stop_body)
        parent.need(stop['state']=='ROOT_VERIFIED_FRESH_OWNED_VM_STOP_WITH_BRIDGE_DISCONNECTED'
                    and stop['ownerRunID']==b['ownerRunID'] and stop['mode']=='normal'
                    and stop['pid']==p['blankBinding']['ownedProcess']['pid']
                    and all(stop[k] is True for k in ('processAndGroupAbsent','port2345Absent','canonicalSocketAbsent'))
                    and stop['guestCleanExitClaimed'] is False,'Actual original own normal-stop receipt required')
        result={'state':'ORIGINAL_NORMAL_STOP_RETURNED_RESTORATION_AND_RELEASE_PENDING',
                'stopReceiptRef':stop_ref,'returncode':0,'signals':0,'guardReleaseClaimed':False,'retryAllowed':False}
        save(window/'normal-stop-supervisor-result.json',result);return result
    except BaseException as error:
        save(window/'normal-stop-supervisor-failed.json',{'state':'NORMAL_STOP_CHILD_OR_TARGET_RETAINED_NO_REPLAY',
            'childPID':child.pid if child is not None else None,'errorType':type(error).__name__,
            'error':str(error)[:2048],'signals':0,'retryAllowed':False,'guardReleaseClaimed':False})
        raise


def main():
    os.umask(0o077);sys.dont_write_bytecode=True
    parent.need(len(sys.argv)==3 and sys.argv[1]=='--binding-ref','Exact new normal-stop binding reference required')
    print(json.dumps(execute(json.loads(sys.argv[2]))))
if __name__=='__main__':main()
