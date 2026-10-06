"""Exact committed src/tests plus four controller and five root overlays; HOST_ONLY."""
from pathlib import Path
import ast
import hashlib
import io
import json
import os
import signal
import subprocess
import sys
import tarfile
import tempfile
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'a02d53f2047753fd8673cbcc377d772da1ceb531'
EVIDENCE = ROOT.parents[1] / 'outputs/editor-mixed-readers-prepare/v1'
OWN = [
    'src/editor/editor_mixed_readers_prepare.h', 'src/editor/editor_mixed_readers_prepare.c',
    'tests/editor_mixed_readers_prepare_test.c', 'tests/test_editor_mixed_readers_prepare.py',
]
ROOT_CORE = [
    'src/core/mixed_readers_activation.h', 'src/core/mixed_readers_activation.c',
    'src/core/mixed_scheduled_readers.h', 'src/core/mixed_scheduled_readers.c',
    'tests/mixed_scheduled_readers_test.c',
]
PROTECTED = [
    'AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h',
    'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c',
    'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c',
    'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py',
    'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py',
]


def fingerprint(path):
    body = path.read_bytes()
    return {'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest(),
            'mode': path.stat().st_mode & 0o777}


def constants(folder, name, key):
    values = {}
    for node in ast.parse((folder / name).read_text()).body:
        if isinstance(node, ast.Assign):
            try:
                value = ast.literal_eval(node.value)
            except (ValueError, TypeError):
                continue
            for target in node.targets:
                if isinstance(target, ast.Name):
                    values[target.id] = value
        elif isinstance(node, ast.AugAssign) and isinstance(node.target, ast.Name) and isinstance(node.op, ast.Add):
            values[node.target.id] += ast.literal_eval(node.value)
    return values[key]


