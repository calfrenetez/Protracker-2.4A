"""Fresh source freeze and actual-start binding for the finite pointer test.

No reservation, service restart, emulator launch, input, recovery or release.
The binder's only target operation is one original guarded Guest CPU admission.
"""
import copy
import importlib
import importlib.util
import json
import os
from pathlib import Path
import sys
import time
from types import SimpleNamespace

HARNESS = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
if __name__ == '__main__':
    info = time.get_clock_info('monotonic')
    if not (sys.executable == str(HARNESS / '.venv/bin/python')
            and tuple(sys.version_info[:3]) == (3, 14, 0) and sys.platform == 'darwin'
            and sys.implementation.name == 'cpython' and info.monotonic and not info.adjustable):
        raise RuntimeError('Original harness clock required before binding imports')

import editor_pointer_parent as parent
from editor_capture_volume import profile_append
from editor_probe_namespace import save, directory_ref

PLAN = 'ROOT_REVIEWED_PASSIVE_V2_FRESH_SOURCE_PLAN_NOT_FROZEN'
READONLY_PLAN = 'ROOT_REVIEWED_READONLY_PASSIVE_V2_FRESH_SOURCE_PLAN_NOT_FROZEN'
BIND = 'ROOT_ACCEPTED_PASSIVE_V2_ACTUAL_START_BINDING_ONLY'
READONLY_BIND = 'ROOT_ACCEPTED_READONLY_PASSIVE_V2_ACTUAL_START_BINDING_ONLY'
EXTRA = ('fresh_preservation', 'canonical_profile_session_files', 'service_readiness')
LIFECYCLE = ('editor_pointer_binding', 'editor_pointer_parent', 'editor_pointer_dispatch',
             'editor_probe_stopped', 'editor_capture_settlement', 'editor_capture_namespace')
SAVED_FILE_HELPER = (10249, '8c1fbc9f12b31fd846e37916d2178ab153f34477608c77378438ee43d9e8f15b')


def configure_saved_files(f, sources, preparation_ref, before_ref):
    """Same qualified saved-file functions, with the actual new paired mount.

    Used before the original patch AND restoration; never adopt the helper's
    historical temporary-volume literal as the new profile session.
    """
    parent.need(parent.digest(parent.pinned(sources[str(Path(f.__file__))])) == SAVED_FILE_HELPER
                and f.I == HARNESS and f.ROLES == {
                    'canonicalProfile': HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae',
                    'emulatorState': HARNESS / 'runtime/emulator.json',
                    'bridgeEnabled': HARNESS / 'runtime/Dev/Tools/bridge-enabled'},
                'Unchanged original saved-file helper and three canonical roles required')
    prep = json.loads(parent.pinned(preparation_ref)); before = parent.pinned(before_ref)
    suffix = profile_append(before, str(Path(prep['privateCandidate']['path']).parent))
    parent.need(suffix.decode('ascii') == prep['appendASCII']
                and prep['proposedProfile'] == {'bytes': len(before + suffix), 'sha256': parent.digest(before + suffix)[1]},
                'Actual new paired mount differs from its prepared profile')
    f.APPEND = suffix; f.SESSION = parent.digest(before + suffix)
    return f


def configure_readonly_probe_saved_files(f, sources, preparation_ref, probe_ref, before_ref):
    """Prospective paired-volume configuration, never a patch or admission.

    The new preparation source must be explicitly selected. No existing live
    binding is routed here; a future distinct carrier must admit the ro volume.
    Both patch and restoration must use this same complete suffix.
    """
    path = str(parent.TOOLS / 'editor_probe_volume.py')
    module = checked_module('editor_probe_volume', path, sources)
    prepared = module.verify(probe_ref)
    parent.need(prepared['inputs']['capturePreparation'] == preparation_ref
                and parent.pinned(prepared['inputs']['originalProfile']) == parent.pinned(before_ref),
                'Same actual editor preparation and original profile bytes required')
    capture = json.loads(parent.pinned(preparation_ref)); before = parent.pinned(before_ref)
    suffix = module.profile_append(before, str(Path(capture['privateCandidate']['path']).parent),
                                   prepared['volumePath'])
    parent.need(prepared['appendASCII'] == suffix.decode('ascii')
                and prepared['proposedProfile'] == {'bytes': len(before + suffix),
                    'sha256': parent.digest(before + suffix)[1]},
                'Complete prospective paired-profile preparation differs')
    configure_saved_files(f, sources, preparation_ref, before_ref)
    f.APPEND = suffix; f.SESSION = parent.digest(before + suffix)
    return f


