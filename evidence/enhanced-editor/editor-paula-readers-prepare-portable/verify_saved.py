"""Pure-stdlib verifier of bundled saved bytes and their exact relationships.

No imports of builders, no subprocess, target, guard, SDK or source-pool reads.
The excluded executable's original byte observation is evidence, not a replay.
"""
from pathlib import Path
import hashlib
import json
import stat

BASE = Path(__file__).resolve().parent
EXPECTED_HEAD = '86cb65fdef27a19082a4d469acee8aa3bb39e0bb'
EXPECTED_PRODUCT = {'bytes': 469568, 'sha256': '971dda5d3c3670375e6f518f6728e6452cbbb51ad4131b937985052f679f875c'}
EXPECTED_STDOUT = {'bytes': 755, 'sha256': 'facfb3c2fe6d7c70cb89b496cb92bdb5830dfde8266ad227641671405c998e3d'}
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os', '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-Isrc/core', '-I.', '-fbbb=-']
OWN = ['src/editor/editor_paula_readers_prepare.h', 'src/editor/editor_paula_readers_prepare.c', 'tests/editor_paula_readers_prepare_test.c', 'tests/test_editor_paula_readers_prepare.py']
OLD_HASHES = ['9addce76226bbed3f371b02ac4fa5a5355fe7d1ae605439a5021dd48c1e89b85', 'c761cbf9554a618598fbd91b8d1eadee0eab4b81c981c3c611b0dae979a7fc11', 'c9059e2f4ab0cb10403d715070eb3902eab0446a373b7f747900287df7372fe9', 'cc5b308250cb4a227bb762eeb347b21718b0cd87d00a140354b891ea39f77232']
NEW_HASHES = [OLD_HASHES[0], 'e532997750fccb046eeb36b607811769cca431b51e69fa6f77af48fe7b2ff721', '3dd596b2afda2d558ab03b07bf3b910902e8dd0e9abc607806d2b36adeaf40a0', OLD_HASHES[3]]


def data(relative):
    relative = Path(relative)
    assert not relative.is_absolute() and '..' not in relative.parts
    path = BASE / relative
    before = path.lstat()
    assert stat.S_ISREG(before.st_mode) and not (getattr(before, 'st_flags', 0) & 0x40000000)
    assert path.resolve().is_relative_to(BASE)
    contents = path.read_bytes()
    after = path.lstat()
    assert (before.st_dev, before.st_ino, before.st_mode, before.st_size, before.st_mtime_ns) == (after.st_dev, after.st_ino, after.st_mode, after.st_size, after.st_mtime_ns)
    assert len(contents) == before.st_size
    return contents


def fingerprint(relative):
    contents = data(relative)
    return {'bytes': len(contents), 'sha256': hashlib.sha256(contents).hexdigest()}


def compact(pin):
    return {key: pin[key] for key in ('bytes', 'sha256')}


def load(relative):
    return json.loads(data(relative))


def dependencies(manifest):
    prefix = str(Path(manifest['builder']['path']).parent / 'source') + '/'
    result = {}
    for path, pin in manifest['dependencies'].items():
        key = 'FROZEN/' + path[len(prefix):] if path.startswith(prefix) else path
        assert key not in result
        result[key] = compact(pin)
        if path.startswith(prefix):
            assert compact(pin) == manifest['source_before'][path[len(prefix):]]
        assert pin['ordinary_resident'] and not (pin['flags'] & 0x40000000)
    return result


