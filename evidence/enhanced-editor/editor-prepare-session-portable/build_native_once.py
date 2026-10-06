"""Host-only pinned portable crossbuild. Never executes any target product."""
from pathlib import Path
import hashlib
import json
import shlex
import shutil
import stat
import struct
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
REPO = ROOT / 'work/Protracker-2.4A'
HOST = BASE.parent / 'v1/attempt-9vc22i93'
INPUT = HOST / 'source'
SOURCE = BASE / 'source'
OUTPUT = BASE / 'native'
OLD = ROOT / 'outputs/editor-metadata-bridges/v2'
PINS = ROOT / 'outputs/readers-activation-1rr1yyrp/native-ram-entry-sampled-stack-private-build-v1/manifest.json'
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
         '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-Isrc/core', '-I.', '-fbbb=-']
started_overall = time.monotonic()
DEADLINE = started_overall + 600


def record(path):
    path = Path(path)
    assert stat.S_ISREG(path.lstat().st_mode), 'Nonordinary input: ' + str(path)
    data = path.read_bytes()
    return {'path': str(path), 'resolved_path': str(path.resolve()),
            'bytes': len(data), 'mode': stat.S_IMODE(path.lstat().st_mode),
            'sha256': hashlib.sha256(data).hexdigest()}


def inventory(root):
    return {p.relative_to(root).as_posix(): {'bytes': r['bytes'], 'sha256': r['sha256']}
            for p in sorted(root.rglob('*')) if p.is_file()
            for r in [record(p)]}


assert not SOURCE.exists() and not OUTPUT.exists(), 'Do not overwrite a prior build.'
assert not (BASE / 'manifest.json').exists(), 'Do not replay a terminal build.'
host = json.loads((HOST / 'run.json').read_text())
assert host['status'] == 'PASS' and host['head'] == 'd57e178683c3350a0618a1e02cf31d613dd2387d'
assert len(host['calls']) == 6 and all(c['status'] == 'PASS' and c['returncode'] == 0 for c in host['calls'])
original_source = inventory(INPUT)
assert original_source == host['inputs']
host_custody = inventory(HOST)
fresh_controls = json.loads((BASE / 'fresh-host-controls.json').read_text())
assert fresh_controls['status'] == 'PASS_FRESH_HOST_SOURCE_CURRENT_FOUR_AND_PRIOR_FAILURE_CUSTODY'
assert inventory(BASE.parent / 'portable-v1') == fresh_controls['prior_portable_v1']
assert all((HOST / (c['label'] + '.stderr')).stat().st_size == 0 for c in host['calls'])
assert all(record(REPO / p)['sha256'] == pin['sha256'] for p, pin in fresh_controls['own_four'].items())
units = [arg for arg in host['calls'][0]['argv'] if arg.endswith('.c')]
assert units[0] == 'tests/editor_mixed_prepare_session_test.c' and len(units) == len(set(units))
assert 'src/editor/editor_mixed_prepare_session.c' in units
assert len(units) > 70, 'Do not substitute a progress-only fixture.'
protected = json.loads((OLD / 'source-controls.json').read_text())['protected']
for relative, pin in protected.items():
    actual = record(REPO / relative)
    assert actual['sha256'] == pin['sha256'] and (REPO / relative).lstat().st_mode == pin['mode']
pins = json.loads(PINS.read_text())
for pin in [*pins['tools'].values(), *pins['runtime_inputs'].values()]:
    actual = record(pin['path'])
    assert actual['sha256'] == pin['sha256'] and actual['bytes'] == pin['bytes']
shutil.copytree(INPUT, SOURCE)
assert inventory(SOURCE) == original_source
OUTPUT.mkdir()
shutil.copyfile(HOST / 'run.json', BASE / 'host-run.json')
shutil.copyfile(HOST / 'session-run.stdout', BASE / 'expected-native.stdout')
shutil.copyfile(PINS, BASE / 'prior-compiler-pins.json')
shutil.copyfile(OLD / 'build_native_once.py', BASE / 'prior-builder.py.reference')
compiler = pins['tools']['compiler']
result = {
    'status': 'RUNNING_COMPILER_ONLY_NEVER_EXECUTED',
    'scope': 'Host compiler/link only; complete portable session/checked/editor assertion bodies. No target product execution, emulator, physical, UI PLAY, placement, stack, wallclock, audio or device acceptance.',
    'target_execution': 'NEVER_EXECUTED', 'emulator_execution': 'NEVER_EXECUTED',
    'physical_execution': 'NEVER_EXECUTED', 'host_baseline': host['head'],
    'overlay_files': ['src/editor/editor_mixed_prepare_session.c',
                      'src/editor/editor_mixed_prepare_session.h',
                      'tests/editor_mixed_prepare_session_test.c',
                      'tests/test_editor_mixed_prepare_session.py'],
    'source_before': original_source, 'source_count': len(original_source),
    'fresh_host_controls': record(BASE / 'fresh-host-controls.json'),
    'prior_portable_v1_first_failure_preserved': True,
    'current_own_four': fresh_controls['own_four'],
    'builder': record(__file__), 'host_evidence': record(HOST / 'run.json'),
    'expected_stdout': record(BASE / 'expected-native.stdout'),
    'compiler_pin_source': record(PINS), 'compiler': compiler, 'tools': pins['tools'],
    'runtime_inputs': pins['runtime_inputs'], 'flags': FLAGS, 'ordered_units': units,
    'unit_count': len(units), 'calls': [], 'dependencies': {}, 'retained_files': {},
    'bounds': {'overall_seconds': 600, 'each_compiler_call_seconds': 120,
               'behavior': 'First failure retained; no automatic retry or product execution'},
    'python': {'version': sys.version, 'invoked_executable': sys.executable,
               'executable': record(Path(sys.executable).resolve())},
    'protected_16_before': protected,
}