def checked_module(name, path, sources):
    parent.need(path in sources, 'Selected imported source omitted from freeze')
    parent.pinned(sources[path])
    spec = importlib.util.find_spec(name)
    parent.need(spec and spec.origin == path, 'Selected module resolution differs before import')
    module = importlib.import_module(name)
    parent.need(str(Path(module.__file__)) == path, 'Actual selected module differs after import')
    parent.pinned(sources[path])
    return module


def source_shape(record):
    sources, modules = record['sources'], record['modulePaths']
    session = Path(modules['capture_runner']).parent
    window = Path(record['newWindow'])
    parent.need(set(modules) == set(parent.MODULES) and type(sources) is dict and sources
                and all(k == v['path'] for k, v in sources.items()), 'Exact selected source mapping required')
    for path in (session, window):
        parent.need(path.is_absolute() and path.parent == Path('/private/tmp')
                    and path.name.startswith('pt24g-') and '..' not in path.parts,
                    'Fresh direct private session/window required')
    directory_ref(session)
    local = {'capture_runner', 'capture_case_mounted', 'first_mouse_api', 'capture_support', 'root_observation'}
    shared = {'launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'}
    for name, path in modules.items():
        if name in local: expected = session / (name + '.py')
        elif name in shared: expected = HARNESS / 'scripts' / (name + '.py')
        elif name.startswith('amiberry_mcp.'):
            expected = HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name.split('.')[-1] + '.py')
        else: expected = parent.TOOLS / (name + '.py')
        parent.need(path == str(expected) and path in sources, 'Selected canonical module path differs')
    parent.need(record['extraModulePaths'] == {n: str(session / (n + '.py')) for n in EXTRA},
                'Exact original private helper paths required')
    required = set(modules.values()) | set(record['extraModulePaths'].values())
    required |= {str(parent.TOOLS / (n + '.py')) for n in LIFECYCLE}
    required |= {str(session / n) for n in ('SOURCE-PROVENANCE.json', 'publish_mouse_observation.py', 'clock_runtime.py')}
    parent.need(required <= set(sources), 'Complete selected binder/lifecycle/helper sources required')
    for ref in sources.values():
        body = parent.pinned(ref)
        if Path(ref['path']).suffix == '.py': compile(body, ref['path'], 'exec')
    return session, window


def load_extras(record):
    session, _ = source_shape(record)
    sys.path[:0] = [str(session), str(HARNESS / 'scripts'), str(parent.TOOLS)]
    return {n: checked_module(n, record['extraModulePaths'][n], record['sources']) for n in EXTRA}


def freeze_sources(plan, dependencies=None):
    parent.need(plan['state'] == PLAN and plan['acceptedByRoot'] is True,
                'Actual newly reviewed complete source plan required')
    session, window = source_shape(plan)
    parent.need(not os.path.lexists(window) and not os.path.lexists(session / 'SOURCE-FREEZE.json'),
                'Fresh source freeze/window required; no replay')
    d = dependencies if dependencies is not None else load_extras(plan)
    # Original helper takes both original shared locks; readiness still requires
    # the actual new human approval/result and current EMPTY guard/service epoch.
    with d['canonical_profile_session_files'].locks():
        d['service_readiness'].require_human_approved_service(
            ['--service-readiness-ref', json.dumps(plan['serviceReadinessRef'])])
        source_shape(plan)
        record = {'state': parent.FROZEN_STATE, 'sources': plan['sources'],
                  'modulePaths': plan['modulePaths'], 'extraModulePaths': plan['extraModulePaths'],
                  'newWindow': str(window), 'actualApprovedServiceReadinessRef': plan['serviceReadinessRef'],
                  'targetOperations': 0, 'runtimeAdmission': False}
        return save(session / 'SOURCE-FREEZE.json', record)


