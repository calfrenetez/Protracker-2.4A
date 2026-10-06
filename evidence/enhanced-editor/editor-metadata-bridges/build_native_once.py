"""Root-owned pinned portable 68k compile/link. Never executes the HUNK."""
from pathlib import Path
import hashlib
import json
import shlex
import struct
import subprocess
import time

BASE = Path(__file__).resolve().parent
SOURCE = BASE / 'candidate'
OUTPUT = BASE / 'native'
PINS = BASE.parents[1] / 'readers-activation-1rr1yyrp/native-ram-entry-sampled-stack-private-build-v1/manifest.json'
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
         '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-Isrc/core', '-fbbb=-']

def record(path):
    path = Path(path)
    data = path.read_bytes()
    return {'path': str(path), 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

def inventory():
    return {p.relative_to(SOURCE).as_posix(): record(p)['sha256']
            for p in sorted(SOURCE.rglob('*')) if p.is_file()}

assert not OUTPUT.exists(), 'Do not overwrite a prior build result.'
current = inventory()
assert current == json.loads((BASE / 'source-controls.json').read_text())['source_files']
host = json.loads((BASE / 'host/execution.json').read_text())
assert host['status'] == 'PASS_HOST_ONLY' and host['source_stable']
assert host['source_before'] == host['source_after'] == current
units = [arg for arg in host['calls'][0]['argv'] if arg.endswith('.c')]
assert units[0] == 'tests/editor_mixed_bridges_test.c' and len(units) == len(set(units))
pins = json.loads(PINS.read_text())
compiler = pins['tools']['compiler']
assert record(compiler['path'])['sha256'] == compiler['sha256']
assert all(record(pin['path'])['sha256'] == pin['sha256'] for pin in pins['tools'].values())
OUTPUT.mkdir()
result = {'scope': 'Compiler/link only; HUNK never executed; no native stack, timing, placement or device proof',
          'status': 'RUNNING', 'compiler': compiler, 'tools': pins['tools'], 'flags': FLAGS,
          'source_before': inventory(), 'units': units, 'calls': [], 'dependencies': {}}

def save():
    (OUTPUT / 'manifest.json').write_text(json.dumps(result, indent=2) + '\n')

def run(argv, label):
    assert record(compiler['path'])['sha256'] == compiler['sha256']
    entry = {'argv': argv, 'cwd': str(SOURCE), 'label': label, 'timeout_seconds': 120}
    result['calls'].append(entry)
    save()
    started = time.monotonic()
    with (OUTPUT / (label + '.stdout')).open('wb') as out, (OUTPUT / (label + '.stderr')).open('wb') as err:
        try:
            completed = subprocess.run(argv, cwd=SOURCE, stdout=out, stderr=err,
                                       timeout=120, check=False)
            entry['returncode'] = completed.returncode
        except BaseException as failure:
            entry['failure'] = repr(failure)
            raise
        finally:
            entry['elapsed_seconds'] = time.monotonic() - started
            save()
    if completed.returncode:
        raise subprocess.CalledProcessError(completed.returncode, argv)
    return (OUTPUT / (label + '.stdout')).read_text()

try:
    run([compiler['path'], '--version'], 'compiler-version')
    for name, pin in pins['runtime_inputs'].items():
        path = Path(run([compiler['path'], '-m68000', '-msoft-float', '-mcrt=nix20',
                         '-print-file-name=' + name], 'runtime-' + name.replace('.', '-')).strip())
        assert path.resolve() == Path(pin['path']).resolve()
        assert record(path)['sha256'] == pin['sha256']
    result['runtime_inputs'] = pins['runtime_inputs']
    for index, unit in enumerate(units):
        text = run([compiler['path'], *FLAGS, '-M', unit], 'dependency-%02d' % index)
        paths = shlex.split(text.replace('\\\n', ' ').split(':', 1)[1])
        for raw in paths:
            path = (SOURCE / raw).resolve() if not Path(raw).is_absolute() else Path(raw).resolve()
            result['dependencies'][str(path)] = record(path)
    binary = OUTPUT / 'PTEditorBridgesTest'
    run([compiler['path'], *FLAGS, *units, '-o', str(binary)], 'compile-link')
    assert struct.unpack('>I', binary.read_bytes()[:4])[0] == 0x3F3
    result['product'] = record(binary)
    assert all(record(path)['sha256'] == pin['sha256'] for path, pin in result['dependencies'].items())
    assert all(record(pin['path'])['sha256'] == pin['sha256'] for pin in result['runtime_inputs'].values())
    assert all(record(pin['path'])['sha256'] == pin['sha256'] for pin in result['tools'].values())
    result['status'] = 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED'
except BaseException as failure:
    result['status'] = 'FIRST_FAILURE_RETAINED'
    result['failure'] = repr(failure)
    raise
finally:
    result['source_after'] = inventory()
    result['source_stable'] = result['source_after'] == result['source_before']
    if not result['source_stable']: result['status'] = 'SOURCE_CHANGED_FAILURE'
    save()
print(json.dumps({'status': result['status'], 'calls': len(result['calls']),
                  'product': result.get('product'), 'dependencies': len(result['dependencies'])}))
if result['status'] != 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED': raise SystemExit(1)
