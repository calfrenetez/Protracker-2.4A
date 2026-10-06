"""One controlled standalone strict quantized audit HOST qualifier: UNSUPPLIED committed admission, two query-only modified and four additive PRIVATE overlays.

No invocation is authorized by this file's presence. A source-only freeze and a
separate root authorization precede each new once-only attempt. No target calls,
previous evidence adoption, product retries or source repair inside the qualifier.
"""
from pathlib import Path
import argparse
import ast
import hashlib
import io
import json
import os
import re
import shutil
import signal
import stat
import subprocess
import sys
import tarfile
import time

if sys.flags.optimize != 0:
    raise RuntimeError('Non-optimized HOST qualifier required; no preparation attempted')
sys.dont_write_bytecode = True
ROOT = Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
DRAFT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT.parents[1] / 'outputs/paired-quantized-strict-audit/v1/host-qualification'
BASELINE_STATUS = 'UNSUPPLIED_ROOT_MUST_SELECT_COMMITTED_BASELINE'
MODIFIED = ['src/core/render.h', 'src/core/render.c']
ADDITIVE = ['src/editor/mixed_quantized_audit.h', 'src/editor/mixed_quantized_audit.c', 'tests/mixed_quantized_audit_test.c', 'tests/test_mixed_quantized_audit.py']
OWN = MODIFIED + ADDITIVE
# These are exact unchanged source-origin facts, not previous acceptance or a
# selected qualifier baseline. Root must supply the current committed admission.
SOURCE_ORIGIN = 'PROPOSAL_SOURCE_PINS_ONLY_NOT_ADMISSION'
REQUIRED_COMMITTED_EXPECTED = {'src/editor/mixed_readers_plan_normalize.h': {'bytes': 8385, 'sha256': '306fe15de7fb39c8e045279f42963f773d96746767b432e5597a57a16c73a220'}, 'src/editor/mixed_readers_plan_normalize.c': {'bytes': 27173, 'sha256': '2e1e2628e7230f8fc2e06137b7861ff9b7332e4a8d24c30c55de3124d1c8f6fd'}, 'src/core/amigus_trigger_levels.h': {'bytes': 1886, 'sha256': 'f1db99e67a4b1e2c05ddfe666dc4c69ee640cd5837de33a01bbe88c1f521f6b0'}, 'src/core/amigus_trigger_levels.c': {'bytes': 3763, 'sha256': 'f5a5ecc8e3a5f2583ea01421a8cff66eef11ece2f1987a6f3e04479de63baaf3'}, 'src/editor/mixed_preflight.h': {'bytes': 7968, 'sha256': '805c75ef06d417787cf930a3344dd207f8e34b758e651469aa685599f67afbd2'}, 'src/editor/mixed_preflight.c': {'bytes': 22982, 'sha256': '394994b3129933cd99008bcefbdd9b4e02db5f5576634886857ac6c944b6b893'}, 'src/core/render_commands.h': {'bytes': 9193, 'sha256': 'ea176e8382ea808929d11571396d6214bb40d2fce8defc9b19569c1bb5c30d14'}, 'src/core/document.h': {'bytes': 1965, 'sha256': 'f66757baf8d762bf0413afd2b4e7b2e589ad6f6a6e2b59f6f72d9458a3fa279f'}}
MODIFIED_BASE = {'src/core/render.h': {'bytes': 11891, 'sha256': '50bca0c9fd47be061c3b0c027388f5c1bdbe4316a1caed3f33a696f7527812ea'}, 'src/core/render.c': {'bytes': 63218, 'sha256': '9253ab8bda1b9fd7c652a094615180edb14abf983265a8591754d982d4a17a94'}}
PROTECTED = [
    'AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h',
    'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c',
    'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c',
    'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py',
    'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py',
]
EXPECTED_STDOUT = b'STRICT Q AUDIT SCHEDULE PASS:12 genuine pinned8/16/24 Paula/card tempo-delay/lead-in lifetimes, literal every-boundary frames, exact centre32639/32895 seven-field images, SAME once-rewound pointer, exact master saves and empty DONE retention; SOFTWARE_ONLY\nSTRICT Q AUDIT FULL16 PASS:4 genuine mixed4Paula/12card full-song8/16-endian geometries,32 raw to16 exact records, origin15 retained through empty DONE, same sequence and literal fields; SOFTWARE_ONLY\nSTRICT Q AUDIT LATE PASS:genuine renderer-valid Paula901 offset256/length3840 old-gate success becomes strict late refusal at2880 before masks/sequence transfer; no cache/key/enqueue callback exists; SOFTWARE_ONLY\nSTRICT Q AUDIT GUARDS PASS:whole original owner/result/workspace/context/spare-master guards before mutation, copied-close refusal, five queried capacity/budget/wrap and four scope refusals, READY owner alias and bounded work; SOFTWARE_ONLY\nSTRICT Q AUDIT CANCEL PASS:all13 construction/local/READY phases cancel and close genuine original Q/renderer children before poisoned former table walks; revision-only stale and permitted selection; SOFTWARE_ONLY\nSTRICT Q AUDIT CALLBACK PASS:10 known-alias no-release,8 allocating reentry/stale/read-only groups, genuine sequence retained after setup-release outer fault, transferred BUSY release and two exact-request NULL refusals without retries; SOFTWARE_ONLY\nSTRICT QUANTIZED AUDIT PASS:standalone genuine every-boundary Q traversal; no composite hook/master promotion/cache/key/enqueue/device/native/timing/audio/listening authority\n'


