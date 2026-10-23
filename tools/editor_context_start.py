"""Distinct V3 freeze and actual-start binding; no reserve/start/restart/release.

One original guarded Guest CPU admission after actual successful own startup.
Never replays a failed admission or constructs human authorization from records.
"""
import copy
import json
import os
from pathlib import Path
import sys
import time

if __name__ == '__main__':
    harness_python = '/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra/.venv/bin/python'
    clock = time.get_clock_info('monotonic')
    if not (sys.executable == harness_python and tuple(sys.version_info[:3]) == (3, 14, 0)
            and sys.platform == 'darwin' and sys.implementation.name == 'cpython'
            and clock.monotonic and not clock.adjustable):
        raise RuntimeError('Original harness clock required before V3 imports')

import editor_pointer_parent as original
import editor_context_binding as bindings
import editor_context_probe as probe
import editor_context_runtime as runtime
import editor_context_volume as volume
from editor_probe_namespace import save, directory_ref

PLAN = 'ROOT_REVIEWED_CONTEXT_V3_FRESH_SOURCE_PLAN_NOT_FROZEN'
STATE = 'ROOT_ACCEPTED_CONTEXT_V3_ACTUAL_START_BINDING_ONLY'
ALLOWED = {'guestCPUAdmissions': 1, 'bindingWrites': 2, 'applicationLaunches': 0,
           'input': 0, 'serviceChanges': 0, 'guardReleases': 0}
LIMITS = {'whole': 60}


def freeze_sources(plan, dependencies=None):
    original.need(plan['state'] == PLAN and plan['acceptedByRoot'] is True,
                  'New explicitly reviewed V3 source plan required')
    session, window = bindings.source_shape(plan, window_must_exist=False)
    prepared = volume.verify(plan['contextPreparationRef'])
    original.need(prepared['preparationSource'] == plan['sources'][str(Path(volume.__file__))]
                  and not os.path.lexists(window) and not os.path.lexists(session / 'SOURCE-FREEZE.json'),
                  'Same selected V3 preparation and absent fresh freeze/window required')
    d = dependencies if dependencies is not None else bindings.load_extras(plan, window_must_exist=False)
    with d['canonical_profile_session_files'].locks():
        d['service_readiness'].require_human_approved_service(
            ['--service-readiness-ref', json.dumps(plan['serviceReadinessRef'])])
        bindings.source_shape(plan, window_must_exist=False); volume.verify(plan['contextPreparationRef'])
        original.need(not os.path.lexists(window) and not os.path.lexists(session / 'SOURCE-FREEZE.json'),
                      'V3 freeze/window appeared before publication')
        return save(session / 'SOURCE-FREEZE.json', {
            'state': probe.FROZEN_STATE, 'sources': plan['sources'], 'modulePaths': plan['modulePaths'],
            'extraModulePaths': plan['extraModulePaths'], 'contextModulePaths': plan['contextModulePaths'],
            'newWindow': str(window), 'contextPreparationRef': plan['contextPreparationRef'],
            'actualApprovedServiceReadinessRef': plan['serviceReadinessRef'],
            'targetOperations': 0, 'runtimeAdmission': False})


