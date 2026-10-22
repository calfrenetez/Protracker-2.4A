"""Prospective executable parent for one newly admitted editor/probe case.

Source preparation alone cannot create an admission, lease or service epoch.
An explicit stage-plan scope permits one guarded probe stage. No restart,
reservation, recovery, cleanup or release is implemented here.
"""
import asyncio
import copy
from contextlib import contextmanager
import fcntl
import hashlib
import importlib
import importlib.util
import json
import os
from pathlib import Path
import stat
import sys
import time
from types import SimpleNamespace

HARNESS = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
TOOLS = Path(__file__).resolve().parent
FACTS = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid',
         'st_size', 'st_mtime_ns', 'st_ctime_ns', 'st_flags')
FROZEN_STATE = 'ROOT_FROZEN_EDITOR_PASSIVE_V2_SESSION_SOURCE_NOT_EXECUTED'
STAGE_PLAN_STATE = 'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2_STAGE_PLAN'
READONLY_STATE = 'ROOT_ADMITTED_EDITOR_MOUSE_WITH_READONLY_PASSIVE_V2'
MODULES = ('capture_runner', 'capture_case_mounted', 'first_mouse_api',
           'launcher_guard', 'launcher_access', 'amiberry_mcp.gui_automation',
           'editor_mouse_probe_case', 'editor_pointer_probe', 'editor_pointer_snapshot',
           'capture_support', 'root_observation', 'bridge_checks', 'shared_guest',
           'shared_infra_render_files', 'amiberry_mcp.ipc_client', 'amiberry_mcp.shared_state',
           'editor_blank_capture_protocol', 'editor_capture_volume',
           'editor_mouse_capture_protocol', 'emulator', 'editor_probe_namespace')


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def require_harness_runtime():
    """Same clock domain as the preserved parent/publisher; before target imports."""
    clock = time.get_clock_info('monotonic')
    need(sys.executable == str(HARNESS / '.venv/bin/python')
         and tuple(sys.version_info[:3]) == (3, 14, 0)
         and sys.implementation.name == 'cpython' and sys.platform == 'darwin'
         and clock.monotonic is True and clock.adjustable is False,
         'Original harness Python3.14 clock domain required before target imports')


if __name__ == '__main__':
    # Keep the precondition ahead of even pure app-protocol imports: Python3.9
    # cannot evaluate their modern annotations and must refuse here first.
    require_harness_runtime()

from editor_mouse_probe_case import ALLOWED, LIMITS
from editor_pointer_probe import NATIVE, SOURCE, EDITOR, digest
from editor_blank_capture_protocol import recent_line

STAGE_PLAN_ALLOWED = dict(ALLOWED, hostProbeFileWrites=1)
STAGE_PLAN_LIMITS = dict(LIMITS, probeStage=60)


def read(path, maximum=2097152):
    """Original bounded resident-file/full-facts discipline for the bootstrap."""
    path = Path(path)
    before = path.lstat()
    need(path.is_absolute() and '..' not in path.parts and stat.S_ISREG(before.st_mode)
         and before.st_nlink == 1 and before.st_size <= maximum
         and not getattr(before, 'st_flags', 0) & 1073741824,
         'Resident ordinary bounded input required')
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        opened = os.fstat(fd)
        body = bytearray()
        while len(body) <= maximum:
            part = os.read(fd, min(65536, maximum + 1 - len(body)))
            if not part:
                break
            body.extend(part)
        after = os.fstat(fd)
    finally:
        os.close(fd)
    facts = lambda st: {k: getattr(st, k, 0) for k in FACTS}
    need(facts(before) == facts(opened) == facts(after) == facts(path.lstat())
         and len(body) == after.st_size, 'Input changed during bounded read')
    body = bytes(body)
    return body, {'path': str(path), 'bytes': len(body),
                  'sha256': hashlib.sha256(body).hexdigest(), 'fullFacts': facts(after)}


