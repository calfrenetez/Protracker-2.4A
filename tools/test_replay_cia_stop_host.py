#!/usr/bin/env python3
"""Run the causal CIA ownership HOST model; never opens an Amiga target."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    'src/native/native_replay_cia_stop.c',
    'src/native/native_replay_cia_stop.h',
    'tests/native_replay_cia_stop_stubs.h',
    'tests/test_native_replay_cia_stop.c',
    'tools/test_replay_cia_stop_host.py',
)
LOG_LIMIT = 1024 * 1024
PRODUCT_LIMIT = 8 * 1024 * 1024


def source_hashes():
    facts = {}
    for name in SOURCES:
        path = ROOT / name
        if path.is_symlink() or not path.is_file() or path.stat().st_size > 65536:
            raise ValueError('Missing or nonordinary bounded source: ' + name)
        body = path.read_bytes()
        facts[name] = dict(bytes=len(body), sha256=hashlib.sha256(body).hexdigest())
    return facts


def command(argv, label, seconds, out, work, env, whole_deadline):
    """Bound two ordinary local child commands and preserve their exact logs."""
    start = time.monotonic()
    deadline = min(start + seconds, whole_deadline)
    row = dict(label=label, argv=argv, seconds_limit=seconds,
               timed_out=False, output_limit=False, returncode=None)
    paths = [out / (label + '.' + stream + '.log') for stream in ('stdout', 'stderr')]
    child = None
    with paths[0].open('xb') as stdout, paths[1].open('xb') as stderr:
        try:
            if deadline <= start:
                raise TimeoutError('Whole HOST deadline expired')
            child = subprocess.Popen(argv, cwd=work, env=env, stdin=subprocess.DEVNULL,
                                     stdout=stdout, stderr=stderr, start_new_session=True)
            while child.poll() is None:
                row['output_limit'] = any(p.stat().st_size > LOG_LIMIT for p in paths)
                row['timed_out'] = time.monotonic() >= deadline
                if row['output_limit'] or row['timed_out']:
                    # Only this invocation's new session is eligible for cleanup.
                    try:
                        os.killpg(child.pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
                    try:
                        child.wait(timeout=1)
                    except subprocess.TimeoutExpired:
                        try:
                            os.killpg(child.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                        child.wait(timeout=1)
                    break
                time.sleep(0.01)
            row['returncode'] = child.returncode
        finally:
            if child is not None and child.poll() is None:
                try:
                    os.killpg(child.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                child.wait(timeout=1)
    row['seconds'] = time.monotonic() - start
    row['streams'] = {}
    for stream, path in zip(('stdout', 'stderr'), paths):
        size = path.stat().st_size
        if size > LOG_LIMIT:
            row['output_limit'] = True
        # Hash logs as streams; do not load an excessive diagnostic into memory.
        digest = hashlib.sha256()
        with path.open('rb') as saved:
            for block in iter(lambda: saved.read(65536), b''):
                digest.update(block)
        row['streams'][stream] = dict(file=path.name, bytes=size, sha256=digest.hexdigest())
    row['passed'] = (row['returncode'] == 0 and not row['timed_out'] and
                     not row['output_limit'] and
                     all(s['bytes'] == 0 for s in row['streams'].values()))
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence-dir', required=True,
                        help='Fresh directory for exact compile/run logs and report.json')
    args = parser.parse_args()
    out = Path(args.evidence_dir).absolute()
    out.mkdir(parents=True, exist_ok=False)
    start = time.monotonic()
    report = dict(scope='HOST causal API/register model only; no native API, IRQ, DMA or target acceptance',
                  assertions=True, sanitizers=['address', 'undefined'],
                  leak_detection=False, whole_seconds_limit=40, checks=[], passed=False)
    try:
        report['source_before'] = source_hashes()
        with tempfile.TemporaryDirectory(prefix='pt24g-cia-stop-host-') as directory:
            work = Path(directory)
            binary = work / 'cia-stop-host'
            env = dict(PATH='/usr/bin:/bin', LANG='C', LC_ALL='C', TMPDIR=str(work),
                       ASAN_OPTIONS='detect_leaks=0:halt_on_error=1:abort_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            compile_argv = ['/usr/bin/cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-UNDEBUG', '-fsanitize=address,undefined',
                            '-fno-omit-frame-pointer', str(ROOT / SOURCES[3]), '-o', str(binary)]
            report['checks'].append(command(compile_argv, 'compile', 8, out, work, env, start + 40))
            if not report['checks'][-1]['passed']:
                raise ValueError('Compilation failed, exceeded a bound or emitted diagnostics')
            if binary.is_symlink() or not binary.is_file() or binary.stat().st_size > PRODUCT_LIMIT:
                raise ValueError('Missing or excessive HOST executable')
            body = binary.read_bytes()
            report['executable'] = dict(bytes=len(body), sha256=hashlib.sha256(body).hexdigest())
            report['checks'].append(command([str(binary)], 'run', 2, out, work, env, start + 40))
            if not report['checks'][-1]['passed']:
                raise ValueError('Causal model assertions/sanitizers or bounds failed')
        report['source_after'] = source_hashes()
        report['source_unchanged'] = report['source_before'] == report['source_after']
        report['passed'] = report['source_unchanged'] and time.monotonic() < start + 40
    except Exception as error:
        report['error'] = str(error)
    report['seconds'] = time.monotonic() - start
    (out / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('PASS causal CIA HOST model' if report['passed'] else 'FAIL causal CIA HOST model')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
