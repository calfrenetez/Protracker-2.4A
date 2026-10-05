"""Root-owned finite host checks. Retain first outputs/products; no target access."""
from pathlib import Path
import hashlib
import importlib
import io
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest

BASE = Path(__file__).resolve().parent
SOURCE = BASE / 'candidate'
OUTPUT = BASE / 'host-v1'
GROUPS = ['test_sampler_prepare_memory']
sys.dont_write_bytecode = True

def inventory():
    return {p.relative_to(SOURCE).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SOURCE.rglob('*')) if p.is_file()}

def write_result():
    (OUTPUT / 'execution.json').write_text(json.dumps(result, indent=2) + '\n')

assert not OUTPUT.exists(), 'Retain prior evidence; use a separately approved changed run.'
assert not any(name in os.environ for name in
               ('CPATH', 'C_INCLUDE_PATH', 'LIBRARY_PATH', 'ASAN_OPTIONS', 'UBSAN_OPTIONS'))
OUTPUT.mkdir()
before = inventory()
seal = json.loads((BASE / 'source-controls-v2.json').read_text())
assert before == seal['source_files'], 'Selected source changed after review.'
result = {'scope': 'Host ASan/UBSan fixtures only; no target or native execution',
          'status': 'RUNNING', 'source_before': before, 'calls': [], 'groups': []}
write_result()
original_run, original_temporary = subprocess.run, tempfile.TemporaryDirectory
active = None

class RetainedDirectory:
    def __init__(self, *args, **kwargs):
        assert active is not None and not args and not kwargs
        self.path = OUTPUT / active / 'work'
        self.path.mkdir(parents=True, exist_ok=False)
        self.name = str(self.path)
    def __enter__(self): return self.name
    def __exit__(self, *args): return False
    def cleanup(self): pass

def retained_run(argv, **kwargs):
    assert isinstance(argv, list) and argv
    index = len(result['calls'])
    if argv[0] == 'cc':
        argv = ['/usr/bin/cc', '-UNDEBUG', *map(str, argv[1:])]
        assert '-fsanitize=address,undefined' in argv
        assert argv.count('-o') == 1
        assert Path(argv[argv.index('-o') + 1]).is_relative_to(OUTPUT / active / 'work')
        timeout = 120
    else:
        argv = list(map(str, argv))
        assert len(argv) == 1 and Path(argv[0]).is_relative_to(OUTPUT)
        timeout = 45
    assert not (set(kwargs) - {'cwd', 'check'})
    prefix = OUTPUT / active / ('%02d' % index)
    record = {'argv': argv, 'group': active, 'cwd': str(kwargs.get('cwd', SOURCE)),
              'timeout_seconds': timeout}
    result['calls'].append(record)
    write_result()
    started = time.monotonic()
    with prefix.with_suffix('.stdout').open('wb') as out, prefix.with_suffix('.stderr').open('wb') as err:
        try:
            completed = original_run(argv, cwd=record['cwd'], stdout=out, stderr=err,
                                     timeout=timeout, check=False)
            record['returncode'] = completed.returncode
        except BaseException as failure:
            record['failure'] = repr(failure)
            raise
        finally:
            record['elapsed_seconds'] = time.monotonic() - started
            write_result()
    if completed.returncode:
        raise subprocess.CalledProcessError(completed.returncode, argv)
    if '-o' in argv:
        product = Path(argv[argv.index('-o') + 1])
        record['product'] = {'path': str(product), 'bytes': product.stat().st_size,
                             'sha256': hashlib.sha256(product.read_bytes()).hexdigest()}
        write_result()
    return completed

try:
    sys.path.insert(0, str(SOURCE / 'tests'))
    subprocess.run, tempfile.TemporaryDirectory = retained_run, RetainedDirectory
    for active in GROUPS:
        (OUTPUT / active).mkdir()
        module = importlib.import_module(active)
        suite = unittest.defaultTestLoader.loadTestsFromModule(module)
        stream = io.StringIO()
        outcome = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
        (OUTPUT / active / 'unittest.txt').write_text(stream.getvalue())
        result['groups'].append({'name': active, 'tests': outcome.testsRun,
                                 'success': outcome.wasSuccessful(),
                                 'sources': [arg for call in result['calls'] if call['group'] == active
                                             and call['argv'][0] == '/usr/bin/cc'
                                             for arg in call['argv'] if arg.endswith('.c')]})
        write_result()
        if not outcome.wasSuccessful():
            raise RuntimeError('First failed group retained: ' + active)
        assert outcome.testsRun == 1
        pair = [call for call in result['calls'] if call['group'] == active]
        assert len(pair) == 2 and pair[0]['argv'][0] == '/usr/bin/cc'
        assert pair[1]['argv'] == [pair[0]['product']['path']]
    assert len(result['calls']) == 2 and len(result['groups']) == 1
    result['status'] = 'PASS_HOST_ONLY'
except BaseException as failure:
    result['status'] = 'FIRST_FAILURE_RETAINED'
    result['failure'] = repr(failure)
    raise
finally:
    subprocess.run, tempfile.TemporaryDirectory = original_run, original_temporary
    result['source_after'] = inventory()
    result['source_stable'] = result['source_after'] == before
    if not result['source_stable']: result['status'] = 'SOURCE_CHANGED_FAILURE'
    write_result()
print(json.dumps({'status': result['status'], 'groups': len(result['groups']),
                  'calls': len(result['calls']), 'source_stable': result['source_stable']}))
if result['status'] != 'PASS_HOST_ONLY':
    raise SystemExit(1)