def pinned(ref):
    need(type(ref) is dict and set(ref) == {'path', 'bytes', 'sha256', 'fullFacts'},
         'Exact actual file reference required')
    body, actual = read(ref['path'])
    need(actual == ref, 'Actual pinned input changed')
    return body


@contextmanager
def command_lock(path):
    """Existing nonblocking original command lock; never creates or bypasses one."""
    path = Path(path)
    before = path.lstat()
    identity = lambda st: tuple(getattr(st, k) for k in
                               ('st_dev', 'st_ino', 'st_mode', 'st_uid', 'st_gid', 'st_nlink', 'st_size'))
    need(stat.S_ISREG(before.st_mode) and before.st_uid == os.getuid()
         and before.st_nlink == 1 and before.st_size == 0,
         'Safe existing original command lock required')
    fd = os.open(path, os.O_RDWR | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        need(identity(before) == identity(os.fstat(fd)) == identity(path.lstat()),
             'Original command lock identity changed')
        yield fd
    finally:
        os.close(fd)


def validate_binding(binding, *, pin=pinned, stage_plan=False, readonly=False):
    """Read-only validation; no imported harness, guard or target operation."""
    b = binding
    need(not (readonly and stage_plan), 'Read-only parent cannot stage a Dev probe')
    if readonly:
        import editor_readonly_probe as ro
    need(b['state'] == (READONLY_STATE if readonly else STAGE_PLAN_STATE if stage_plan else 'ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2')
         and b['acceptedByRoot'] is True and b['target'] == 'amiberry-030'
         and b['allowedActions'] == (STAGE_PLAN_ALLOWED if stage_plan else ALLOWED)
         and b['limits'] == (STAGE_PLAN_LIMITS if stage_plan else LIMITS)
         and b['nativeSize'] == [640, 512], 'Distinct exact parent admission required')
    freeze = json.loads(pin(b['sourceFreezeRef']))
    need(freeze['state'] == (ro.FROZEN_STATE if readonly else FROZEN_STATE) and freeze['sources'] == b['sourcePins']
         and freeze['modulePaths'] == b['blankBinding']['modulePaths']
         and set(freeze['modulePaths']) == set(ro.MODULES if readonly else MODULES),
         'Actual distinct source freeze association required')
    sources = b['sourcePins']
    need(bool(sources) and all(k == v['path'] for k, v in sources.items()),
         'Actual source paths must key their exact references')
    for source in sources.values():
        pin(source)
    need(str(Path(__file__)) in sources, 'This actual parent must be frozen')
    for name in MODULES:
        need(freeze['modulePaths'][name] in sources, 'Every selected imported module must be pinned')
    for name in ('editor_mouse_probe_case', 'editor_pointer_probe', 'editor_pointer_snapshot',
                 'editor_blank_capture_protocol', 'editor_capture_volume',
                 'editor_mouse_capture_protocol', 'shared_infra_render_files', 'editor_probe_namespace'):
        need(freeze['modulePaths'][name] == str(TOOLS / (name + '.py')),
             'Repository-owned composed module path differs')
    session = Path(freeze['modulePaths']['capture_runner']).parent
    need(session.is_absolute() and '..' not in session.parts
         and str(session).startswith('/private/tmp/pt24g-'), 'Fresh private source session required')
    for name in ('capture_runner', 'capture_case_mounted', 'first_mouse_api',
                 'capture_support', 'root_observation'):
        need(freeze['modulePaths'][name] == str(session / (name + '.py')),
             'Original carrier modules must share one frozen private session')
    for name in ('launcher_guard', 'launcher_access', 'bridge_checks', 'shared_guest', 'emulator'):
        need(freeze['modulePaths'][name] == str(HARNESS / 'scripts' / (name + '.py')),
             'Original shared guard/bridge module path differs')
    for name in ('gui_automation', 'ipc_client', 'shared_state'):
        need(freeze['modulePaths']['amiberry_mcp.' + name]
             == str(HARNESS / '.venv/lib/python3.14/site-packages/amiberry_mcp' / (name + '.py')),
             'Original installed GUI module path differs')
    proof = json.loads(pin(b['nativeGeometryProofRef']))
    need(proof['state'] == 'ARCHIVED_ACTUAL_BUILD_GEOMETRY_SOURCE_REVIEW_ONLY'
         and proof['nativeCellCenter'] == [218, 424] and proof['expectedRow'] == 4
         and proof['expectedChannel'] == 1 and proof['expectedField'] == 0
         and proof['clickDispatchUsesUnchangedIntuiMessageCoordinates'] is True
         and proof['candidate'] == b['blankBinding']['candidate'],
         'Exact candidate hit-test source proof required')
    for source in proof['nativeSourceCopies'].values():
        pin(source['newCopy'])
    blank = b['blankBinding']
    need(blank['state'] == (ro.BLANK_STATE if readonly else 'ROOT_ADMITTED_LOCAL_VOLUME_EDITOR_CAPTURE_ONLY')
         and blank['ownerRunID'] == b['ownerRunID'] and blank['sourcePins'] == sources
         and blank['mountedCandidate'] == b['mountedCandidate']
         and blank['outputDirectory'] == b['outputDirectory']
         and Path(blank['outputDirectory']).name == b['runLabel'],
         'Original blank subphase must share exact owner, sources, candidate and output')
    out = Path(b['outputDirectory'])
    recent_line(b['runLabel'])
    need(out.is_absolute() and '..' not in out.parts
         and str(out.parent).startswith('/private/tmp/pt24g-')
         and str(out.parent) == freeze['newWindow']
         and not out.exists() and not out.is_symlink(),
         'Fresh exact private case output required; never replay')
    probe = b['passiveProbe']
    need(probe['ownerRunID'] == b['ownerRunID'] and probe['sourcePins'] == sources
         and probe['editorCandidate'] == b['mountedCandidate']
         and probe['windowDirectory'] == str(out.parent),
         'Probe must share exact original owner/source/editor pins')
    need(digest(pin(blank['candidate'])) == EDITOR
         and digest(pin(b['mountedCandidate'])) == EDITOR, 'Exact current editor bytes required')
    pin(blank['activeProfile'])
    need(digest(pin(probe['nativeSource'])) == SOURCE
         and digest(pin(probe['nativeCandidate'])) == NATIVE,
         'Exact V2 source/native/staged bytes required before application launch')
    if not stage_plan:
        need(pin(probe['stagedCandidate']) == pin(probe['nativeCandidate']),
             'Actual staged candidate differs from exact native bytes')
    receipt = json.loads(pin(probe['compileReceipt']))
    need(receipt['state'] == 'PASSIVE_EDITOR_V2_SOURCE_AND_NATIVE_COMPILE_HOST_CHECKED_NOT_EXECUTED'
         and receipt['binary'] == {'bytes': NATIVE[0], 'sha256': NATIVE[1]}
         and receipt['sourcePinsEqualBeforeAfter']['tools/diagnostics/native_editor_pointer_snapshot.c']
         == {'bytes': SOURCE[0], 'sha256': SOURCE[1]}, 'Actual V2 compile association required')
    need(type(b['ownerRunID']) is str and b['ownerRunID']
         and b['ownerRunID'][0].isascii() and b['ownerRunID'][0].isalnum()
         and all(c.isascii() and (c.isalnum() or c in '_-') for c in b['ownerRunID'])
         and len(b['ownerRunID']) <= 96, 'Safe original probe owner required')
    if readonly:
        from editor_pointer_binding import readonly_source_shape
        readonly_source_shape(freeze)
        prepared = ro.prepared_input(probe, pin=pin)
        active = pin(probe['activeProfile'])
        need(active == pin(probe['originalProfileBeforeimage']) + prepared['appendASCII'].encode('ascii')
             and probe['activeProfile']['path'] == str(HARNESS / 'runtime/amiberry/Configurations/A1200-030-DEV.uae'),
             'Complete actual paired profile required before parent admission')
        need(blank['readonlyProbe'] == probe and probe['runLabel'] == b['runLabel']
             and probe['sourceFreezeRef'] == b['sourceFreezeRef']
             and probe['originalProfileBeforeimage'] == blank['originalProfileBeforeimage']
             and probe['activeProfile'] == blank['activeProfile']
             and probe['capturePreparationRef'] == blank['localVolumePreparation'],
             'Same paired-profile child must protect blank and mouse phases')
        return freeze
    directory = 'pt24g-editor-pointer-' + b['ownerRunID']
    need(probe['guestCandidate'] == 'Dev:Tests/' + directory + '/PTEditorPointerV2',
         'Separate exact owned passive probe namespace required')
    if stage_plan:
        need('namespaceStageRef' not in probe and 'stagedCandidate' not in probe
             and not os.path.lexists(HARNESS / 'runtime/Dev/Tests' / directory),
             'Stage plan must not adopt an existing or guessed namespace endpoint')
    else:
        need(probe['stagedCandidate']['path']
             == str(HARNESS / 'runtime/Dev/Tests' / directory / 'PTEditorPointerV2'),
             'Actual owned staged endpoint path differs')
        from editor_probe_namespace import validate_stage
        staged = validate_stage(probe, probe['namespaceStageRef'])
        need(staged['stagedCandidate'] == probe['stagedCandidate'],
             'Actual owned stage receipt must bind the parent probe endpoint')
    return freeze


def load_original_modules(binding, freeze):
    """Only after all selected module bodies are pinned; imports do not admit a target."""
    session = Path(freeze['modulePaths']['capture_runner']).parent
    need(session.is_absolute() and '..' not in session.parts
         and str(session).startswith('/private/tmp/pt24g-'), 'Fresh private source session required')
    sys.path[:0] = [str(session), str(HARNESS / 'scripts'), str(TOOLS)]
    modules = {}
    for name, path in freeze['modulePaths'].items():
        need(path in binding['sourcePins'], 'Imported source not frozen')
        pinned(binding['sourcePins'][path])
        spec = importlib.util.find_spec(name)
        need(spec is not None and spec.origin == path, 'Module resolution differs before import')
        module = importlib.import_module(name)
        need(str(Path(module.__file__)) == path, 'Actual imported module path differs')
        pinned(binding['sourcePins'][path])
        modules[name] = module
    blank = modules['capture_case_mounted']
    need(binding['blankBinding']['allowedActions'] == blank.ALLOWED
         and binding['blankBinding']['limits'] == blank.LIMITS,
         'Original blank phase action scope differs')
    return SimpleNamespace(
        guard=modules['launcher_guard'].LauncherGuard,
        lease_input=modules['launcher_access'].lease_input,
        install_client=modules['launcher_access'].install_client,
        install_ipc=modules['launcher_access'].install_ipc,
        operation=modules['launcher_access'].operation,
        original_api=modules['capture_runner'].OriginalAPI,
        blank_case=blank.run_case,
        mouse_api=modules['first_mouse_api'].FirstMouseAPI,
        move_request=modules['amiberry_mcp.gui_automation'].MoveRequest,
        click_request=modules['amiberry_mcp.gui_automation'].ClickRequest,
        composed_case=(modules['editor_mouse_probe_case'].run_readonly_editor_mouse_probe
            if binding['state'] == READONLY_STATE else modules['editor_mouse_probe_case'].run_editor_mouse_probe),
        namespace_stage=modules['editor_probe_namespace'].stage,
        namespace_validate=modules['editor_probe_namespace'].validate_stage)


def execute(binding, dependencies):
    """One existing reservation/command lock/operation; failures propagate to HOLD."""
    d, b = dependencies, binding
    stage_plan = b.get('state') == STAGE_PLAN_STATE
    readonly = b.get('state') == READONLY_STATE
    with command_lock(HARNESS / 'runtime/test.lock') as lock_fd:
        need('AMIGA_LAUNCH_INFLIGHT' not in os.environ, 'Parent cannot borrow an inherited FRAME')
        if readonly:
            validate_binding(b, readonly=True)
        elif stage_plan:
            validate_binding(b, stage_plan=True)
        else:
            validate_binding(b)
        guard = d.guard(HARNESS)
        lease = d.lease_input(HARNESS, 'amiberry-030')
        status = guard.status()
        need(status['phase'] == 'READY' and not status['holds'] and status['inflight'] is None
             and lease['run_id'] == b['ownerRunID'], 'Fresh original READY owner required')
        with guard._locked() as (_, state):
            guard._owner(state, lease, 'amiberry-030')
        d.install_client(); d.install_ipc(HARNESS)
        with d.operation(HARNESS, 'amiberry-030',
                         ('One owned probe stage; ' if stage_plan else '')
                         + 'exact mounted editor one move, passive V2 readback/execution and at most one click',
                         may_leave_running=True) as frame:
            original = d.original_api(b['blankBinding'], frame)
            original.owner(b['ownerRunID'])
            if stage_plan:
                deadline = time.monotonic() + STAGE_PLAN_LIMITS['probeStage']
                def current_stage_owner(owner):
                    need(time.monotonic() < deadline, 'Probe stage deadline exhausted; retain without replay')
                    original.owner(owner)
                    need(time.monotonic() < deadline, 'Probe stage deadline exhausted after owner check')
                window = Path(b['outputDirectory']).parent
                stage_ref = d.namespace_stage(b['passiveProbe'], window / 'passive-probe-stage',
                    SimpleNamespace(stage_owner=current_stage_owner), lock_fd=lock_fd)
                current_stage_owner(b['ownerRunID'])
                stage = d.namespace_validate(b['passiveProbe'], stage_ref)
                # This child data is a subset of the accepted stage+mouse scope.
                # Its endpoint references come only from the actual completed stage.
                child = copy.deepcopy(b)
                child.update(state='ROOT_ADMITTED_EDITOR_MOUSE_WITH_PASSIVE_V2',
                             allowedActions=copy.deepcopy(ALLOWED), limits=copy.deepcopy(LIMITS))
                child['passiveProbe'].update(namespaceStageRef=stage_ref,
                                             stagedCandidate=stage['stagedCandidate'])
                validate_binding(child)
                from editor_probe_namespace import save
                save(window / 'mouse-parent-staged-binding.json', child)
                b = child

            async def run():
                captured = await d.blank_case(b['blankBinding'], Path(b['outputDirectory']), original)
                original.owner(b['ownerRunID'])
                mouse = d.mouse_api(original, b, frame,
                                    move_request=d.move_request, click_request=d.click_request)
                return await d.composed_case(b, captured,
                                             Path(b['outputDirectory']) / 'mouse-phase', mouse, original)

            return asyncio.run(run())


def main():
    require_harness_runtime()
    os.umask(0o077)
    sys.dont_write_bytecode = True
    need(len(sys.argv) == 3 and sys.argv[1] == '--binding-ref',
         'Actual newly admitted parent binding reference required')
    ref = json.loads(sys.argv[2])
    binding = json.loads(pinned(ref))
    freeze = validate_binding(binding, stage_plan=binding.get('state') == STAGE_PLAN_STATE,
                              readonly=binding.get('state') == READONLY_STATE)
    need(ref['path'] == str(Path(binding['outputDirectory']).parent / 'mouse-parent-binding.json'),
         'Binding must be the actual new private case admission')
    dependencies = load_original_modules(binding, freeze)
    pinned(ref)
    result = execute(binding, dependencies)
    print(json.dumps({'state': result['state'], 'attempts': result['attempts'], 'lifecyclePending': True}))


if __name__ == '__main__':
    main()
