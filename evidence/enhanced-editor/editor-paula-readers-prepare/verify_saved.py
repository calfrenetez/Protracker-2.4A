"""Verify saved host evidence bytes/relationships; no compilation or replay."""
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
BASELINE = '86cb65fdef27a19082a4d469acee8aa3bb39e0bb'
OVERLAYS = {
    'src/editor/editor_paula_readers_prepare.h',
    'src/editor/editor_paula_readers_prepare.c',
    'tests/editor_paula_readers_prepare_test.c',
    'tests/test_editor_paula_readers_prepare.py',
}
FAILURES = {
    'attempt-q6e_cxdc', 'attempt-fvjwo9oo', 'attempt-952kg36t',
    'attempt-u8wb_erb', 'attempt-cvxjvn9x', 'attempt-3ha65ijf',
}
CURRENT = 'corrected-current/'

def read(name):
    return json.loads((HERE / name).read_text())

def fingerprint(path):
    assert path.is_file() and not path.is_symlink(), str(path)
    value = path.read_bytes()
    return {'bytes': len(value), 'sha256': hashlib.sha256(value).hexdigest()}

manifest = read('saved-manifest.json')
assert manifest['status'] == 'PASS_SAVED_BYTE_CUSTODY_ONLY'
actual = {str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file()}
assert actual == set(manifest['files']) | {'saved-manifest.json'}
for name, item in manifest['files'].items():
    assert not Path(name).is_absolute() and '..' not in Path(name).parts, name
    assert fingerprint(HERE / name) == item['savedCopy'], name
    if item['kind'] == 'savedCopy':
        assert item['sourceFingerprint'] == item['savedCopy'], name
    else:
        assert item['kind'] == 'generated', name

run = read(CURRENT + 'run.json')
current = read(CURRENT + 'current-source.json')
summary = read('summary.json')
assert run['status'] == current['status'] == summary['status'] == 'PASS'
assert run['head'] == current['baseline'] == summary['baseline'] == BASELINE
assert len(run['inputs']) == summary['saved_inputs'] == 919
assert len(run['calls']) == 6
assert [c['label'] for c in run['calls']] == [
    'controller-compile', 'controller-run', 'workspace-compile',
    'workspace-run', 'establish-compile', 'establish-run',
]
for call in run['calls']:
    assert call['status'] == 'PASS' and call['returncode'] == 0
    assert (HERE / CURRENT / (call['label'] + '.stderr')).read_bytes() == b''
    assert (HERE / CURRENT / (call['label'] + '.stdout')).is_file()
assert set(current['files']) == set(summary['own_inputs']) == OVERLAYS
for name, item in current['files'].items():
    expected = {k: item[k] for k in ('bytes', 'sha256')}
    assert item['saved_equal'] is True
    assert fingerprint(HERE / CURRENT / 'source' / name) == run['inputs'][name] == expected
    assert summary['own_inputs'][name] == expected

assert set(summary['first_failures']) == FAILURES
for name in FAILURES:
    failed = read('first-failures/' + name + '/run.json')
    assert failed['status'] == 'FAIL' and failed['head'] == BASELINE
    calls = failed['calls']
    assert calls and calls[-1]['status'] == 'FAIL' and calls[-1]['returncode'] != 0
    diagnostic = HERE / 'first-failures' / name / (calls[-1]['label'] + '.stderr')
    assert diagnostic.is_file() and diagnostic.stat().st_size > 0
    assert summary['first_failures'][name]['failed_call'] == calls[-1]['label']

stdout = (HERE / CURRENT / 'controller-run.stdout').read_bytes()
for marker in (
    b'PREPARE PASS:72', b'81 full-span', b'24 real ABI2',
    b'3 full song/oracle', b'3 complete producer/child/representation',
    b'3 poisoned former-table and3 expired-terminal',
    b'PAULA READERS SONG PASS:', b'PAULA READERS SONG FAULTS PASS:',
):
    assert marker in stdout, marker
