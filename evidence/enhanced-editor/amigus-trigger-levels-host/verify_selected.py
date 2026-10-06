"""Read-only standard-library verifier of seven selected D1 HOST evidence files.

No omitted artifact proof, test replay, live process or target authority.
The external frozen publication binds this verifier source itself.
"""
from pathlib import Path
import hashlib
import json
import math
import stat
import sys

if sys.flags.optimize:
    raise RuntimeError('Non-optimized selected-byte verification required')
sys.dont_write_bytecode = True
EXPECTED = {'README.md': {'bytes': 2872, 'sha256': '8d5eaf781ea6a068715c6f50d8fcd2c5bb2250d71dfe9b9edeb72550d84ff988', 'mode': 33188}, 'observations.json': {'bytes': 42125, 'sha256': 'a889ffd5f9eb5ca91448754afcf9f870beb25045e28ce4b118215b46ae7f4b0b', 'mode': 33188}, 'current.stdout': {'bytes': 912, 'sha256': '7abb2a0547eda8c0c7008b511f9a6844583d204cc55e97721e8075c782904604', 'mode': 33188}, 'current.stderr': {'bytes': 0, 'sha256': 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855', 'mode': 33188}, 'first-compile-failure.stdout': {'bytes': 0, 'sha256': 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855', 'mode': 33188}, 'first-compile-failure.stderr': {'bytes': 405, 'sha256': '130aedc44415d19dec873641557aafde5a06d725670af8cbc75cf925cb942b95', 'mode': 33188}}
NAMES = set(EXPECTED) | {'verify_selected.py'}
SUCCESS = b'AMIGUS TRIGGER LEVELS SELECTED HOST PROJECTION PASS:7 ordinary files; six complete current lines;26 current and14 preserved failure driver records; selected bytes only\n'