def save():
    (BASE / 'manifest.json').write_text(json.dumps(result, indent=2) + '\n')


def check_tools():
    for pin in [*pins['tools'].values(), *pins['runtime_inputs'].values()]:
        actual = record(pin['path'])
        assert actual['sha256'] == pin['sha256'] and actual['bytes'] == pin['bytes']


def call(argv, label):
    check_tools()
    remaining = DEADLINE - time.monotonic()
    assert remaining > 0, 'Overall 600-second build deadline exceeded.'
    timeout = min(120, remaining)
    entry = {'argv': argv, 'cwd': str(SOURCE), 'label': label,
             'timeout_seconds': timeout, 'product_execution': 'NEVER_EXECUTED'}
    result['calls'].append(entry)
    save()
    started = time.monotonic()
    outpath, errpath = OUTPUT / (label + '.stdout'), OUTPUT / (label + '.stderr')
    try:
        with outpath.open('xb') as out, errpath.open('xb') as err:
            completed = subprocess.run(argv, cwd=SOURCE, stdout=out, stderr=err,
                                       timeout=timeout, check=False)
        entry['returncode'] = completed.returncode
        entry['status'] = 'PASS' if completed.returncode == 0 else 'FIRST_FAILURE_RETAINED'
        if completed.returncode:
            raise subprocess.CalledProcessError(completed.returncode, argv)
        return outpath.read_text()
    except BaseException as failure:
        entry['failure'] = repr(failure)
        raise
    finally:
        entry['elapsed_seconds'] = time.monotonic() - started
        entry['stdout'] = record(outpath) if outpath.exists() else None
        entry['stderr'] = record(errpath) if errpath.exists() else None
        save()


try:
    call([compiler['path'], '--version'], 'compiler-version')
    for name, pin in pins['runtime_inputs'].items():
        runtime_path = Path(call([compiler['path'], '-m68000', '-msoft-float', '-mcrt=nix20',
                                 '-print-file-name=' + name], 'runtime-' + name.replace('.', '-')).strip())
        assert runtime_path.resolve() == Path(pin['path']).resolve()
        assert record(runtime_path)['sha256'] == pin['sha256']
    for index, unit in enumerate(units):
        text = call([compiler['path'], *FLAGS, '-M', unit], 'dependency-%02d' % index)
        raw_paths = shlex.split(text.replace('\\\n', ' ').split(':', 1)[1])
        assert raw_paths
        for raw in raw_paths:
            path = (SOURCE / raw).resolve() if not Path(raw).is_absolute() else Path(raw).resolve()
            result['dependencies'][str(path)] = record(path)
    assert inventory(SOURCE) == original_source
    assert all(record(p)['sha256'] == pin['sha256'] for p, pin in result['dependencies'].items())
    binary = OUTPUT / 'PTEditorPrepareSessionTest'
    call([compiler['path'], *FLAGS, *units, '-o', str(binary)], 'compile-link')
    assert struct.unpack('>I', binary.read_bytes()[:4])[0] == 0x3F3, 'Expected Amiga HUNK.'
    result['product'] = {**record(binary), 'execution': 'NEVER_EXECUTED'}
    assert all(record(p)['sha256'] == pin['sha256'] for p, pin in result['dependencies'].items())
    check_tools()
    assert inventory(HOST) == host_custody
    assert inventory(BASE.parent / 'portable-v1') == fresh_controls['prior_portable_v1']
    assert all(record(REPO / p)['sha256'] == pin['sha256'] for p, pin in fresh_controls['own_four'].items())
    for relative, pin in protected.items():
        actual = record(REPO / relative)
        assert actual['sha256'] == pin['sha256'] and (REPO / relative).lstat().st_mode == pin['mode']
    result['protected_16_stable'] = True
    result['original_host_evidence_stable'] = True
    result['status'] = 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED'
except BaseException as failure:
    result['status'] = 'FIRST_FAILURE_RETAINED'
    result['failure'] = repr(failure)
    raise
finally:
    result['source_after'] = inventory(SOURCE)
    result['source_stable'] = result['source_before'] == result['source_after']
    if not result['source_stable']:
        result['status'] = 'SOURCE_CHANGED_FIRST_FAILURE_RETAINED'
    result['overall_elapsed_seconds'] = time.monotonic() - started_overall
    result['retained_files'] = {p.relative_to(BASE).as_posix(): record(p)
                                for p in sorted(BASE.rglob('*'))
                                if p.is_file() and p != BASE / 'manifest.json'}
    save()
print(json.dumps({'status': result['status'], 'source_count': len(original_source),
                  'unit_count': len(units), 'calls': len(result['calls']),
                  'dependencies': len(result['dependencies']), 'product': result.get('product')}))
if result['status'] != 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED':
    raise SystemExit(1)
