"""Pure saved-byte/relationship checks. No imports or execution of products."""
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
BASELINE = 'c6783d79ea679bdc8acff3d38ee1c66f1564c9ed'
OWN = ['src/core/mixed_scheduled_readers.h', 'src/core/mixed_scheduled_readers.c',
       'tests/mixed_scheduled_readers_test.c', 'tests/test_mixed_scheduled_readers.py']
LABELS = ['paired-compile', 'paired-run', 'abi2-compile', 'abi2-run']
FLAGS = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
         '-fsanitize=address,undefined', '-Isrc/core', '-I.']
UNITS = ['tests/mixed_scheduled_readers_test.c', 'src/core/mixed_scheduled_readers.c',
         'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c',
         'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c',
         'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c',
         'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c',
         'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c',
         'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c',
         'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c',
         'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c',
         'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c',
         'src/core/playback_pcm.c']
PAIRED = (
    'MIXED READERS PASS:12 genuine16-action paired lifetimes;20 constructor/180 typed admissions;'
    '78 callback/effect/proof cases;24 explicit clock/refusal/pending/cancellation groups;'
    '6 malformed-reader envelopes;18 endian/loop/padding groups;6 full alias,32-reader '
    'capacity/replacement and control/STOP groups; expired controls/consumed close0; '
    'exact common grid/master saves; SOFTWARE_ONLY\n'
    'amigus reservation lifecycle: PASS (fake library, no hardware)\n'
    'WAVETABLE UPLOAD JOB PASS: bounded steps, cancellation, unpublished leases, partial words, '
    'live ownership loss and hit transfer; injected only\n'
    'AMIGUS WAVETABLE OWNER PASS: explicit resource, pinned cache lifetime, failure cleanup, '
    'lost ownership refusal; fake library/bus only\n')
ABI2 = ('SCHEDULED READERS PASS: independent command detach and persistent reader retirement; '
        '20 controls with command capacity2\n')


def need(value, message):
    if not value:
        raise ValueError(message)


def fingerprint(path):
    need(path.is_file() and not path.is_symlink(), 'regular file required: ' + str(path))
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
            'mode': path.stat().st_mode & 0o777}


def load(name):
    return json.loads((HERE / name).read_text())


