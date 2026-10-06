"""Pure public saved-byte/relationship checks; no SDK, product or target calls."""
from pathlib import Path
import hashlib
import json
import stat

BASE = Path(__file__).resolve().parent


def read(path):
    info = path.lstat()
    assert stat.S_ISREG(info.st_mode) and not getattr(info, 'st_flags', 0) & 0x40000000
    data = path.read_bytes()
    after = path.lstat()
    assert (info.st_dev, info.st_ino, info.st_mode, info.st_size, info.st_mtime_ns) == (
        after.st_dev, after.st_ino, after.st_mode, after.st_size, after.st_mtime_ns)
    assert len(data) == info.st_size
    return data


def fp(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def same(data, pin):
    assert fp(data) == {key: pin[key] for key in ('bytes', 'sha256')}


def load(name):
    return json.loads(read(BASE / name))


manifest = load('saved-manifest.json')
assert manifest['status'] == 'FROZEN_PUBLIC_COMPILER_CUSTODY_NEVER_EXECUTED'
files = manifest['files']
actual = {p.relative_to(BASE).as_posix() for p in BASE.rglob('*') if p.is_file()}
assert actual == set(files) | {'saved-manifest.json'}
for name, pin in files.items():
    assert not Path(name).is_absolute() and '..' not in Path(name).parts
    same(read(BASE / name), pin)

compiled = load('compiler/manifest.json')
proof = load('compiler/saved-byte-verification.json')
host = load('host/run.json')
controls = load('host/current-source-v2.json')
assert compiled['status'] == 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED'
assert proof['status'] == 'PASS_INDEPENDENT_SAVED_BYTES_COMPLETE_PORTABLE_FIXTURE_NEVER_EXECUTED'
same(read(BASE / 'compiler/manifest.json'), proof['manifest'])
assert compiled['source_stable'] and compiled['frozen_c678_plus_four_verified']
assert all(compiled[key] for key in ('protected_16_stable', 'original_host_evidence_stable',
                                    'four_overlays_stable', 'git_head_index_stable'))
assert compiled['source_before'] == compiled['source_after']
assert compiled['source_before'] == {
    name: {key: pin[key] for key in ('bytes', 'sha256')}
    for name, pin in host['inputs'].items()}
assert compiled['host_source_modes'] == {name: pin['mode'] for name, pin in host['inputs'].items()}
assert len(host['inputs']) == 923
assert host['status'] == controls['status'] == 'PASS'
assert host['head'] == controls['baseline'] == compiled['host_baseline'] == (
    'c6783d79ea679bdc8acff3d38ee1c66f1564c9ed')
same(read(BASE / 'host/run.json'), compiled['host_evidence'])
for name, pin in controls['files'].items():
    assert host['inputs'][name] == {key: pin[key] for key in ('bytes', 'sha256', 'mode')}
same(read(BASE / 'host/independent-final-host-review.json'), controls['finalReview'])
assert len(host['calls']) == 4
for call in host['calls']:
    assert call['status'] == 'PASS' and call['returncode'] == 0
    assert read(BASE / ('host/' + call['label'] + '.stderr')) == b''
assert compiled['ordered_units'] == [arg for arg in host['calls'][0]['argv'] if arg.endswith('.c')]
assert len(compiled['ordered_units']) == len(set(compiled['ordered_units'])) == 26
assert compiled['ordered_units'][:2] == ['tests/mixed_scheduled_readers_test.c',
                                      'src/core/mixed_scheduled_readers.c']
assert len(compiled['calls']) == 35 and len(compiled['dependencies']) == 93
assert len(compiled['tools']) == 5 and len(compiled['runtime_inputs']) == 7
for call in compiled['calls']:
    assert call['status'] == 'PASS' and call['returncode'] == 0
    assert call['product_execution'] == 'NEVER_EXECUTED' and call['timeout_seconds'] <= 120
    for kind in ('stdout', 'stderr'):
        data = read(BASE / ('compiler/logs/' + Path(call[kind]['path']).name))
        same(data, call[kind])
        if kind == 'stderr':
            assert data == b''
assert compiled['flags'] == ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
                             '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-Isrc/core', '-I.', '-fbbb=-']
assert compiled['bounds']['overall_seconds'] == 600
assert compiled['bounds']['each_compiler_call_seconds'] == 120
assert compiled['overall_elapsed_seconds'] < 600
assert read(BASE / 'compiler/build.stderr') == read(BASE / 'compiler/saved-verification.stderr') == b''
same(read(BASE / 'compiler/verify_saved_bytes.py.reference'), proof['observer'])
expected = read(BASE / 'expected-native.stdout')
same(expected, compiled['expected_stdout'])
assert expected == read(BASE / 'host/paired-run.stdout')
assert fp(expected) == {'bytes': 725, 'sha256': '1b285024f5e96e22849f7228540fc8f461cb2eb5a8fd1f8338ce588fad5f6d9b'}
assert len(expected.splitlines()) == 4
for marker in (b'MIXED READERS PASS:', b'amigus reservation lifecycle: PASS',
               b'WAVETABLE UPLOAD JOB PASS:', b'AMIGUS WAVETABLE OWNER PASS:'):
    assert marker in expected
assert {key: compiled['product'][key] for key in ('bytes', 'sha256')} == {
    'bytes': 187332, 'sha256': '8ea8dae1da8ba6f4bfc4e0e64adc7a550dd903c2baf09f37eb956933c3dd3e05'}
assert proof['product']['bytes'] == compiled['product']['bytes']
assert proof['product']['sha256'] == compiled['product']['sha256']
assert all(compiled[key] == 'NEVER_EXECUTED' for key in
           ('target_execution', 'emulator_execution', 'physical_execution', 'native_execution'))
assert compiled['product']['execution'] == proof['product_execution'] == 'NEVER_EXECUTED'
withdrawn = load('prior/precompiler-withdrawal.json')
assert withdrawn['compilerCalls'] == 0 and not withdrawn['productCreated']
assert not withdrawn['productExecution'] and withdrawn['targetActions'] == 0
assert manifest['productIncluded'] is False and manifest['targetExecution'] == 'NOT_RUN'
print(json.dumps({'status': 'PASS_PUBLIC_SAVED_COMPILER_CUSTODY_NEVER_EXECUTED',
                  'files': len(files), 'sources': 923, 'units': 26, 'calls': 35,
                  'dependencies': 93, 'markers': 4}))
