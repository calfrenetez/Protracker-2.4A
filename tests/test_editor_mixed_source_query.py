"""One controlled private SOURCE query HOST qualifier: explicit committed admission, three modified and two additive PRIVATE query overlays.

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
EVIDENCE = ROOT.parents[1] / 'outputs/paired-quantized-song-producer/v1/source-query-host-qualification'
BASELINE_STATUS = 'UNSUPPLIED_ROOT_MUST_SELECT_COMMITTED_BASELINE'
MODIFIED = ['src/editor/editor_mixed_readers_prepare.c', 'src/editor/editor_mixed_readers_source_internal.h', 'tests/editor_mixed_source_borrow_test.c']
ADDITIVE = ['tests/editor_mixed_source_query_test.c', 'tests/test_editor_mixed_source_query.py']
OWN = MODIFIED + ADDITIVE
# These are exact accepted-seam committed source facts, not an executable
# baseline default. Root still supplies the exact live first-attempt admission.
# Historical controller origin is lineage only, never current input authority.
SOURCE_ORIGIN = '0269c20bf97db83f0996d92ed75bd4ff6ab2fe40'
HISTORICAL_CONTROLLER_ORIGIN = 'ee9aa13ca24e1a7a0784f150d9f98d6b7bc330fe'
# Current committed admission must retain the independently adopted audit6.
ADOPTED_AUDIT_ORIGIN = '641c281c43054a63612b001897cb6676e2a5e04d'
REQUIRED_COMMITTED_EXPECTED = {'src/core/amigus_trigger_levels.h': {'bytes': 1886, 'sha256': 'f1db99e67a4b1e2c05ddfe666dc4c69ee640cd5837de33a01bbe88c1f521f6b0'}, 'src/core/amigus_trigger_levels.c': {'bytes': 3763, 'sha256': 'f5a5ecc8e3a5f2583ea01421a8cff66eef11ece2f1987a6f3e04479de63baaf3'}, 'tests/amigus_trigger_levels_test.c': {'bytes': 16223, 'sha256': 'a64502130b44e4f4325beffc029207ba1c8ec4b09b41e59887b7bc9b7bcfdaa4'}, 'tests/test_amigus_trigger_levels.py': {'bytes': 22524, 'sha256': '0d9554affc294dacf08d35a5d23c5e28298da376f5438657058ffd20c9956bab'}, 'src/editor/sampler_mixed_readers.h': {'bytes': 11621, 'sha256': '2a3135a5282721a0452032a220d9f5ba764f1dfc3c192d63720d78baa086e28e'}, 'src/editor/sampler_mixed_readers.c': {'bytes': 44694, 'sha256': 'd5f2d5f4e7574f28b0a9bc70921e4b037802cd837ce60153bb10e09dc591c921'}, 'tests/sampler_mixed_quantized_test.c': {'bytes': 34620, 'sha256': '5e13f252b61758522bf098343b259e2addcdf709e40bc24edae642a0c767ca14'}, 'tests/test_sampler_mixed_quantized.py': {'bytes': 26048, 'sha256': '4e4536d21d28b9fed7d314188bb2636745d5969122d8ade9d68e90d10917c47d'}, 'src/core/mixed_readers_activation.h': {'bytes': 10423, 'sha256': '64edd461b1413fc5b896f7b972b41421c22f04f327c66a21dfbc9b7043394ab5'}, 'src/core/mixed_readers_activation.c': {'bytes': 39498, 'sha256': 'edd96b2ebc4b076e66a865eeaa637e25e0cfe56e480d46c0c4b6746a028d456c'}, 'src/core/mixed_scheduled_readers.h': {'bytes': 11336, 'sha256': 'b24b9e762906bae10d5a511cd949c547715aada7893156e932aeab4315af70c4'}, 'src/core/mixed_scheduled_readers.c': {'bytes': 33837, 'sha256': '2808fbbaf081d48c8ab70c36b57f2882d5bb0677aeb6f218c7a89d3bd0b570df'}, 'tests/sampler_mixed_activation_test.c': {'bytes': 27118, 'sha256': '43ac611f5ccec110002931386e07152af09b0a335eb23f138ce12dc6dde2ad03'}, 'tests/test_editor_mixed_readers_prepare.py': {'bytes': 14109, 'sha256': 'fbccf17297f79ecd8e6f3868053e6d49271b0f4822d2964a9e615b8e038a4161'}, 'tests/editor_mixed_readers_prepare_test.c': {'bytes': 39185, 'sha256': 'ddf2e13cc4e4a9104e344b514aa0a9a6b207f98950835b83b00dba16f2a36178'}, 'tests/editor_mixed_readers_quantized_test.c': {'bytes': 30819, 'sha256': '0e5af87bb819658508766d87f4f8e0565190c2cfaee6bdb2ccba6dcef7eb38a5'}, 'tests/test_editor_mixed_readers_quantized.py': {'bytes': 29731, 'sha256': '1be6673a7f0cb662d56cf18f6fd794afec59eb5de9e4af0c60618b4276b79044'}, 'src/core/render.h': {'bytes': 12179, 'sha256': 'aa531e26bcf3d3fbe6ddfb61a3ea86ae2c1aeb90016e41c4649076a33793bc2f'}, 'src/core/render.c': {'bytes': 63608, 'sha256': 'acc53af54275d5ec481ca2128dcf72fde305bf91ed28295fd781a81f42966936'}, 'src/editor/mixed_quantized_audit.h': {'bytes': 5672, 'sha256': '8ddec0873a6131d4a258845412ea7b69b9d7e6e40562dc1802e63fc992ca9733'}, 'src/editor/mixed_quantized_audit.c': {'bytes': 31119, 'sha256': '24a642896358f49e38d492bbedcbc3d4f6ada083e7f06adc47cbd6cfa4b607d6'}, 'tests/mixed_quantized_audit_test.c': {'bytes': 36372, 'sha256': '6696985872dcd22aeb3a2684b516466fd5bed7de30f3b830126d20d64f180cac'}, 'tests/test_mixed_quantized_audit.py': {'bytes': 26296, 'sha256': 'e45550a2c00d8da9b44809ba26e88487a7df670f4518be2177262e54a8f4c6b0'}, 'src/editor/editor_mixed_readers_prepare.h': {'bytes': 11926, 'sha256': '587047d8f65374f1c361082f8dfbce3150915e537a757f9f51f422981b26d1de'}, 'tests/test_editor_mixed_source_borrow.py': {'bytes': 32887, 'sha256': 'f2bafef93c22e47a4b391f9a10bf4e1531172c5b02545f51e68f3c6e7dec9893'}}
MODIFIED_BASE = {'src/editor/editor_mixed_readers_prepare.c': {'bytes': 65725, 'sha256': 'b8fb10c2b8fc32aa40de71a2be7b945d754001a6a5d4be393e412cb0248e7814'}, 'src/editor/editor_mixed_readers_source_internal.h': {'bytes': 4505, 'sha256': '1e7fb1f2cba3f8a996f819fdf9516e65baa21dc3448d6a94a9220e23910049cc'}, 'tests/editor_mixed_source_borrow_test.c': {'bytes': 40821, 'sha256': '572199c8319119cf56b52c4e77ef39f82e1d61fe8b8939cb0f2d19abfb4bc31e'}}
PROTECTED = [
    'AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h',
    'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c',
    'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c',
    'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py',
    'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py',
]
EXPECTED_STDOUT = b'SAMPLER MIXED FACTORY PASS:24 genuine16-reader paired8/16/24 master lifetimes; real private validation and persistent/TEMP pins; selective signed8 Chip and8/16 card literal endian/channel/padding oracles; exact saves/grid/independent proof orders; constructor/admission/all-phase cancel/source expiry/alias/reentry/fragmented cache-slot2 arena-block0; factory queued effects/unadopted pins/CONTROL+STOP/output guards/budget;6 released caller/current persistent-pin lifetimes;32-reader replacement/pressure and malformed exact-domain drains; SOFTWARE_ONLY\nSAMPLER MIXED ACTIVATION PASS:12 genuine16-reader8/16/24 master/Chip8/card8+16 paired lifetimes; copied whole-packet exact-window activation, CONTROL and STOP; caller-pin expiry, literal precision/endian/channel/padding and exact saves; independent quiet/persistent retention, partial effects and one shutdown with later independent source proof;32-reader replacement pressure preserves new keys, fragmented slot2/block0 and six unpublished full-span output guards; SOFTWARE_ONLY\nEDITOR MIXED READERS PREPARE PASS:12 genuine paired editor 8/16/24 master/Chip8/card8+16 precision/channel/endian lifetimes; original-window CONTROL/STOP only from positive ACTIVE keys; actual edit/undo/route/dispose veto until independent C/R and source quiet; construction/action cancellation, revision-only stale and poisoned former tables, complete unpublished/spare output/input guards, authoritative enqueue transfer, callback faults/partial effects, slot serial reuse and exact master saves; SOFTWARE_ONLY\nMIXED READERS PASS:12 genuine16-action paired lifetimes;20 constructor/180 typed admissions;78 callback/effect/proof cases;24 explicit clock/refusal/pending/cancellation groups;6 malformed-reader envelopes;18 endian/loop/padding groups;6 full alias,32-reader capacity/replacement and control/STOP groups; expired controls/consumed close0; exact common grid/master saves; SOFTWARE_ONLY\namigus reservation lifecycle: PASS (fake library, no hardware)\nWAVETABLE UPLOAD JOB PASS: bounded steps, cancellation, unpublished leases, partial words, live ownership loss and hit transfer; injected only\nAMIGUS WAVETABLE OWNER PASS: explicit resource, pinned cache lifetime, failure cleanup, lost ownership refusal; fake library/bus only\nEDITOR QUANTIZED ADMISSION PASS:44 fixed16 semantic/span refusals;14 genuine legacy/quantized ordinary batch-allocation original/tail/output/spare aliases including earlier copied-fire veto;4 reserve-two refusals,2 exact-two-slot admissions and4 separately labelled numeric suffix/publication oracles; SOFTWARE_ONLY\nEDITOR QUANTIZED OWNERSHIP PASS:48 genuine16-reader 8/16/24 masters with mixed4Paula/12card or16card; exact copied gain pairs, seven-field geometry/endian/channel/padding and matched arena/full-capacity oracles; expired inputs, exact grid/master saves and both proof orders; SOFTWARE_ONLY\nEDITOR QUANTIZED LIFETIME PASS:2 invalid/valid callback reentries;5 genuine CONTROL/STOP tag/copy/raw-key-failure and poisoned-source groups;24 incremental unpublished phase cancels;2 genuine odd8 geometry and2 queried one-byte budget/workspace refusals; actual enqueue transfer under callback fault; unchanged original editor hook/transfer/proof/source-quiet suite invoked once; SOFTWARE_ONLY\nEDITOR QUANTIZED PASS:genuine controller request/ref route plus allocator-span repair in BOTH entries; no new owner/layout/normalizer certificate, full-song producer, native/device/IRQ/timing/audio/listening authority\nEDITOR SOURCE BORROW ADMISSION PASS:18 complete-parent/span/role refusals;5 explicit whole-role refusals and16 later mutable/control/unrelated-immutable/narrowed-role refusals, each followed by genuine SAME-scope exact-role activation; unset future input then exact full-card reuse;3 genuine early8/16/24 bounded promotions and independent current pins; copied control/publisher, stale serial, missing-master activation refusal; unchanged standalone enums/suites once; SOFTWARE_ONLY\nEDITOR SOURCE BORROW EXCLUSION PASS:genuine first master allocation editor reentry retains actual job then cancels once; original/copy busy enter/leave/activation;2 descriptor/tag-poisoned SOURCE cleanup groups;10 full mutable/original/promoted/master/table/publisher/parent allocator aliases;3 real construction-phase cancels; SOFTWARE_ONLY\nEDITOR SOURCE BORROW LIFETIME PASS:real paired quantized original publication/copied-only fire; independent C/R then pending source quiet; failure-plus-NULL child consumption retained and never retried; original borrow release then positive final hook clear;4090 extension activation dedup and255-slot full PCM/marker maximum-source refusal preserve every guard; SOFTWARE_ONLY\nEDITOR SOURCE BORROW PASS:private early fixed-hook ownership seam only; no caller READY/quiet certificate, full song producer/audit integration, new standalone link dependency, timer/PLAY/MMIO/native/device/timing/audio/listening authority\nEDITOR SOURCE QUERY IDENTITY PASS:3 genuine8/16/24 scopes; original/copy/stale/partial/hook refusals; nested held-latch diagnostics and actual zero publisher after controller expiry;2 poisoned-tag/descriptor cleanups; no query mutation/callback/clock/reentry; SOFTWARE_ONLY\nEDITOR SOURCE QUERY SPANS PASS:complete14 caller spans and full original/spare/promoted capacities; numeric-only8192-last-guard/37-ordinary/32-Chip bounds and partial-span refusals; adjacency/full-request-tail oracles; pre-refresh promotion remains genuine sampler ownership; SOFTWARE_ONLY\nEDITOR SOURCE QUERY REGISTRATION PASS:4 genuine paired C/R proof-order/failure-plus-NULL groups; task callback nested queries and PRESENT-during-release then actual ABSENT; original issued refs/later serial reuse;5 separately-labelled partial-record refusal groups; pending source quiet/barrier retained; SOFTWARE_ONLY\nEDITOR SOURCE QUERY PASS:private read-only captured/registration diagnostics only; no new owner/hook/layout/link unit, arbitrary issuance/full-size/READY/ACTIVE/quiet certificate, full song producer or native/device/IRQ/timing/audio/listening authority\n'


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
        'scope': 'Private query increment over accepted early SOURCE seam: read-only original/ZERO diagnostic, bounded numeric captured-only full allocation spans and genuine issued C/R record observations, nested task callbacks, actual NULL/proof/serial reuse. Inherited corrected SOURCE/legacy/quantized suite once. Full size/issuer provenance remain caller obligations; pre-refresh promotions remain genuine sampler/establish ownership. No new owner/hook/layout/link unit, queue/source table query, full producer, READY/ACTIVE/quiet certificate, native/emulator/physical/device/stack/timing/audio authority.',
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
            raise RuntimeError('Root-selected baseline must already commit the accepted SOURCE seam, genuine controller, D1/D2 and adopted audit6 origins')
        for name, expected in {**REQUIRED_COMMITTED_EXPECTED, **MODIFIED_BASE}.items():
            actual = archived[name]
            if {k: actual[k] for k in ('bytes', 'sha256')} != expected:
                raise RuntimeError('frozen accepted-seam/controller/D1/D2/adopted-audit source-origin differs: ' + name)
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
        editor = constants(source, 'tests/test_editor.py', 'SOURCES')[1:]
        studio = constants(source, 'tests/test_editor_studio.py', 'EXTRA')
        dispatch = constants(source, 'tests/test_wavetable_dispatch.py', 'DISPATCH')
        preflight = constants(source, 'tests/test_paula_preflight.py', 'SOURCES')[1:]
        units = list(dict.fromkeys([
            'tests/editor_mixed_source_query_test.c', 'src/editor/editor_mixed_readers_prepare.c',
            'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_activation.c',
            'src/core/amigus_trigger_levels.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c',
            'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c',
            *sampler, *editor, *studio, *dispatch, *preflight,
            *['src/core/' + n + '.c' for n in ['amigus_reservation', 'amigus_wavetable_cache',
                'amigus_sample_ram', 'sample_cache', 'playback_pcm']],
            *['src/editor/' + n + '.c' for n in ['editor_mixed', 'mixed_owner', 'mixed_transport',
                'mixed_preflight', 'paula_voices', 'paula_dispatch', 'editor_wavetable']],
            *['src/platform/' + n + '.c' for n in ['sample_import', 'raw_import', 'mod_import', 'pp20_import']],
        ]))
        for name in units:
            if name not in archived and name not in OWN:
                raise RuntimeError('translation unit outside declared frozen input: ' + name)
        flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                 '-fsanitize=address,undefined', '-Isrc/core', '-I.']
        inputs = inventory(source)
        if len(inputs) != len(archived) + len(ADDITIVE) + 1:
            raise RuntimeError('derived complete source count differs from archive plus two declared additions and committed font')
        report.update(status='PREPARED_NOT_RUN', phase='COMPILE', inputs=inputs,
                      archived_file_count=len(archived), source_file_count=len(inputs),
                      modified_overlay_count=len(MODIFIED), additive_overlay_count=len(ADDITIVE),
                      required_committed_source_origin_pins=REQUIRED_COMMITTED_EXPECTED, modified_controller_original_pins=MODIFIED_BASE, source_origin_commit=SOURCE_ORIGIN, historical_controller_origin_commit=HISTORICAL_CONTROLLER_ORIGIN, adopted_audit_origin_commit=ADOPTED_AUDIT_ORIGIN,
                      ordered_units=units, unit_count=len(units), flags=flags,
                      live_committed=live_committed,
                      expected_stdout={'bytes': len(EXPECTED_STDOUT),
                                       'sha256': hashlib.sha256(EXPECTED_STDOUT).hexdigest()})
        persist()
        # Fresh custody check BEFORE starting either actual driver/product. No
        # preparation replay can qualify old failures, old source or prior runs.
        controls_unchanged('precompile')
        executable = attempt / 'editor-source-query'
        call('editor-source-query-compile', [compiler, *flags, *units, '-o', str(executable)], source, 120)
        report['product'] = fingerprint(executable)
        controls_unchanged('postcompile')
        controls_unchanged('prerun')
        report['phase'] = 'RUN'
        persist()
        stdout = call('editor-source-query-run', [str(executable)], source, 180)
        if stdout != EXPECTED_STDOUT:
            raise RuntimeError('full nineteen-marker output differs from inherited corrected seam/standalone plus exact focused query contract')
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
        report.update(status='PASS', phase='COMPLETE', marker_count=19)
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