def canonical(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def brief(value):
    return {key: value[key] for key in ('bytes', 'sha256', 'mode')}


def main():
    manifest = load('saved-manifest.json')
    summary = load('summary.json')
    need(manifest['schema'] == 'mixed-scheduled-readers-host-saved-manifest/v1', 'manifest schema')
    need(summary['schema'] == 'mixed-scheduled-readers-host-summary/v1', 'summary schema')
    need(manifest['status'] == summary['status'] == 'HOST_ONLY_PASS', 'scope/status')
    need(summary['baseline'] == BASELINE and summary['own'] == OWN, 'baseline/overlay controls')
    need(summary['orderedUnits'] == UNITS and len(UNITS) == 26, 'ordered translation units')
    excluded = {'saved-manifest.json', 'verification.json'}
    need(manifest['excludedSelfAndResult'] == sorted(excluded), 'self/result exclusions')
    actual = {str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file()}
    need(actual - excluded == set(manifest['files']), 'complete saved file inventory')
    for name, item in manifest['files'].items():
        need(not Path(name).is_absolute() and '..' not in Path(name).parts, 'local saved path')
        need(fingerprint(HERE / name) == item['savedCopy'], 'saved byte/mode mismatch: ' + name)
        if name in summary['provenance']:
            source = summary['provenance'][name]
            need(item['source'] == source['source'], 'provenance path: ' + name)
            need(item['sourceFingerprint'] == source['sourceFingerprint'] == item['savedCopy'],
                 'saved/source fingerprint: ' + name)
    need(set(summary['provenance']) <= set(manifest['files']), 'all provenance saved')
    need(len(summary['attempts']) == 8, 'eight distinct attempts')
    need(len({a['attempt'] for a in summary['attempts']}) == 8, 'unique attempts')
    current = load('current/run.json')
    need(current['attempt'].endswith('/attempt-oxjh_ms9'), 'current exact attempt')
    base_map = {k: v for k, v in current['inputs'].items() if k not in OWN + ['pt_font.h']}
    need(len(base_map) == summary['baselineSourceCount'] == 918, 'baseline inventory count')
    need(len(current['protected']) == 16, 'protected controls count')
    records = {}
    calls = 0
    for entry in summary['attempts']:
        folder = entry['folder']
        run = load(folder + '/run.json')
        records[entry['attempt']] = run
        need(run['attempt'].endswith('/' + entry['attempt']), 'attempt relationship: ' + folder)
        need(run['head'] == BASELINE and 'HOST_ONLY' in run['scope'], 'host baseline/scope')
        need(run['status'] == entry['status'] and len(run['calls']) == entry['calls'], 'attempt outcome')
        need(len(run['inputs']) == summary['sourceCount'] == 923, 'complete frozen input map')
        need(set(run['inputs']) == set(base_map) | set(OWN) | {'pt_font.h'}, 'exact source set')
        need({k: run['inputs'][k] for k in base_map} == base_map, 'same committed baseline controls')
        need(run['inputs']['pt_font.h'] == current['inputs']['pt_font.h'], 'same derived font')
        need(run['protected'] == current['protected'], 'same protected controls')
        for name in OWN:
            need(fingerprint(HERE / folder / 'source' / name) == run['inputs'][name],
                 'own frozen source relationship: ' + folder + '/' + name)
        need([c['label'] for c in run['calls']] == LABELS[:len(run['calls'])], 'ordered call labels')
        for call in run['calls']:
            calls += 1
            need(call['timeout_seconds'] == (120 if call['label'].endswith('compile') else 180), 'bounded call')
            need(call['status'] == ('PASS' if call['returncode'] == 0 else 'FAIL'), 'actual return status')
            need(call['elapsed_seconds'] >= 0, 'saved elapsed duration')
            for suffix in ('stdout', 'stderr'):
                need(folder + '/' + call['label'] + '.' + suffix in manifest['files'], 'complete actual logs')
        if entry['role'] == 'first failure':
            need(run['status'] == 'FAIL' and run['calls'][-1]['returncode'] != 0, 'first failure retained')
            need(all(c['returncode'] == 0 for c in run['calls'][:-1]), 'stop at first failed call')
        else:
            need(run['status'] == 'PASS' and len(run['calls']) == 4, 'full passing qualifier')
            need(all(c['returncode'] == 0 for c in run['calls']), 'passing calls')
            for call in run['calls']:
                need((HERE / folder / (call['label'] + '.stderr')).read_bytes() == b'', 'empty passing stderr')
            need(run['calls'][0]['argv'] == ['cc', *FLAGS, *UNITS, '-o', run['attempt'] + '/paired'], 'paired compile argv')
            need(run['calls'][1]['argv'] == [run['attempt'] + '/paired'], 'paired run argv')
            need(run['calls'][2]['argv'] == ['cc', *FLAGS, 'tests/scheduled_readers_test.c',
                 'src/core/elapsed_clock.c', '-o', run['attempt'] + '/abi2'], 'ABI2 white-box compile argv')
            need(run['calls'][3]['argv'] == [run['attempt'] + '/abi2'], 'ABI2 run argv')
            need((HERE / folder / 'abi2-run.stdout').read_text() == ABI2, 'ABI2 complete marker')
    need(calls == 23, '23 actual saved calls')
    need(sum(a['role'] == 'first failure' for a in summary['attempts']) == 4, 'four actual first failures')
    need(sum(a['role'] == 'historical PASS' for a in summary['attempts']) == 3, 'three historical passes')
    need((HERE / 'current/paired-run.stdout').read_text() == PAIRED, 'complete current four-line stdout')
    for label in ('paired-compile', 'abi2-compile'):
        need((HERE / 'current' / (label + '.stdout')).read_bytes() == b'', 'empty current compiler stdout')
    controls = load(summary['currentControls'])
    need(controls['status'] == 'PASS' and controls['baseline'] == BASELINE, 'root current controls')
    need(controls['run'] == current['attempt'] + '/run.json' and controls['source_count'] == 923, 'root run/source controls')
    need(set(controls['files']) == set(OWN), 'root four controls')
    for name in OWN:
        need(brief(controls['files'][name]) == current['inputs'][name] and controls['files'][name]['saved_equal'] is True,
             'root current source equality')
    review = load(summary['review'])
    need(review['status'] == 'PASS_INDEPENDENT_FINAL_MIXED_READERS_HOST_REVIEW_NO_NATIVE_OR_TARGET_ADMISSION', 'final review status')
    need(review['reviewed_head'] == BASELINE and review['concrete_blockers'] == [], 'review baseline/blockers')
    need(review['reviewed_files'] == {k: current['inputs'][k] for k in OWN}, 'review exact four source controls')
    need(brief(review['host_record']) == fingerprint(HERE / 'current/run.json'), 'review host record byte/mode')
    need(review['host_record']['path'] == controls['run'] and review['host_record']['calls'] == current['calls'], 'review exact run/calls')
    need(review['custody']['protected_paths'] == current['protected'], 'review protected controls')
    need(review['custody']['input_inventory_canonical_sha256'] == canonical(current['inputs']), 'review full input map')
    for flag in ('all_923_frozen_inputs_and_saved_permission_modes_match',
                 'exact_current_four_files_match_frozen_successful_attempt',
                 'all_16_current_protected_fingerprints_match_saved_protected_map',
                 '918_archived_baseline_src_tests_bytepins_match_exact_c678_baseline',
                 'font_matches_exact_baseline_raw_font_derivation',
                 'source_set_is_exact_918_baseline_plus_four_additives_plus_derived_font'):
        need(review['custody'][flag] is True, 'independent custody: ' + flag)
    saved_review = fingerprint(HERE / summary['review'])
    need({k: controls['finalReview'][k] for k in ('bytes', 'sha256')} ==
         {k: saved_review[k] for k in ('bytes', 'sha256')}, 'root final review byte pin')
    for preserved in review['preserved_attempts']:
        entry = next(a for a in summary['attempts'] if a['attempt'] == preserved['attempt'])
        need(preserved['record'] == fingerprint(HERE / entry['folder'] / 'run.json'), 'review historical record pin')
        need(preserved['status'] == records[entry['attempt']]['status'], 'review historical status')
    old = load(summary['historicalControls'])
    oldrun = records['attempt-gi0vqwpn']
    need(old['status'] == 'PASS' and old['baseline'] == BASELINE and old['source_count'] == 923, 'old control status')
    need(old['run'] == oldrun['attempt'] + '/run.json', 'historical control run')
    for name in OWN:
        need(brief(old['files'][name]) == oldrun['inputs'][name] and old['files'][name]['saved_equal'] is True, 'old source controls')
    withdrawal = load(summary['precompilerWithdrawal'])
    need(withdrawal['status'] == 'WITHDRAWN_BEFORE_COMPILER_SOURCE_CORRECTION_PENDING', 'actual withdrawal status')
    need(withdrawal['compilerCalls'] == withdrawal['targetActions'] == 0, 'withdrawn zero actions')
    need(withdrawal['productCreated'] is False and withdrawal['productExecution'] is False, 'withdrawn no product/execution')
    oldfp = fingerprint(HERE / summary['historicalControls'])
    need({k: withdrawal['historicalControls'][k] for k in ('bytes', 'sha256')} ==
         {k: oldfp[k] for k in ('bytes', 'sha256')}, 'withdrawal historical controls byte pin')
    print(json.dumps({'status': 'PASS_SAVED_BYTES_AND_RELATIONSHIPS',
                      'scope': 'saved HOST_ONLY artifacts; no compilation, execution, operational imports or target action',
                      'filesVerified': len(manifest['files']), 'attemptsVerified': 8,
                      'actualCallsVerified': calls, 'currentCallsPassed': 4,
                      'firstFailureRecordsPreserved': 4, 'historicalPassRecordsPreserved': 3,
                      'sourceInputsPerAttempt': 923, 'ownSourceOverlaysPerAttempt': 4,
                      'protectedControlsPerAttempt': 16, 'orderedPairedUnits': 26,
                      'manifest': fingerprint(HERE / 'saved-manifest.json')}, indent=2))


if __name__ == '__main__':
    main()