def fingerprint(path):
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode):
        raise RuntimeError('ordinary file required: ' + str(path))
    if getattr(info, 'st_flags', 0) & 0x40000000:
        raise RuntimeError('resident ordinary file required: ' + str(path))
    body = path.read_bytes()
    after = path.lstat()
    if (info.st_dev, info.st_ino, info.st_mode, info.st_size, info.st_mtime_ns) != (
            after.st_dev, after.st_ino, after.st_mode, after.st_size, after.st_mtime_ns):
        raise RuntimeError('ordinary input changed during read: ' + str(path))
    return {'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest(),
            'mode': info.st_mode}


def inventory(folder):
    return {str(p.relative_to(folder)): fingerprint(p)
            for p in sorted(folder.rglob('*')) if not p.is_dir()}


def constants(folder, name, key):
    # Parse literal source declarations without importing/executing old tests.
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
            if node.target.id not in values:
                raise RuntimeError('literal source augmentation lacks prior declaration')
            values[node.target.id] += ast.literal_eval(node.value)
    value = values[key]
    if not isinstance(value, list) or any(not isinstance(p, str) for p in value):
        raise RuntimeError('literal ordered source declaration required')
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', required=True,
                        help='Exact forty-hex committed baseline explicitly selected by root')
    parser.add_argument('--attempt-id', required=True, help='New root-selected once-only attempt name; existing directory refuses')
    args = parser.parse_args()
    if not re.fullmatch('[a-z0-9][a-z0-9-]{0,63}', args.attempt_id):
        parser.error('--attempt-id requires1..64 lowercase letters/digits/hyphens')
    if not re.fullmatch('[0-9a-f]{40}', args.baseline):
        parser.error('--baseline requires an exact forty-hex commit')
    baseline = args.baseline
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    attempt = EVIDENCE / ('attempt-' + args.attempt_id)
    if attempt.exists():
        raise RuntimeError('First attempt evidence preserved: no automatic retry')
    attempt.mkdir()
    record = attempt / 'run.json'
    started = time.monotonic()
    overall_deadline = started + 600
    report = {
        'status': 'PREPARATION_ATTEMPTED', 'phase': 'PREPARATION',
        'baseline_argument': baseline, 'baseline_default': BASELINE_STATUS, 'attempt': str(attempt),
        'scope': 'HOST standalone strict every-boundary quantized geometry owner with genuine renderer/pinned source lifetime, two control requests and SAME-sequence transfer; four descriptive renderer queries only. No composite editor hook, source promotion/cache/key/enqueue, native/emulator/physical/device/stack/timing/audio/listening authority.',
        'overall_call_budget_seconds': 600, 'preparation_calls': [], 'calls': [],
        'first_failure': None,
    }

    def persist():
        record.write_text(json.dumps(report, indent=2) + '\n')

    def first_failure(label, error):
        if report['first_failure'] is None:
            report['first_failure'] = {'label': label, 'error': repr(error),
                                       'exception_type': type(error).__name__}

    # Persist BEFORE initial Git/source/head/index/archive/tool reads. Preparation
    # failures are actual first failures with raw partial logs, never NOT RUN.
    persist()
    environment = {
        'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'LANG': 'C', 'LC_ALL': 'C',
        'TMPDIR': str(attempt / 'tmp'), 'GIT_OPTIONAL_LOCKS': '0',
        'GIT_CONFIG_NOSYSTEM': '1', 'GIT_CONFIG_GLOBAL': '/dev/null',
        'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
        'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1',
    }
    def group_exists(pgid):
        try:
            os.killpg(pgid, 0)
            return True
        except ProcessLookupError:
            return False

    def call(label, argv, cwd, limit, collection='calls'):
        start = time.monotonic()
        deadline = min(start + limit, overall_deadline)
        stdout = attempt / (label + '.stdout')
        stderr = attempt / (label + '.stderr')
        row = {'label': label, 'argv': argv, 'cwd': str(cwd),
               'whole_call_seconds': limit, 'effective_remaining_seconds': max(0, deadline-start),
               'cleanup_reserve_seconds': 5,
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
                if deadline - time.monotonic() <= 5:
                    raise RuntimeError('overall deadline leaves no owned cleanup reserve')
                process = subprocess.Popen(argv, cwd=cwd, stdout=out, stderr=err,
                                           env=environment, start_new_session=True)
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
        if cleanup_error is not None:
            row['cleanup_error'] = repr(cleanup_error)
            if error is None:
                error = cleanup_error
        if process is not None and error is None and (process.returncode or row['stderr_snapshot']['bytes']):
            error = RuntimeError(label + ' nonzero return or stderr')
        # Include the required successful raw-output read and complete log
        # fingerprints before accepting recorded completion. Preserve an earlier
        # driver/cleanup failure rather than replacing it with a later overrun.
        # Synchronous filesystem latency is not hard real-time enforcement.
        captured_stdout = stdout.read_bytes() if error is None else None
        row['elapsed_seconds'] = time.monotonic() - start
        row['completion_within_effective_deadline'] = (
            row['elapsed_seconds'] <= row['effective_remaining_seconds'])
        if not row['completion_within_effective_deadline'] and error is None:
            error = RuntimeError(label + ' recorded completion exceeded effective call deadline')
        if error is not None:
            row.update(status='FAIL', error=repr(error), exception_type=type(error).__name__)
            first_failure(label, error)
            persist()
            raise error
        row['status'] = 'PASS'
        persist()
        return captured_stdout

    def git(label, *args):
        return call(label, ['/usr/bin/git', *args], ROOT, 30, 'preparation_calls')

    def controls_unchanged(label):
        if fingerprint(Path(compiler)) != {k: v for k, v in report['compiler'].items() if k != 'path'}:
            raise RuntimeError('compiler input changed')
        if git(label + '-head', 'rev-parse', 'HEAD').decode().strip() != baseline:
            raise RuntimeError('HEAD changed')
        if git(label + '-index', 'ls-files', '--stage') != index:
            raise RuntimeError('index changed')
        if {p: fingerprint(ROOT / p) for p in PROTECTED} != protected:
            raise RuntimeError('protected work changed')
        if {p: fingerprint(DRAFT / p) for p in OWN} != overlays:
            raise RuntimeError('declared overlay changed')
        if {p: fingerprint(ROOT / p) for p in committed} != live_committed:
            raise RuntimeError('current committed input changed')
        if inventory(source) != inputs:
            raise RuntimeError('saved source inventory changed')

    try:
        (attempt / 'tmp').mkdir()
        report['environment'] = environment
        report['interpreter'] = {'path': str(Path(sys.executable).resolve()),
                                 **fingerprint(Path(sys.executable).resolve())}
        compiler = shutil.which('cc', path=environment['PATH'])
        if compiler is None:
            raise RuntimeError('controlled compiler not found')
        compiler = str(Path(compiler).resolve())
        report['compiler'] = {'path': compiler, **fingerprint(Path(compiler))}
        report['git'] = {'path': '/usr/bin/git', **fingerprint(Path('/usr/bin/git'))}
        persist()
        if git('prepare-head', 'rev-parse', 'HEAD').decode().strip() != baseline:
            raise RuntimeError('current HEAD differs from explicitly selected baseline')
        index = git('prepare-index', 'ls-files', '--stage')
        if git('prepare-staged', 'diff', '--cached', '--name-only', '-z'):
            raise RuntimeError('index contains staged changes')
        status = git('prepare-status', 'status', '--porcelain=v1', '-z',
                     '--untracked-files=all', '--', 'src', 'tests', 'AGENTS.md',
                     'amiga-test.json', 'tools')
        status_rows = status.decode().split('\0')
        allowed = set(PROTECTED)
        for row in status_rows:
            if row and (len(row) < 4 or row[:2] not in (' M', '??') or row[3:] not in allowed):
                raise RuntimeError('unrelated dirty/adoptable input refused: ' + repr(row))
        protected = {p: fingerprint(ROOT / p) for p in PROTECTED}
        overlays = {p: fingerprint(DRAFT / p) for p in OWN}
        report.update(protected=protected, overlays=overlays,
                      status_rows=[r for r in status_rows if r],
                      index_sha256=hashlib.sha256(index).hexdigest())
        persist()
        archive = git('prepare-archive', 'archive', baseline, 'src', 'tests')
        report['archive'] = {'bytes': len(archive), 'sha256': hashlib.sha256(archive).hexdigest()}
        (attempt / 'baseline-src-tests.tar').write_bytes(archive)
        source = attempt / 'source'
        source.mkdir()
        with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
            members = stream.getmembers()
            names = set()
            for member in members:
                path = Path(member.name)
                if (not (member.isdir() or member.isfile()) or path.is_absolute() or
                        '..' in path.parts or not path.parts or
                        path.parts[0] not in ('src', 'tests') or member.name in names):
                    raise RuntimeError('fixed committed archive member refused')
                names.add(member.name)
            stream.extractall(source, filter='data')
        archived = inventory(source)
        if set(ADDITIVE) & set(archived):
            raise RuntimeError('additive overlay already adopted by selected baseline')
        if not set(MODIFIED + list(REQUIRED_COMMITTED_EXPECTED)).issubset(archived):
            raise RuntimeError('Root-selected baseline must already commit genuine Q normalizer, D1 and unchanged renderer/control origins')
        for name, expected in {**REQUIRED_COMMITTED_EXPECTED, **MODIFIED_BASE}.items():
            actual = archived[name]
            if {k: actual[k] for k in ('bytes', 'sha256')} != expected:
                raise RuntimeError('frozen Q/render source-origin differs: ' + name)
        committed = sorted(archived)
        live_committed = {p: fingerprint(ROOT / p) for p in committed}
        for name in committed:
            a, current = archived[name], live_committed[name]
            if name not in PROTECTED and (a['bytes'], a['sha256'], a['mode'] & 0o111) != (
                    current['bytes'], current['sha256'], current['mode'] & 0o111):
                raise RuntimeError('unrelated current input differs from baseline: ' + name)
        for name in OWN:
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes((DRAFT / name).read_bytes())
            path.chmod(overlays[name]['mode'] & 0o777)
            if fingerprint(path) != overlays[name]:
                raise RuntimeError('overlay freeze mismatch: ' + name)
        font = git('prepare-font', 'show', baseline + ':vendor/pt23f/raw/ptfont.raw')
        if len(font) != 580:
            raise RuntimeError('unexpected committed font size')
        report['font'] = {'bytes': len(font), 'sha256': hashlib.sha256(font).hexdigest(),
                          'purpose': 'controlled committed generated include, no visual acceptance'}
        (source / 'pt_font.h').write_text('static const unsigned char pt_font[580] = {' + ','.join(str(b) for b in font) + '};\n')
        sampler = constants(source, 'tests/test_sampler.py', 'SOURCES')[1:]
        preflight = constants(source, 'tests/test_paula_preflight.py', 'SOURCES')[1:]
        dispatch = constants(source, 'tests/test_wavetable_dispatch.py', 'DISPATCH')
        # Fixture includes the exact private audit C to inspect only genuine
        # retained child/result identities. Do not also link that C separately.
        units = list(dict.fromkeys([
            'tests/mixed_quantized_audit_test.c',
            'src/editor/mixed_readers_plan_normalize.c', 'src/core/amigus_trigger_levels.c',
            'src/editor/mixed_preflight.c', *preflight, *dispatch, 'src/editor/sampler_wavetable.c',
            *['src/core/' + n + '.c' for n in ['amigus_reservation', 'amigus_wavetable_cache',
                'amigus_sample_ram', 'sample_cache', 'playback_pcm']], *sampler,
        ]))
        for name in units:
            if name not in archived and name not in OWN:
                raise RuntimeError('translation unit outside declared frozen input: ' + name)
        flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                 '-fsanitize=address,undefined', '-Isrc/core', '-I.']
        inputs = inventory(source)
        if len(inputs) != len(archived) + len(ADDITIVE) + 1:
            raise RuntimeError('derived complete source count differs from archive plus four declared additions and committed font')
        report.update(status='PREPARED_NOT_RUN', phase='COMPILE', inputs=inputs,
                      archived_file_count=len(archived), source_file_count=len(inputs),
                      modified_overlay_count=len(MODIFIED), additive_overlay_count=len(ADDITIVE),
                      required_committed_source_origin_pins=REQUIRED_COMMITTED_EXPECTED, query_only_renderer_original_pins=MODIFIED_BASE, source_origin_commit=SOURCE_ORIGIN,
                      ordered_units=units, unit_count=len(units), flags=flags,
                      live_committed=live_committed,
                      expected_stdout={'bytes': len(EXPECTED_STDOUT),
                                       'sha256': hashlib.sha256(EXPECTED_STDOUT).hexdigest()})
        persist()
        # Fresh custody check BEFORE starting either actual driver/product. No
        # preparation replay can qualify old failures, old source or prior runs.
        controls_unchanged('precompile')
        executable = attempt / 'strict-qaudit'
        call('strict-qaudit-compile', [compiler, *flags, *units, '-o', str(executable)], source, 120)
        report['product'] = fingerprint(executable)
        controls_unchanged('postcompile')
        controls_unchanged('prerun')
        report['phase'] = 'RUN'
        persist()
        stdout = call('strict-qaudit-run', [str(executable)], source, 180)
        if stdout != EXPECTED_STDOUT:
            raise RuntimeError('full seven-marker output differs from exact standalone strict audit fixture contract')
        report['phase'] = 'VERIFY_CUSTODY'
        persist()
        controls_unchanged('postrun')
        controls_unchanged('verify')
        if fingerprint(Path(compiler)) != {k: v for k, v in report['compiler'].items() if k != 'path'}:
            raise RuntimeError('compiler input changed')
        if fingerprint(executable) != report['product']:
            raise RuntimeError('product changed')
        # Required custody/compiler/product checks are complete before sampling
        # elapsed admission. No PASS may carry an observed overall overrun.
        report['elapsed_seconds'] = time.monotonic() - started
        if report['elapsed_seconds'] > report['overall_call_budget_seconds']:
            raise RuntimeError('overall completion exceeded600 seconds after custody checks')
        report.update(status='PASS', phase='COMPLETE', marker_count=7)
    except BaseException as error:
        first_failure(report['phase'], error)
        report.update(status='FAIL', error=repr(error), exception_type=type(error).__name__)
        raise
    finally:
        report['elapsed_seconds'] = time.monotonic() - started
        report['overall_completion_within_budget'] = (
            report['elapsed_seconds'] <= report['overall_call_budget_seconds'])
        completion_error = None
        if not report['overall_completion_within_budget'] and report['first_failure'] is None:
            completion_error = RuntimeError('recorded overall completion exceeded600 seconds')
            first_failure('OVERALL_COMPLETION_BOUND', completion_error)
            report.update(status='FAIL', phase='OVERALL_COMPLETION_BOUND',
                          error=repr(completion_error), exception_type=type(completion_error).__name__)
        # Preserve any earlier actual failure and raw logs. Final persistence is
        # synchronous: no hard real-time bound on filesystem latency is claimed.
        persist()
        print(record, flush=True)
        if completion_error is not None:
            raise completion_error


if __name__ == '__main__':
    main()