def read_regular(path, limit=1048576):
    before = path.lstat()
    if not stat.S_ISREG(before.st_mode) or before.st_size > limit:
        raise ValueError('Bounded ordinary selected file required: ' + path.name)
    if getattr(before, 'st_flags', 0) & 0x40000000:
        raise ValueError('Resident selected file required')
    body = path.read_bytes()
    after = path.lstat()
    if (before.st_dev, before.st_ino, before.st_mode, before.st_size, before.st_mtime_ns) != (after.st_dev, after.st_ino, after.st_mode, after.st_size, after.st_mtime_ns):
        raise ValueError('Selected file changed during read')
    return body, {'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest(), 'mode': before.st_mode}


def unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate JSON key')
        result[key] = value
    return result


def require(ok, message):
    if not ok:
        raise ValueError(message)


def finite(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def records(items, labels, current, root):
    require(type(items) is list and [r['label'] for r in items] == labels, 'Exact selected driver membership required')
    for row in items:
        require(type(row['pid']) is int and row['pid'] > 0 and row['pid'] == row['pgid'], 'Saved owned PGID required')
        require(row['owned_new_session'] is True and row['driver_reaped'] is True and row['group_quiescent'] is True, 'Saved positive owned-driver cleanup required')
        require(row['cleanup_reserve_seconds'] == 5 and finite(row['elapsed_seconds']), 'Selected cleanup/elapsed bound required')
        limit = row['whole_seconds'] if root else row['whole_call_seconds']
        effective = limit if root else row['effective_remaining_seconds']
        require(finite(effective) and row['elapsed_seconds'] <= effective <= limit, 'Observed selected completion within effective bound required')
        if not root:
            require(row['completion_within_effective_deadline'] is True, 'Saved measured completion acceptance required')
        if current:
            require(row['status'] == 'PASS' and row['returncode'] == 0 and row['stderr']['bytes'] == 0, 'Current driver RC0/empty-stderr required')


def main():
    require(len(sys.argv) == 1, 'This verifier accepts no external/private file arguments')
    folder = Path(__file__).absolute().parent
    require(stat.S_ISDIR(folder.lstat().st_mode), 'Ordinary packet directory required')
    require({p.name for p in folder.iterdir()} == NAMES, 'Exactly seven selected files required')
    raw = {}
    for name in sorted(NAMES):
        body, observed = read_regular(folder / name)
        if name in EXPECTED:
            require(observed == EXPECTED[name], 'Frozen selected file differs: ' + name)
        raw[name] = body
    obs = json.loads(raw['observations.json'], object_pairs_hook=unique,
                     parse_constant=lambda value: (_ for _ in ()).throw(ValueError('Nonfinite JSON constant')))
    require(obs['schema'] == 'pt24g-amigus-trigger-levels-selected-host-projection-v1' and obs['scope'] == 'D1_PURE_NUMERIC_HELPER_HOST_ONLY', 'Exact narrow projection schema required')
    current, failed = obs['current'], obs['first_compile_failure']
    require(current['attempt'] == 'first-v3' and current['status'] == 'PASS' and current['phase'] == 'COMPLETE' and current['first_failure'] is None, 'Distinct corrected HOST pass required')
    require(current['saved_source_file_count'] == 945 and current['archived_source_file_count'] == 940 and current['translation_unit_count'] == len(current['ordered_units']) == 17, 'Actual saved membership required')
    require((current['preparation_call_count'], current['compile_and_runtime_call_count'], current['root_wrapper_call_count'], current['completed_owned_driver_records']) == (16, 2, 8, 26), 'Actual corrected driver counts required')
    preparation = ['prepare-head', 'prepare-index', 'prepare-staged', 'prepare-status', 'prepare-archive', 'prepare-font', 'precompile-head', 'precompile-index', 'postcompile-head', 'postcompile-index', 'prerun-head', 'prerun-index', 'postrun-head', 'postrun-index', 'verify-head', 'verify-index']
    wrappers = ['before-head', 'before-branch', 'before-staged', 'before-index', 'first-qualifier', 'after-head', 'after-index', 'after-staged']
    records(current['qualifier_records'], preparation + ['trigger-levels-compile', 'trigger-levels-run'], True, False)
    records(current['root_records'], wrappers, True, True)
    require(current['overall_completion_within_budget'] is True and finite(current['qualifier_elapsed_seconds']) and current['qualifier_elapsed_seconds'] <= 600, 'Saved observed whole completion required')
    require(raw['current.stderr'] == b'' and len(raw['current.stdout']) == 912 and raw['current.stdout'].count(b'\n') == 6 and raw['current.stdout'].endswith(b'\n'), 'Complete current raw stream required')
    require(failed['attempt'] == 'first-v2' and failed['status'] == 'FAIL' and failed['phase'] == 'COMPILE' and failed['compiler_returncode'] == 1 and failed['runtime_call_count'] == 0 and failed['host_product_state'] == 'NOT_CREATED_NOT_RUN', 'Original compile failure must remain unqualified')
    require((failed['preparation_call_count'], failed['compiler_call_count'], failed['root_wrapper_call_count']) == (8, 1, 5), 'Original14 driver records required')
    records(failed['qualifier_records'], preparation[:8] + ['trigger-levels-compile'], False, False)
    records(failed['root_records'], wrappers[:5], False, True)
    require(failed['qualifier_records'][-1]['returncode'] == 1 and failed['root_records'][-1]['returncode'] == 1, 'Actual original failure RC required')
    require(raw['first-compile-failure.stdout'] == b'' and len(raw['first-compile-failure.stderr']) == 405 and b"'REFUSE' macro redefined" in raw['first-compile-failure.stderr'], 'Complete original diagnostic required')
    review = obs['independent_saved_host_review']
    require(review['status'] == 'PASS_INDEPENDENT_SAVED_D1_HOST_EVIDENCE_ONLY' and review['findings'] == [] and review['body_copied'] is False, 'Independent saved HOST review binding required')
    require(len(obs['source_four_current']) == len(obs['source_four_first_failure']) == 4, 'Four source origins required')
    authority = obs['authority']
    require(authority['target_operations'] == 0 and all(v is False for k, v in authority.items() if k != 'target_operations'), 'No integration/native/device/timing/audio authority may be inferred')
    require(obs['subsequent_exact_source_adoption']['qualification_preceded_adoption'] is True and obs['subsequent_exact_source_adoption']['source_four_same_bytes'] is True and obs['subsequent_exact_source_adoption']['additional_tests_or_targets'] is False, 'Qualification and later exact adoption must remain distinct')
    sys.stdout.buffer.write(SUCCESS)


if __name__ == '__main__':
    main()
