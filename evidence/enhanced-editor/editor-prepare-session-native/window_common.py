"""Read-only exact-candidate custody; missing publication/pins refuse admission."""
import hashlib
import json
import stat
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
BASE = HERE.parent / 'portable-v2'
HOST = HERE.parent / 'v1/attempt-9vc22i93'
REPO = Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
INFRA = Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
NAME = 'PTEditorPrepareSessionTest'
STAGE = INFRA / 'runtime/Dev/Tests/PT-editor-prepare-session-20261006-v3'
OWN = ('src/editor/editor_mixed_prepare_session.c',
       'src/editor/editor_mixed_prepare_session.h',
       'tests/editor_mixed_prepare_session_test.c',
       'tests/test_editor_mixed_prepare_session.py')


def fp(path):
    path = Path(path)
    info = path.lstat()
    assert stat.S_ISREG(info.st_mode) and not getattr(info, 'st_flags', 0) & 0x40000000, str(path)
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def save(name, value):
    with (HERE / name).open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def binding():
    record = json.loads((HERE / 'binding.json').read_text())
    assert record['status'] == 'EXACT_HOST_PROMOTION_NO_TARGET_EXECUTION'
    assert len(record['sourceCommit']) == 40 and record['sourceCommit'] != '0' * 40
    assert record['sourceCount'] == 915
    return record


def check_custody():
    bound = binding()
    pins = json.loads((HERE / 'pins.json').read_text())['files']
    for path, expected in pins.items():
        assert fp(path) == expected, path
    promotion = json.loads((HERE / 'promotion.json').read_text())
    assert promotion['status'] == 'PASS_EXACT_HOST_PROMOTION_NOT_EXECUTED'
    assert fp(HERE / 'binding.json') == promotion['binding']
    assert fp(HERE / 'pins.json') == promotion['pins']
    assert Path(sys.executable).resolve() == Path(bound['python']).resolve()
    assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip() == bound['sourceCommit']
    assert not subprocess.check_output(['git', 'diff', '--cached', '--name-only'], cwd=REPO)
    build = json.loads((BASE / 'manifest.json').read_text())
    host = json.loads((HOST / 'run.json').read_text())
    assert build['status'] == 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and host['status'] == 'PASS'
    assert build['target_execution'] == build['emulator_execution'] == build['physical_execution'] == 'NEVER_EXECUTED'
    assert build['source_stable'] and build['protected_16_stable'] and build['original_host_evidence_stable']
    assert build['source_before'] == build['source_after'] == host['inputs']
    assert len(build['source_before']) == bound['sourceCount'] and build['unit_count'] == 82
    assert len(build['dependencies']) == 233 and len(build['calls']) == 91
    assert all(call['status'] == 'PASS' and call['returncode'] == 0 for call in build['calls'])
    assert len(host['calls']) == 6 and all(call['status'] == 'PASS' and call['returncode'] == 0 for call in host['calls'])
    for call in host['calls']:
        assert (HOST / (call['label'] + '.stderr')).read_bytes() == b''
    for name, expected in build['source_before'].items():
        assert fp(BASE / 'source' / name) == expected, name
        assert fp(HOST / 'source' / name) == expected, name
    for name, expected in build['protected_16_before'].items():
        path = REPO / name
        assert fp(path)['sha256'] == expected['sha256'] and path.lstat().st_mode == expected['mode'], name
    for name in OWN:
        assert fp(REPO / name) == build['source_before'][name], name
    for path, expected in build['dependencies'].items():
        assert fp(path) == {key: expected[key] for key in ('bytes', 'sha256')}, path
    for path, expected in build['retained_files'].items():
        assert fp(BASE / path) == {key: expected[key] for key in ('bytes', 'sha256')}, path
    product = build['product']
    assert fp(product['path']) == bound['candidate'] == {key: product[key] for key in ('bytes', 'sha256')}
    assert fp(BASE / 'expected-native.stdout') == bound['expectedStdout']
    return product, len(pins)


def check_complete(data):
    expected = (BASE / 'expected-native.stdout').read_bytes()
    assert fp(BASE / 'expected-native.stdout') == binding()['expectedStdout']
    assert data == expected, 'Complete target stdout differs from exact host assertion markers'
    assert len(data.splitlines()) == 3 and data.endswith(b'\n')
    return {'markerLines': 3, 'editorLegacy': 15, 'editorCheckedLifecycle': 39,
            'wholeControlAliases': 18, 'sessionLifecycle': 102,
            'sessionAdmissionAliases': 78, 'sessionExpiredTerminal': 3,
            'completeStdout': fp(BASE / 'expected-native.stdout'),
            'scope': 'Actual portable assertion bodies on target CPU; injected clocks/voices and ordinary-RAM bus callbacks only'}


def hold_failure():
    """Only future admitted callers use this; no target request or recovery."""
    try:
        from launcher_access import lease_input
        from launcher_guard import LauncherGuard
        guard = LauncherGuard(INFRA)
        state = guard.status()
        if not state['reservation']:
            return {'status': 'NO_RESERVATION_NO_TARGET_ADMISSION'}
        lease = lease_input(INFRA, 'amiberry-030')
        assert state['reservation']['lease']['run_id'] == lease['run_id']
        if not state['holds']:
            guard.hold(lease, 'ProTracker exact preparation-session first failure; no automatic retry or recovery')
        return {'status': 'OWNED_HOLD_RETAINED'}
    except BaseException as error:
        return {'status': 'HOLD_CONFIRMATION_REFUSED_PRESERVE_STATE',
                'errorType': type(error).__name__, 'error': str(error)[:300]}