def readonly_source_shape(record):
    ro = checked_module('editor_readonly_probe', str(parent.TOOLS / 'editor_readonly_probe.py'), record['sources'])
    session, window = ro.source_shape(record)
    sources = record['sources']
    required = {str(parent.TOOLS / (n + '.py')) for n in LIFECYCLE}
    required |= {str(session / n) for n in ('SOURCE-PROVENANCE.json', 'publish_mouse_observation.py', 'clock_runtime.py')}
    required |= {str(parent.TOOLS / (n + '.py')) for n in ('editor_readonly_capture', 'editor_readonly_observation')}
    parent.need(required <= set(sources)
        and parent.digest(parent.pinned(sources[str(session / 'canonical_profile_session_files.py')])) == SAVED_FILE_HELPER,
        'Complete read-only lifecycle and unchanged saved-file helper required')
    for actual, template in (('capture_case_mounted.py', 'editor_readonly_capture.py'),
                             ('publish_mouse_observation.py', 'editor_readonly_observation.py')):
        parent.need(parent.pinned(sources[str(session / actual)]) == parent.pinned(sources[str(parent.TOOLS / template)]),
                    'Fresh read-only capture/publisher must match selected reviewed template')
    return session, window


def load_readonly_extras(record):
    session, _ = readonly_source_shape(record)
    sys.path[:0] = [str(session), str(HARNESS / 'scripts'), str(parent.TOOLS)]
    return {n: checked_module(n, record['extraModulePaths'][n], record['sources']) for n in EXTRA}


def freeze_readonly_sources(plan, dependencies=None):
    """Distinct 23-module source freeze; no runtime binding or target action."""
    parent.need(plan['state'] == READONLY_PLAN and plan['acceptedByRoot'] is True,
                'Actual newly reviewed complete read-only source plan required')
    path = str(parent.TOOLS / 'editor_readonly_probe.py')
    ro = checked_module('editor_readonly_probe', path, plan['sources'])
    session, window = readonly_source_shape(plan)
    prepared = ro.volume.verify(plan['readonlyPreparationRef'])
    parent.need(prepared['preparationSource'] == plan['sources'][str(parent.TOOLS / 'editor_probe_volume.py')]
                and not os.path.lexists(window) and not os.path.lexists(session / 'SOURCE-FREEZE.json'),
                'Same selected input preparation and fresh freeze/window required; no replay')
    d = dependencies if dependencies is not None else {
        n: checked_module(n, plan['extraModulePaths'][n], plan['sources']) for n in EXTRA}
    with d['canonical_profile_session_files'].locks():
        d['service_readiness'].require_human_approved_service(
            ['--service-readiness-ref', json.dumps(plan['serviceReadinessRef'])])
        readonly_source_shape(plan); ro.volume.verify(plan['readonlyPreparationRef'])
        parent.need(not os.path.lexists(window) and not os.path.lexists(session / 'SOURCE-FREEZE.json'),
                    'Read-only freeze/window appeared before publication')
        record = {'state': ro.FROZEN_STATE, 'sources': plan['sources'],
            'modulePaths': plan['modulePaths'], 'extraModulePaths': plan['extraModulePaths'],
            'newWindow': str(window), 'actualApprovedServiceReadinessRef': plan['serviceReadinessRef'],
            'readonlyPreparationRef': plan['readonlyPreparationRef'], 'targetOperations': 0, 'runtimeAdmission': False}
        return save(session / 'SOURCE-FREEZE.json', record)


def validate_actual(record):
    return _validate_actual(record, readonly=False)


def validate_readonly_actual(record):
    return _validate_actual(record, readonly=True)