def main():
    packet = load('packet-manifest.json')
    assert packet['status'] == 'PUBLIC_SAVED_EVIDENCE_HOST_COMPILER_ONLY_NATIVE_NOT_RUN'
    assert packet['baseline'] == EXPECTED_HEAD
    assert packet['native_entry'] == packet['constructor_execution'] == packet['emulator_execution'] == packet['physical_execution'] == 'NOT_RUN'
    assert compact(packet['corrected_current']['product']) == EXPECTED_PRODUCT
    assert not packet['corrected_current']['product']['bundled']
    assert packet['corrected_current']['product']['execution'] == 'NEVER_EXECUTED'
    assert compact(packet['expected_stdout']) == EXPECTED_STDOUT
    assert packet['expected_stdout']['marker_count'] == 3
    assert packet['bounds'] == {'each_compiler_call_seconds': 120, 'overall_seconds': 600}
    assert packet['counts'] == {'ordered_units_per_attempt': 80, 'frozen_inputs_per_attempt': 919, 'dependencies_per_attempt': 230, 'compiler_calls_per_attempt': 89, 'host_calls_per_attempt': 6, 'total_host_calls': 12, 'preserved_first_failure_files_in_original': 1111}
    assets = packet['assets']
    for relative, pin in assets.items():
        assert fingerprint(relative) == compact(pin), 'Changed bundled bytes: ' + relative
        assert not relative.endswith(('.c', '.h', '.o', '.a')) and '/source/' not in relative
        assert Path(relative).name != 'PTEditorPaulaReadersPrepareTest'
    observed = set()
    for path in BASE.rglob('*'):
        mode = path.lstat().st_mode
        assert stat.S_ISDIR(mode) or stat.S_ISREG(mode)
        if stat.S_ISREG(mode):
            observed.add(path.relative_to(BASE).as_posix())
    assert observed - {'packet-manifest.json', 'verification.json', 'verification.stderr'} == set(assets)
    checks = ['All declared ordinary saved assets match exact bytes; no executable, object, SDK headers or source pool bundled.']
    manifests = {}
    for public, state, hashes in [('first-failure', 'FIRST_FAILURE_RETAINED', OLD_HASHES), ('corrected-current', 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED', NEW_HASHES)]:
        m = load(public + '/manifest.json')
        manifests[public] = m
        assert m['status'] == state and m['host_baseline'] == EXPECTED_HEAD
        assert m['source_count'] == len(m['source_before']) == 919
        assert m['source_stable'] and m['source_before'] == m['source_after']
        assert m['frozen_source86_plus_four_verified']
        assert len(m['protected_16_before']) == 16
        assert m['flags'] == FLAGS
        assert all(m[key] == 'NEVER_EXECUTED' for key in ['target_execution', 'emulator_execution', 'physical_execution', 'native_execution'])
        assert m['bounds']['each_compiler_call_seconds'] == 120 and m['bounds']['overall_seconds'] == 600
        assert m['overall_elapsed_seconds'] < 600
        controls = load(public + '/current-source.json')
        assert controls['baseline'] == EXPECTED_HEAD and controls['status'] == 'PASS'
        assert list(controls['files']) == OWN and controls['files'] == m['current_own_four']
        for path, expected_hash in zip(OWN, hashes):
            pin = controls['files'][path]
            assert pin['saved_equal'] and pin['sha256'] == expected_hash
            assert compact(pin) == m['source_before'][path]
        host = load(public + '/host-run.json')
        assert host['head'] == EXPECTED_HEAD and host['status'] == 'PASS'
        assert len(host['calls']) == 6 and all(c['status'] == 'PASS' and c['returncode'] == 0 for c in host['calls'])
        assert host['inputs'] == m['source_before']
        assert fingerprint(public + '/host-run.json') == compact(m['host_evidence'])
        assert controls['run'] == m['host_evidence']['path']
        for call in host['calls']:
            for stream in ('stdout', 'stderr'):
                key = call['label'] + '.' + stream
                assert fingerprint(public + '/host/' + key) == m['host_attempt_custody'][key]
            assert fingerprint(public + '/host/' + call['label'] + '.stderr')['bytes'] == 0
        units = [arg for arg in next(c for c in host['calls'] if c['label'] == 'controller-compile')['argv'] if arg.endswith('.c')]
        assert units == m['ordered_units'] and len(units) == len(set(units)) == m['unit_count'] == 80
        assert units[:2] == ['tests/editor_paula_readers_prepare_test.c', 'src/editor/editor_paula_readers_prepare.c']
        assert 'tests/paula_readers_song_test.c' not in units
        assert len(m['dependencies']) == m['dependency_count'] == 230
        assert len(m['tools']) == 5 and len(m['runtime_inputs']) == 7
        assert len(m['calls']) == m['compiler_call_count'] == 89
        labels = ['compiler-version'] + ['runtime-' + name.replace('.', '-') for name in m['runtime_inputs']] + ['dependency-%02d' % i for i in range(80)] + ['compile-link']
        assert [c['label'] for c in m['calls']] == labels
        for index, call in enumerate(m['calls']):
            assert call['product_execution'] == 'NEVER_EXECUTED' and call['timeout_seconds'] <= 120
            assert call['status'] == ('FIRST_FAILURE_RETAINED' if public == 'first-failure' and index == 88 else 'PASS')
            assert call['returncode'] == (1 if public == 'first-failure' and index == 88 else 0)
            for stream in ('stdout', 'stderr'):
                rel = public + '/native/' + call['label'] + '.' + stream
                assert fingerprint(rel) == compact(call[stream])
            if index < 88 or public == 'corrected-current':
                assert call['stderr']['bytes'] == 0
        assert m['calls'][-1]['argv'][1:-2] == [*FLAGS, *units]
        assert fingerprint(public + '/expected-native.stdout') == compact(m['expected_stdout']) == EXPECTED_STDOUT
        expected = data(public + '/expected-native.stdout')
        assert expected == data(public + '/host/controller-run.stdout') and len(expected.splitlines()) == 3
        for marker in [b'EDITOR PAULA READERS PREPARE PASS:72 ', b'PAULA READERS SONG PASS:', b'PAULA READERS SONG FAULTS PASS:']:
            assert marker in expected
        assert fingerprint(public + '/build_native_once.py.reference') == compact(m['builder'])
        checks.append(public + ': complete six-call host record, 80 ordered units, 919 inputs, 230 dependencies, 89 compiler argv/log pairs and all three expected markers match.')
    old, new = manifests['first-failure'], manifests['corrected-current']
    assert 'product' not in old and old['calls'][-1]['label'] == 'compile-link'
    diagnostic = data('first-failure/native/compile-link.stderr').decode()
    for site in ['tests/editor_paula_readers_prepare_test.c:361:', 'tests/editor_paula_readers_prepare_test.c:391:', 'src/editor/editor_paula_readers_prepare.c:18:', 'src/editor/editor_paula_readers_prepare.c:300:']:
        assert site in diagnostic
    assert '-Werror=misleading-indentation' in diagnostic
    oldproof = load('first-failure/first-failure-verification.json')
    assert oldproof['status'] == 'PASS_INDEPENDENT_FIRST_FAILURE_CUSTODY_NO_CANDIDATE'
    assert oldproof['candidate'] == 'ABSENT_NEVER_EXECUTED'
    assert compact(oldproof['manifest']) == fingerprint('first-failure/manifest.json')
    assert oldproof['first_failure']['returncode'] == 1 and compact(oldproof['first_failure']['diagnostic']) == fingerprint('first-failure/native/compile-link.stderr')
    assert new['protected_16_stable'] and new['original_host_evidence_stable'] and new['four_overlays_stable'] and new['git_head_index_stable']
    assert new['ordered_units'] == old['ordered_units'] and new['protected_16_before'] == old['protected_16_before']
    assert new['tools'] == old['tools'] and new['runtime_inputs'] == old['runtime_inputs']
    assert set(new['source_before']) == set(old['source_before'])
    changed = [path for path in new['source_before'] if new['source_before'][path] != old['source_before'][path]]
    assert sorted(changed) == sorted([OWN[1], OWN[2]])
    assert all(key in dependencies(new) for key in dependencies(old))
    changed_deps = [key for key, pin in dependencies(new).items() if pin != dependencies(old)[key]]
    assert sorted(changed_deps) == sorted(['FROZEN/' + OWN[1], 'FROZEN/' + OWN[2]])
    assert compact(new['product']) == EXPECTED_PRODUCT and new['product']['execution'] == 'NEVER_EXECUTED'
    currentproof = load('corrected-current/saved-byte-verification.json')
    assert currentproof['status'] == 'PASS_INDEPENDENT_SAVED_BYTES_COMPLETE_PORTABLE_FIXTURE_NEVER_EXECUTED'
    assert compact(currentproof['manifest']) == fingerprint('corrected-current/manifest.json')
    assert compact(currentproof['product']) == EXPECTED_PRODUCT and currentproof['product_execution'] == 'NEVER_EXECUTED'
    assert compact(currentproof['expected_stdout']) == EXPECTED_STDOUT
    prior = load('corrected-current/prior-portable-v1-custody.json')
    assert prior['status'] == 'OBSERVED_READ_ONLY_PRIOR_FIRST_FAILURE_CUSTODY'
    assert len(prior['inventory']) == prior['count'] == new['prior_portable_v1_count'] == currentproof['prior_portable_v1_count'] == 1111
    assert new['prior_portable_v1_first_failure_custody_stable']
    assert fingerprint('corrected-current/prior-portable-v1-custody.json') == compact(new['prior_portable_v1_custody'])
    for rel, pin in assets.items():
        if rel.startswith('first-failure/'):
            oldrel = rel[len('first-failure/'):]
            if oldrel.startswith('host/'):
                continue
            if oldrel.endswith('.reference'):
                oldrel = oldrel[:-len('.reference')]
            assert oldrel in prior['inventory'] and compact(pin) == prior['inventory'][oldrel]
    checks.append('Original four-site compiler failure remains distinct; corrected C/test are the only changed frozen inputs and compiler dependencies; all 1111 prior files retain saved custody.')
    checks.append('Candidate fingerprint agrees across original PASS build, independent audit and packet; native constructor/entry/emulator/physical remain NOT_RUN. No candidate bytes are bundled or opened.')
    result = {'status': 'PASS_PUBLIC_SAVED_BYTES_AND_RELATIONSHIPS_ONLY', 'scope': 'Pure stdlib local bundled bytes only; no candidate execution or external input access.',
              'packet_manifest': fingerprint('packet-manifest.json'), 'asset_count': len(assets), 'asset_bytes': sum(pin['bytes'] for pin in assets.values()),
              'baseline': EXPECTED_HEAD, 'source_count_per_attempt': 919, 'ordered_units_per_attempt': 80, 'dependencies_per_attempt': 230, 'compiler_calls_per_attempt': 89,
              'host_calls_total': 12, 'product_metadata': EXPECTED_PRODUCT, 'product_bundled': False, 'native_entry': 'NOT_RUN', 'constructor_execution': 'NOT_RUN',
              'expected_stdout': EXPECTED_STDOUT, 'bounds': packet['bounds'], 'checks': checks}
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