assert b'PAULA READERS SONG WORKSPACE PASS:' in (HERE / CURRENT / 'workspace-run.stdout').read_bytes()
assert b'EDITOR ESTABLISH PASS:27' in (HERE / CURRENT / 'establish-run.stdout').read_bytes()
assert summary['scope'] == 'HOST_ONLY ASan/UBSan; saved bytes and relationships only'
assert summary['native_execution'] == summary['physical_execution'] == 'NOT_RUN'
assert summary['portable_compile_claim'] == 'NOT_CLAIMED_SEPARATE_WORK'
assert len(summary['assembly_refusals']) == 1
assembly = summary['assembly_refusals'][0]
assert assembly['classification'] == 'ASSEMBLY_REFUSAL_NOT_SOURCE_OR_TEST_FAILURE'
assert assembly['command'] == 'python3 -B evidence/enhanced-editor/editor-paula-readers-prepare/verify_saved.py'
assert assembly['observed_assertion'] == 'AssertionError: verify_saved.py'
assert assembly['original_logs_saved'] is False
assert assembly['original_returncode'] == 'NOT_SAVED'
assert assembly['source'] == 'Reported root tool receipt during active packet assembly'
initial = read('run.json')
initial_source = read('current-source.json')
assert initial['status'] == 'PASS' and initial['head'] == BASELINE and len(initial['inputs']) == 919
assert len(initial['calls']) == 6
for call in initial['calls']:
    assert call['status'] == 'PASS' and call['returncode'] == 0
    assert (HERE / (call['label'] + '.stderr')).read_bytes() == b''
for name, item in initial_source['files'].items():
    expected = {k: item[k] for k in ('bytes', 'sha256')}
    assert fingerprint(HERE / 'source' / name) == initial['inputs'][name] == expected
for name in ('src/editor/editor_paula_readers_prepare.c', 'tests/editor_paula_readers_prepare_test.c'):
    assert initial_source['files'][name]['sha256'] != current['files'][name]['sha256']
for name in OVERLAYS - {'src/editor/editor_paula_readers_prepare.c', 'tests/editor_paula_readers_prepare_test.c'}:
    assert initial_source['files'][name]['sha256'] == current['files'][name]['sha256']
initial_review = read('initial-review/final-source-and-host-review.json')
assert initial_review['status'] == initial_review['coverage_status'] == 'BLOCKER'
assert initial_review['source_review_status'] == initial_review['recorded_host_integrity_status'] == 'PASS'
initial_review_fingerprints = {item['path']: {k: item[k] for k in ('bytes', 'sha256')}
                               for item in initial_review['evidence']['manifest_fingerprints']}
assert initial_review_fingerprints['current-source.json'] == fingerprint(HERE / 'current-source.json')
assert initial_review_fingerprints['attempt-seyoypo8/run.json'] == fingerprint(HERE / 'run.json')
review_file = CURRENT + 'final-source-and-host-review-corrected.json'
review = read(review_file)
assert review['status'] == review['source_review_status'] == review['recorded_host_integrity_status'] == review['coverage_status'] == 'PASS'
assert set(review['files']) == OVERLAYS
for name, item in review['files'].items():
    assert {k: item[k] for k in ('bytes', 'sha256')} == run['inputs'][name]
assert review['evidence']['saved_inputs_checked'] == 919
assert review['evidence']['saved_inputs_all_match'] is True
assert review['evidence']['run'] == 'attempt-rlzcu7ln/run.json'
review_fingerprints = {item['path']: {k: item[k] for k in ('bytes', 'sha256')}
                       for item in review['evidence']['manifest_fingerprints']}
assert review_fingerprints['current-source.json'] == fingerprint(HERE / CURRENT / 'current-source.json')
assert review_fingerprints['attempt-rlzcu7ln/run.json'] == fingerprint(HERE / CURRENT / 'run.json')
assert len(review['evidence']['calls']) == 6
for call in review['evidence']['calls']:
    assert call['status'] == 'PASS' and call['returncode'] == 0 and call['stderr_bytes'] == 0
assert summary['review_file'] == review_file
assert fingerprint(HERE / 'contract_snapshot.md') == summary['contract_fingerprint']
publication = (HERE / 'future-publication-paths.txt').read_text().splitlines()
expected_publication = OVERLAYS | {'docs/EDITOR_PAULA_READERS_PREPARE.md'} | {
    'evidence/enhanced-editor/editor-paula-readers-prepare/' + name
    for name in actual
}
assert len(publication) == len(set(publication)) == 73
assert set(publication) == expected_publication
print('PASS saved HOST_ONLY editor Paula readers evidence: 919 input records, 6 corrected calls, 4 exact overlays, 6 preserved failures and initial BLOCKER review; no replay or target access')