def validate_actual(record):
    r = record
    original.need(r['state'] == STATE and r['acceptedByRoot'] is True
                  and r['allowedActions'] == ALLOWED and r['limits'] == LIMITS,
                  'Distinct V3 actual-start binder scope required')
    freeze = json.loads(original.pinned(r['sourceFreezeRef']))
    original.need(freeze['state'] == probe.FROZEN_STATE, 'Actual distinct V3 source freeze required')
    session, window = bindings.source_shape(freeze); directory_ref(window)
    original.need(r['sourceFreezeRef']['path'] == str(session / 'SOURCE-FREEZE.json')
                  and r['windowDirectory'] == str(window), 'Actual same-window V3 source freeze required')
    fixed = {'reservationRef': 'reserve-result.json', 'startupResultRef': 'startup-result.json',
             'ownedProcessRef': 'ROOT_CURRENT_STARTED_PROCESS.json',
             'serviceProcessRef': 'ROOT_CURRENT_DEVBENCH_PROCESS.json',
             'nativeViewRef': 'ROOT_NATIVE_WORKBENCH_VIEW.json',
             'originalProfileRef': 'before-canonicalProfile.bin',
             'preservationBeforeRef': 'fresh-preservation-before.json'}
    rows = {}
    for key, leaf in fixed.items():
        original.need(r[key]['path'] == str(window / leaf), 'Actual new V3 binder endpoint differs')
        raw = original.pinned(r[key]); rows[key] = raw if key == 'originalProfileRef' else json.loads(raw)
    reserve = rows['reservationRef']; owner = reserve['run_id']; owned = rows['ownedProcessRef']
    original.need(reserve['state'] == 'FRESH_OWN_RESERVATION_NO_LAUNCH_OR_INPUT'
                  and reserve['window'] == str(window) and reserve['target'] == 'amiberry-030'
                  and reserve['serviceReadinessRef'] == freeze['actualApprovedServiceReadinessRef'],
                  'Actual new V3 reservation/readiness association required')
    ready = json.loads(original.pinned(reserve['serviceReadinessRef']))
    original.need(ready['state'] == 'ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY'
                  and ready['acceptedByRoot'] is True, 'Actual successful approved service readiness required')
    for key in ('authorizationRef', 'restartIntentRef', 'restartResultRef'): original.pinned(ready[key])
    view = rows['nativeViewRef']; service = rows['serviceProcessRef']
    original.need(rows['startupResultRef']['returncode'] == 0 and type(owned['pid']) is int and owned['pid'] > 1
                  and type(owned['argv']) is list and len(owned['argv']) == 3
                  and owned['argv'][:2] == [str(owned['pid']), str(owned['pid'])]
                  and type(owned['socket_inode']) is int and owned['socket_inode'] > 0
                  and view['actuallyViewed'] is True and view['pid'] == owned['pid']
                  and view['applicationVisible'] is False,
                  'Actual successful own startup/socket and Root-viewed Workbench required')
    original.need(type(service['pid']) is int and service['pid'] > 1
                  and service['pid'] == int(ready['observed']['job']['pid'])
                  and type(service['argv']) is list and len(service['argv']) == 3
                  and service['argv'][0] == str(service['pid']), 'Actual service must match approved epoch')
    original.recent_line(r['runLabel'])
    prepared = volume.verify(r['contextPreparationRef']); refs = prepared['inputs']
    original.need(r['contextPreparationRef'] == freeze['contextPreparationRef']
                  and refs['capturePreparation'] == r['preparationRef']
                  and original.pinned(refs['originalProfile']) == rows['originalProfileRef']
                  and all(refs[k] == r[rk] for k, rk in
                          (('nativeSource', 'probeSourceRef'), ('nativeCandidate', 'probeNativeRef'),
                           ('compileReceipt', 'probeCompileRef'))), 'Exact frozen V3 compile/input/start association required')
    before = rows['originalProfileRef']; active = original.pinned(r['activeProfileRef'])
    original.need(r['activeProfileRef']['path']
                  == str(original.HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae')
                  and active == before + prepared['appendASCII'].encode('ascii')
                  and prepared['proposedProfile'] == {'bytes': len(active), 'sha256': original.digest(active)[1]},
                  'Exact complete new V3 paired profile required')
    for leaf in ('mounted-guest-admission-intent.json', 'capture-binding.json', 'context-parent-binding.json'):
        original.need(not os.path.lexists(window / leaf), 'Never replay actual V3 Guest admission or binding')
    return freeze, rows, ready, json.loads(original.pinned(r['preparationRef'])), owner


def execute(record, dependencies=None):
    freeze, rows, ready, prep, owner = validate_actual(record)
    _, window = bindings.source_shape(freeze)
    d = dependencies if dependencies is not None else runtime.load(freeze)
    deadline = time.monotonic() + LIMITS['whole']
    original.need('AMIGA_LAUNCH_INFLIGHT' not in os.environ, 'V3 binder cannot borrow an inherited FRAME')
    lease = d.lease_input(original.HARNESS, 'amiberry-030')
    original.need(lease['run_id'] == owner, 'Original current V3 lease differs')
    guard = d.guard(original.HARNESS)
    prepared = volume.verify(record['contextPreparationRef'])
    p = {'state': probe.INPUT_STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
         'ownerRunID': owner, 'windowDirectory': str(window), 'sourcePins': freeze['sources'],
         'sourceFreezeRef': record['sourceFreezeRef'], 'allowedActions': dict(probe.ALLOWED), 'limits': dict(probe.LIMITS),
         'contextPreparationRef': record['contextPreparationRef'], 'capturePreparationRef': record['preparationRef'],
         'nativeSource': record['probeSourceRef'], 'nativeCandidate': record['probeNativeRef'],
         'compileReceipt': record['probeCompileRef'], 'stagedCandidate': prepared['candidate'],
         'guestCandidate': 'PTContext:PTEditorPointerContextV3', 'editorCandidate': prep['privateCandidate'],
         'originalProfileBeforeimage': record['originalProfileRef'], 'activeProfile': record['activeProfileRef'],
         'runLabel': record['runLabel']}
    c = {'state': bindings.blank.BLANK_STATE, 'acceptedByRoot': True, 'target': 'amiberry-030',
         'ownerRunID': owner, 'sourcePins': freeze['sources'], 'modulePaths': freeze['modulePaths'],
         'allowedActions': dict(bindings.blank.ALLOWED), 'limits': dict(bindings.blank.LIMITS),
         'expectedHistoryBytes': bindings.blank.HISTORY_BYTES, 'candidate': prep['originalCandidate'],
         'mountedCandidate': prep['privateCandidate'], 'guestCandidate': prep['guestCandidate'],
         'localVolumePreparation': record['preparationRef'], 'originalProfileBeforeimage': record['originalProfileRef'],
         'activeProfile': record['activeProfileRef'], 'readonlyProbe': p,
         'ownedProcess': rows['ownedProcessRef'], 'outputDirectory': str(window / record['runLabel'])}
    b = {'state': bindings.STATE, 'acceptedByRoot': True, 'target': 'amiberry-030', 'ownerRunID': owner,
         'sourcePins': freeze['sources'], 'sourceFreezeRef': record['sourceFreezeRef'],
         'allowedActions': copy.deepcopy(bindings.ALLOWED), 'limits': copy.deepcopy(bindings.LIMITS),
         'mountedCandidate': prep['privateCandidate'], 'blankBinding': c, 'passiveProbe': p,
         'runLabel': record['runLabel'], 'outputDirectory': c['outputDirectory']}
    def current():
        original.need(time.monotonic() < deadline, 'V3 binder deadline exhausted; no replay')
        bindings.validate(b); validate_actual(record)
        status = guard.status()
        original.need(status['phase'] == 'READY' and not status['holds'] and status['inflight'] is None,
                      'Current original normal V3 owner required')
        with guard._locked() as (_, state): guard._owner(state, lease, 'amiberry-030')
        d.current_service(b)
        original.need(time.monotonic() < deadline, 'V3 binder deadline exhausted during checks')
    with original.command_lock(original.HARNESS / 'runtime/test.lock'):
        current()
        c['preservation'] = d.extras['fresh_preservation'].verify_current(rows['preservationBeforeRef'])
        save(window / 'mounted-guest-admission-intent.json', {
            'state': 'ONE_ORIGINAL_GUEST_CPU_ADMISSION_CONTEXT_V3', 'ownerRunID': owner,
            'sourceFreezeRef': record['sourceFreezeRef'], 'retryAllowed': False})
        d.install_client(); d.install_ipc(original.HARNESS)
        with d.operation(original.HARNESS, 'amiberry-030', 'One original Guest CPU admission for V3 diagnostic') as frame:
            api = d.original_api(c, frame); api.owner(owner); d.current_service(b)
            original.need(time.monotonic() < deadline, 'V3 binder deadline exhausted before Guest')
            d.guest(original.HARNESS, window); api.owner(owner); d.current_service(b)
            probe.require_current_mounted_input(p, api)
            original.need(time.monotonic() < deadline, 'V3 binder deadline exhausted after Guest')
        bindings.validate(b)
        original.need(original.pinned(p['activeProfile'])
                      == original.pinned(p['originalProfileBeforeimage']) + prepared['appendASCII'].encode('ascii'),
                      'Complete V3 profile changed before binding publication')
        original.need(time.monotonic() < deadline, 'V3 binder deadline exhausted before publication')
        save(window / 'capture-binding.json', c)
        return save(window / 'context-parent-binding.json', b)


def main():
    original.require_harness_runtime()
    os.umask(0o077); sys.dont_write_bytecode = True
    original.need(len(sys.argv) == 3 and sys.argv[1] in ('--freeze-plan-ref', '--actual-binding-ref'),
                  'Actual newly reviewed V3 plan/start-binding reference required')
    ref = json.loads(sys.argv[2]); record = json.loads(original.pinned(ref))
    result = freeze_sources(record) if sys.argv[1] == '--freeze-plan-ref' else execute(record)
    print(json.dumps({'ref': result, 'applicationLaunches': 0, 'lifecyclePending': True}))


if __name__ == '__main__': main()