def _validate_actual(record, *, readonly):
    parent.need(record['state'] == (READONLY_BIND if readonly else BIND) and record['acceptedByRoot'] is True
                and record['allowedActions'] == {'guestCPUAdmissions': 1, 'bindingWrites': 2,
                     'applicationLaunches': 0, 'input': 0, 'serviceChanges': 0, 'guardReleases': 0}
                and record['limits'] == {'whole': 60}, 'Distinct actual-start binder scope required')
    freeze = json.loads(parent.pinned(record['sourceFreezeRef']))
    if readonly:
        ro = checked_module('editor_readonly_probe', str(parent.TOOLS / 'editor_readonly_probe.py'), freeze['sources'])
        parent.need(freeze['state'] == ro.FROZEN_STATE, 'Actual distinct read-only source freeze required')
        session, window = readonly_source_shape(freeze)
    else:
        parent.need(freeze['state'] == parent.FROZEN_STATE, 'Actual new V2 source freeze required')
        session, window = source_shape(freeze)
    directory_ref(window)
    parent.need(record['sourceFreezeRef']['path'] == str(session / 'SOURCE-FREEZE.json')
                and record['windowDirectory'] == str(window), 'Actual same-window freeze required')
    fixed = {'reservationRef': 'reserve-result.json', 'startupResultRef': 'startup-result.json',
             'ownedProcessRef': 'ROOT_CURRENT_STARTED_PROCESS.json',
             'serviceProcessRef': 'ROOT_CURRENT_DEVBENCH_PROCESS.json',
             'nativeViewRef': 'ROOT_NATIVE_WORKBENCH_VIEW.json',
             'originalProfileRef': 'before-canonicalProfile.bin',
             'preservationBeforeRef': 'fresh-preservation-before.json'}
    rows = {}
    for key, leaf in fixed.items():
        parent.need(record[key]['path'] == str(window / leaf), 'Actual new binder endpoint differs')
        body = parent.pinned(record[key]); rows[key] = body if key == 'originalProfileRef' else json.loads(body)
    reserve = rows['reservationRef']; owner = reserve['run_id']; owned = rows['ownedProcessRef']
    parent.need(reserve['state'] == 'FRESH_OWN_RESERVATION_NO_LAUNCH_OR_INPUT'
                and reserve['window'] == str(window) and reserve['target'] == 'amiberry-030'
                and reserve['serviceReadinessRef'] == freeze['actualApprovedServiceReadinessRef'],
                'Actual new reservation/readiness association required')
    ready = json.loads(parent.pinned(reserve['serviceReadinessRef']))
    parent.need(ready['state'] == 'ROOT_ACTUALLY_VERIFIED_HUMAN_APPROVED_DEVBENCH_READY'
                and ready['acceptedByRoot'] is True, 'Actual successful approved service readiness required')
    for key in ('authorizationRef', 'restartIntentRef', 'restartResultRef'): parent.pinned(ready[key])
    parent.need(rows['startupResultRef']['returncode'] == 0
                and type(owned['pid']) is int and owned['pid'] > 1
                and owned['argv'][:2] == [str(owned['pid']), str(owned['pid'])]
                and len(owned['argv']) == 3
                and rows['nativeViewRef']['actuallyViewed'] is True
                and rows['nativeViewRef']['pid'] == owned['pid']
                and rows['nativeViewRef']['applicationVisible'] is False,
                'Actual successful startup and Root-viewed own Workbench required')
    service = rows['serviceProcessRef']
    parent.need(service['pid'] == int(ready['observed']['job']['pid'])
                and service['argv'][0] == str(service['pid']) and len(service['argv']) == 3,
                'Actual startup service must match the approved epoch')
    preparation = json.loads(parent.pinned(record['preparationRef']))
    parent.need(preparation['state'] == 'PRIVATE_LOCAL_CANDIDATE_VOLUME_AND_ADDITIVE_PROFILE_SUFFIX_PREPARED_NOT_ADMITTED_OR_MOUNTED'
                and preparation['targetOperations'] == preparation['canonicalProfileWrites'] == 0,
                'Original fresh local-volume preparation required')
    before = rows['originalProfileRef']; suffix = profile_append(before, str(Path(preparation['privateCandidate']['path']).parent))
    if readonly:
        parent.need(record['readonlyPreparationRef'] == freeze['readonlyPreparationRef'], 'Same frozen read-only preparation required')
        prepared = ro.volume.verify(record['readonlyPreparationRef'])
        parent.need(record['readonlyPreparationRef'] == freeze['readonlyPreparationRef']
            and prepared['inputs']['capturePreparation'] == record['preparationRef']
            and parent.pinned(prepared['inputs']['originalProfile']) == before
            and prepared['inputs']['nativeCandidate'] == record['probeNativeRef']
            and prepared['inputs']['nativeSource'] == record['probeSourceRef']
            and prepared['inputs']['compileReceipt'] == record['probeCompileRef'],
            'Exact frozen read-only preparation and actual-start associations required')
        suffix = prepared['appendASCII'].encode('ascii')
    active = parent.pinned(record['activeProfileRef'])
    profile_preparation = prepared if readonly else preparation
    parent.need(record['activeProfileRef']['path'] == str(HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae')
                and active == before + suffix and profile_preparation['appendASCII'] == suffix.decode('ascii')
                and profile_preparation['proposedProfile'] == {'bytes': len(active), 'sha256': parent.digest(active)[1]},
                'Exact new additive profile association required')
    for leaf in ('mounted-guest-admission-intent.json', 'capture-binding.json', 'mouse-parent-binding.json'):
        parent.need(not os.path.lexists(window / leaf), 'Never replay actual Guest admission or binding')
    return freeze, rows, ready, preparation, owner


def execute(record, dependencies=None):
    return _execute(record, dependencies, readonly=False)


def execute_readonly(record, dependencies=None):
    return _execute(record, dependencies, readonly=True)


def _execute(record, dependencies, *, readonly):
    freeze, rows, ready, prep, owner = _validate_actual(record, readonly=readonly)
    session, window = readonly_source_shape(freeze) if readonly else source_shape(freeze)
    if readonly:
        ro = checked_module('editor_readonly_probe', str(parent.TOOLS / 'editor_readonly_probe.py'), freeze['sources'])
    if dependencies is None:
        extras = load_readonly_extras(freeze) if readonly else load_extras(freeze)
        modules = {n: checked_module(n, p, freeze['sources']) for n, p in freeze['modulePaths'].items()}
        dependencies = SimpleNamespace(extras=extras, guard=modules['launcher_guard'].LauncherGuard,
            lease_input=modules['launcher_access'].lease_input, operation=modules['launcher_access'].operation,
            original_api=modules['capture_runner'].OriginalAPI, guest=modules['shared_guest'].Guest,
            blank_scope=modules['capture_case_mounted'])
    d = dependencies; deadline = time.monotonic() + 60
    parent.need('AMIGA_LAUNCH_INFLIGHT' not in os.environ, 'Binder cannot borrow an inherited FRAME')
    lease = d.lease_input(HARNESS, 'amiberry-030'); parent.need(lease['run_id'] == owner, 'Original current lease differs')
    guard = d.guard(HARNESS)
    blank = {'state': 'ROOT_ADMITTED_LOCAL_VOLUME_EDITOR_CAPTURE_ONLY', 'acceptedByRoot': True,
        'target': 'amiberry-030', 'ownerRunID': owner, 'allowedActions': d.blank_scope.ALLOWED,
        'limits': d.blank_scope.LIMITS, 'expectedHistoryBytes': d.blank_scope.HISTORY_BYTES,
        'candidate': prep['originalCandidate'], 'mountedCandidate': prep['privateCandidate'],
        'guestCandidate': prep['guestCandidate'], 'localVolumePreparation': record['preparationRef'],
        'originalProfileBeforeimage': record['originalProfileRef'], 'activeProfile': record['activeProfileRef'],
        'ownedProcess': rows['ownedProcessRef'], 'outputDirectory': str(window / record['runLabel']),
        'sourcePins': freeze['sources'], 'modulePaths': freeze['modulePaths'],
        'nativeWorkbenchView': record['nativeViewRef'], 'serviceReadinessRef': freeze['actualApprovedServiceReadinessRef'],
        'wholeApplicationABIQualified': False, 'legacyReplayPlaybackAdmitted': False, 'nativeCleanExitAdmitted': False}
    probe = {'ownerRunID': owner, 'windowDirectory': str(window), 'sourcePins': freeze['sources'],
        'editorCandidate': prep['privateCandidate'], 'nativeSource': record['probeSourceRef'],
        'nativeCandidate': record['probeNativeRef'], 'compileReceipt': record['probeCompileRef'],
        'guestCandidate': 'Dev:Tests/pt24g-editor-pointer-' + owner + '/PTEditorPointerV2'}
    binding = dict(blank, state=parent.STAGE_PLAN_STATE, allowedActions=copy.deepcopy(parent.STAGE_PLAN_ALLOWED),
        limits=copy.deepcopy(parent.STAGE_PLAN_LIMITS), nativeSize=[640, 512], blankBinding=blank,
        passiveProbe=probe, sourceFreezeRef=record['sourceFreezeRef'],
        nativeGeometryProofRef=record['geometryProofRef'], runLabel=record['runLabel'])
    if readonly:
        prepared = ro.volume.verify(record['readonlyPreparationRef'])
        probe.update(state=ro.INPUT_STATE, acceptedByRoot=True, target='amiberry-030',
            allowedActions=dict(ro.ALLOWED), limits=dict(ro.LIMITS), sourceFreezeRef=record['sourceFreezeRef'],
            readonlyPreparationRef=record['readonlyPreparationRef'], capturePreparationRef=record['preparationRef'],
            originalProfileBeforeimage=record['originalProfileRef'], activeProfile=record['activeProfileRef'],
            runLabel=record['runLabel'], stagedCandidate=prepared['candidate'], guestCandidate='PTProbe:PTEditorPointerV2')
        blank.update(state=ro.BLANK_STATE, readonlyProbe=probe)
        binding.update(state=ro.PARENT_STATE, allowedActions=copy.deepcopy(parent.ALLOWED),
                       limits=copy.deepcopy(parent.LIMITS))
    def validate_binding():
        return parent.validate_binding(binding, readonly=True) if readonly else parent.validate_binding(binding, stage_plan=True)
    with parent.command_lock(HARNESS / 'runtime/test.lock'):
        validate_binding()
        state = guard.status()
        parent.need(state['phase'] == 'READY' and not state['holds'] and state['inflight'] is None,
                    'Current original normal owned session required')
        with guard._locked() as (_, raw): guard._owner(raw, lease, 'amiberry-030')
        d.extras['service_readiness'].validate_records(ready,
            json.loads(parent.pinned(ready['authorizationRef'])), json.loads(parent.pinned(ready['restartIntentRef'])),
            json.loads(parent.pinned(ready['restartResultRef'])), ready['observed'])
        blank['preservation'] = d.extras['fresh_preservation'].verify_current(rows['preservationBeforeRef'])
        save(window / 'mounted-guest-admission-intent.json', {'state': 'ONE_ORIGINAL_GUEST_CPU_ADMISSION_PASSIVE_V2',
             'ownerRunID': owner, 'sourceFreezeRef': record['sourceFreezeRef'], 'retryAllowed': False})
        with d.operation(HARNESS, 'amiberry-030', 'Original Guest CPU admission for fresh passive V2 editor case') as frame:
            api = d.original_api(blank, frame); api.owner(owner)
            # Current VM/target/socket identity comes from the original API;
            # the selected service identity is checked independently.
            from editor_probe_stopped import HostObservations
            host = getattr(d, 'observations', None) or HostObservations()
            service = rows['serviceProcessRef']
            parent.need(host.command(['/bin/ps', '-p', str(service['pid']), '-o', 'pid=,pgid=,command='], (0,)).strip().split(None, 2)
                        == service['argv'], 'Current approved service identity changed')
            parent.need(time.monotonic() < deadline, 'Actual binder deadline exhausted before Guest')
            d.guest(HARNESS, window); api.owner(owner)
            parent.need(time.monotonic() < deadline, 'Actual binder deadline exhausted after Guest')
        validate_binding()
        parent.need(time.monotonic() < deadline, 'Actual binder deadline exhausted before publishing bindings')
        save(window / 'capture-binding.json', blank)
        return save(window / 'mouse-parent-binding.json', binding)


def main():
    os.umask(0o077); sys.dont_write_bytecode = True
    parent.need(len(sys.argv) == 3 and sys.argv[1] in
                ('--freeze-plan-ref', '--freeze-readonly-plan-ref', '--actual-binding-ref', '--actual-readonly-binding-ref'),
                'Actual newly reviewed freeze or start-binding record required')
    ref = json.loads(sys.argv[2]); record = json.loads(parent.pinned(ref))
    if sys.argv[1] == '--freeze-readonly-plan-ref': result = freeze_readonly_sources(record)
    elif sys.argv[1] == '--freeze-plan-ref': result = freeze_sources(record)
    elif sys.argv[1] == '--actual-readonly-binding-ref': result = execute_readonly(record)
    else: result = execute(record)
    print(json.dumps({'ref': result, 'applicationLaunches': 0, 'lifecyclePending': True}))


if __name__ == '__main__': main()