def main():
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    attempt = Path(tempfile.mkdtemp(prefix='attempt-', dir=EVIDENCE))
    record = attempt / 'run.json'
    report = {
        'status': 'PREPARATION_ATTEMPTED', 'phase': 'PREPARATION',
        'scope': 'HOST software ASan/UBSan only; no native/emulator/physical product execution, timer/IRQ/device/DMA/stack/timing/audio/listening acceptance.',
        'attempt': str(attempt), 'preparation_calls': [], 'calls': [],
        'first_failure': None,
    }

    def persist():
        record.write_text(json.dumps(report, indent=2) + '\n')

    def first_failure(label, error):
        if report['first_failure'] is None:
            report['first_failure'] = {'label': label, 'error': repr(error),
                                       'exception_type': type(error).__name__}

    # This record precedes all Git/head/index/archive/extraction reads. A failed
    # preparation is an actual first failure with its own custody, not NOT RUN.
    persist()

    def group_exists(pgid):
        try:
            os.killpg(pgid, 0)
            return True
        except ProcessLookupError:
            return False

    def call(label, argv, cwd, limit, collection='calls'):
        start = time.monotonic()
        deadline = start + limit
        stdout = attempt / (label + '.stdout')
        stderr = attempt / (label + '.stderr')
        row = {'label': label, 'argv': argv, 'cwd': str(cwd),
               'whole_call_seconds': limit, 'cleanup_reserve_seconds': 5,
               'stdout': str(stdout), 'stderr': str(stderr), 'status': 'ATTEMPTED'}
        report[collection].append(row)
        persist()
        process = None
        pgid = None
        error = None
        cleanup_error = None
        with stdout.open('wb') as out, stderr.open('wb') as err:
            try:
                # Every driver owns a fresh session/process group. Output files
                # capture partial timeout/error bytes without communicate pipe
                # ambiguity or dropping TimeoutExpired's observations.
                process = subprocess.Popen(argv, cwd=cwd, stdout=out, stderr=err,
                                           start_new_session=True)
                pgid = process.pid
                row.update(pid=process.pid, pgid=pgid, owned_new_session=True)
                persist()
                try:
                    process.wait(timeout=max(0, deadline - 5 - time.monotonic()))
                except subprocess.TimeoutExpired as caught:
                    error = caught
                    row['timeout_error'] = repr(caught)
                # A reaped successful driver is insufficient if one of its
                # owned cc1/as/linker descendants is still alive.
                if group_exists(pgid) and error is None:
                    error = RuntimeError('owned process group survived driver exit')
            except BaseException as caught:
                error = caught
            finally:
                try:
                    if pgid is not None and group_exists(pgid):
                        row['cleanup'] = 'OWNED_PGID_TERM_THEN_KILL_IF_REQUIRED'
                        try:
                            os.killpg(pgid, signal.SIGTERM)
                        except ProcessLookupError:
                            pass
                        soft_deadline = min(deadline, time.monotonic() + 0.5)
                        while time.monotonic() < soft_deadline:
                            if process is not None:
                                process.poll()
                            if not group_exists(pgid):
                                break
                            time.sleep(min(0.02, max(0, soft_deadline-time.monotonic())))
                        if group_exists(pgid):
                            try:
                                os.killpg(pgid, signal.SIGKILL)
                            except ProcessLookupError:
                                pass
                        while time.monotonic() < deadline:
                            if process is not None:
                                process.poll()
                            if not group_exists(pgid):
                                break
                            time.sleep(min(0.02, max(0, deadline-time.monotonic())))
                    if process is not None:
                        # poll/wait reap only this actual driver, never an
                        # unrelated process. No broad process-name matching.
                        if process.poll() is None and time.monotonic() < deadline:
                            process.wait(timeout=max(0, deadline-time.monotonic()))
                        row['returncode'] = process.poll()
                        row['driver_reaped'] = row['returncode'] is not None
                    else:
                        row['driver_started'] = False
                    row['group_quiescent'] = pgid is None or not group_exists(pgid)
                    if process is not None and (not row['driver_reaped'] or not row['group_quiescent']):
                        cleanup_error = RuntimeError('owned driver/group cleanup not positively confirmed')
                except BaseException as caught:
                    cleanup_error = caught
        # File closure follows bounded cleanup; hashes cover all captured bytes,
        # including failed creation, timeout, signal and descendant output.
        row['stdout_snapshot'] = fingerprint(stdout)
        row['stderr_snapshot'] = fingerprint(stderr)
        row['elapsed_seconds'] = time.monotonic() - start
        if cleanup_error is not None:
            row['cleanup_error'] = repr(cleanup_error)
            if error is None:
                error = cleanup_error
        if process is not None and error is None and (process.returncode or row['stderr_snapshot']['bytes']):
            error = RuntimeError(label + ' nonzero return or stderr')
        if error is not None:
            row.update(status='FAIL', error=repr(error), exception_type=type(error).__name__)
            first_failure(label, error)
            persist()
            raise error
        row['status'] = 'PASS'
        persist()
        return stdout.read_bytes()

    def git(label, *args):
        return call(label, ['git', *args], ROOT, 30, 'preparation_calls')

    try:
        head = git('prepare-head', 'rev-parse', 'HEAD').decode().strip()
        report['head'] = head
        if head != BASELINE:
            raise RuntimeError('unexpected HEAD')
        index = git('prepare-index', 'ls-files', '--stage')
        protected = {p: fingerprint(ROOT / p) for p in PROTECTED}
        overlays = {p: fingerprint(ROOT / p) for p in OWN + ROOT_CORE}
        exact_root = {
            ROOT_CORE[0]: (10423, '64edd461b1413fc5b896f7b972b41421c22f04f327c66a21dfbc9b7043394ab5'),
            ROOT_CORE[1]: (39498, 'edd96b2ebc4b076e66a865eeaa637e25e0cfe56e480d46c0c4b6746a028d456c'),
            ROOT_CORE[2]: (11336, 'b24b9e762906bae10d5a511cd949c547715aada7893156e932aeab4315af70c4'),
            ROOT_CORE[3]: (33837, '2808fbbaf081d48c8ab70c36b57f2882d5bb0677aeb6f218c7a89d3bd0b570df'),
            ROOT_CORE[4]: (42588, '18a0bc644ecdec004d23e00aa1a3714d78650151afccf914df24514cc79295fe'),
        }
        for name, (size, digest) in exact_root.items():
            if (overlays[name]['bytes'], overlays[name]['sha256']) != (size, digest):
                raise RuntimeError('root core overlay changed: ' + name)
        report.update(overlays=overlays, protected=protected,
                      index_sha256=hashlib.sha256(index).hexdigest())
        persist()
        archive = git('prepare-archive', 'archive', head, 'src', 'tests')
        report['archive_sha256'] = hashlib.sha256(archive).hexdigest()
        source = attempt / 'source'
        source.mkdir()
        with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
            stream.extractall(source, filter='data')
        for name in OWN + ROOT_CORE:
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes((ROOT / name).read_bytes())
        font = git('prepare-font', 'show', head + ':vendor/pt23f/raw/ptfont.raw')
        if len(font) != 580:
            raise RuntimeError('unexpected committed font size')
        report['font_sha256'] = hashlib.sha256(font).hexdigest()
        (source / 'pt_font.h').write_text('static const unsigned char pt_font[580] = {' + ','.join(str(b) for b in font) + '};\n')
        sampler = constants(source, 'tests/test_sampler.py', 'SOURCES')[1:]
        editor = constants(source, 'tests/test_editor.py', 'SOURCES')[1:]
        studio = constants(source, 'tests/test_editor_studio.py', 'EXTRA')
        dispatch = constants(source, 'tests/test_wavetable_dispatch.py', 'DISPATCH')
        preflight = constants(source, 'tests/test_paula_preflight.py', 'SOURCES')[1:]
        units = list(dict.fromkeys([
            'tests/editor_mixed_readers_prepare_test.c', 'src/editor/editor_mixed_readers_prepare.c',
            'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_activation.c',
            'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c',
            'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c',
            *sampler, *editor, *studio, *dispatch, *preflight,
            *['src/core/' + n + '.c' for n in ['amigus_reservation', 'amigus_wavetable_cache',
                'amigus_sample_ram', 'sample_cache', 'playback_pcm']],
            *['src/editor/' + n + '.c' for n in ['editor_mixed', 'mixed_owner', 'mixed_transport',
                'mixed_preflight', 'paula_voices', 'paula_dispatch', 'editor_wavetable']],
            *['src/platform/' + n + '.c' for n in ['sample_import', 'raw_import', 'mod_import', 'pp20_import']],
        ]))
        flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                 '-fsanitize=address,undefined', '-Isrc/core', '-I.']
        inputs = {str(p.relative_to(source)): fingerprint(p) for p in sorted(source.rglob('*')) if p.is_file()}
        report.update(status='PREPARED_NOT_RUN', phase='COMPILE', inputs=inputs,
                      source_file_count=len(inputs), ordered_units=units)
        persist()
        executable = attempt / 'controller'
        call('controller-compile', ['cc', *flags, *units, '-o', str(executable)], source, 120)
        report['phase'] = 'RUN'
        persist()
        stdout = call('controller-run', [str(executable)], source, 180)
        markers = ['EDITOR MIXED READERS PREPARE PASS:', 'SAMPLER MIXED ACTIVATION PASS:',
                   'SAMPLER MIXED FACTORY PASS:']
        if any(marker.encode() not in stdout for marker in markers):
            raise RuntimeError('combined fixture marker absent')
        report['phase'] = 'VERIFY_CUSTODY'
        persist()
        if git('verify-head', 'rev-parse', 'HEAD').decode().strip() != head:
            raise RuntimeError('HEAD changed')
        if git('verify-index', 'ls-files', '--stage') != index:
            raise RuntimeError('index changed')
        if {p: fingerprint(ROOT / p) for p in PROTECTED} != protected:
            raise RuntimeError('protected work changed')
        if {p: fingerprint(ROOT / p) for p in OWN + ROOT_CORE} != overlays:
            raise RuntimeError('overlays changed')
        if {str(p.relative_to(source)): fingerprint(p) for p in sorted(source.rglob('*')) if p.is_file()} != inputs:
            raise RuntimeError('saved source inventory changed')
        report.update(status='PASS', phase='COMPLETE', markers=markers)
    except BaseException as error:
        first_failure(report['phase'], error)
        report.update(status='FAIL', error=repr(error), exception_type=type(error).__name__)
        raise
    finally:
        persist()
        print(record, flush=True)


if __name__ == '__main__':
    main()
