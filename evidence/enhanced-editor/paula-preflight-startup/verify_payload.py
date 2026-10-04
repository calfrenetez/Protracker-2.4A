"""Verify saved startup evidence without extraction or compiler/product execution.

Usage: python3 verify_payload.py [package-directory]
This checks archived evidence; it does not independently repeat the recorded runs.
"""
import argparse
import ast
import hashlib
import json
import re
import tarfile
from pathlib import Path, PurePosixPath

if not __debug__:
    raise RuntimeError('Optimized verifier invocation refused')

BASE = '1cabeffdb98c7b8f73356edb10f28141d0eae180'
OWNED = ['src/core/render.c', 'src/core/render.h', 'src/editor/paula_preflight.c',
         'src/editor/paula_preflight.h', 'tests/render_sequence_startup_test.c',
         'tests/test_render_sequence_startup.py', 'tests/paula_preflight_startup_test.c',
         'tests/test_paula_preflight_startup.py']
MODULES = ['test_render_sequence_startup', 'test_paula_preflight_startup',
           'test_paula_preflight', 'test_render_sequence', 'test_render_plan',
           'test_render_invert', 'test_render_range', 'test_sampler_paula_readers_boundary']
FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
         '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', '-fbbb=-']
MARKERS = ['RENDER STARTUP PASS: bounded original validation, exact legacy plans and scan-free checked reset; host software only',
           'PAULA STARTUP PASS: initial readiness, complete timeline audit and same checked sequence transfer; host software only']

def sha(data):
    return hashlib.sha256(data).hexdigest()

def safe(name):
    p = PurePosixPath(name)
    return bool(name) and not p.is_absolute() and p.as_posix() == name and '\\' not in name and all(x not in ('', '.', '..') for x in name.split('/'))

def enabled(argv):
    return not any(a.startswith('-DNDEBUG') or (a == '-D' and i + 1 < len(argv) and argv[i + 1].split('=', 1)[0] == 'NDEBUG') for i, a in enumerate(argv))

def main():
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('package_directory', nargs='?', type=Path, default=Path(__file__).resolve().parent)
    directory = ap.parse_args().package_directory
    manifest = json.loads((directory / 'manifest.json').read_bytes())
    payload = (directory / 'payload.tar.gz').read_bytes()
    assert manifest['base_commit'] == BASE and manifest['payload'] == {'bytes': len(payload), 'sha256': sha(payload)}
    rows = manifest['files']; expected = {r['path']: r for r in rows}
    assert len(expected) == len(rows) and all(safe(n) for n in expected)
    data = {}
    with tarfile.open(directory / 'payload.tar.gz', 'r:gz') as archive:
        for member in archive:
            assert safe(member.name) and member.isfile() and member.name not in data and member.name in expected
            raw = archive.extractfile(member).read()
            assert member.size == len(raw) == expected[member.name]['bytes'] and sha(raw) == expected[member.name]['sha256']
            data[member.name] = raw
    assert set(data) == set(expected)
    def load(name):
        return json.loads(data[name])
    def bind(record, name):
        raw = data[name]
        assert len(raw) == record['bytes'] and sha(raw) == record['sha256'], name
        return raw
    def saved(record, lane):
        path = record.get('archive_path', record.get('path'))
        p = PurePosixPath(path)
        suffix = p.parent.name + '/' + p.name if p.parent.name in ('products', 'generated') else p.name
        return bind(record, 'proof/' + lane + '/' + suffix)
    frozen = load('records/frozen-source-host-v2.json'); native_frozen = load('records/frozen-source-native-v2.json')
    source = frozen['source']; old_frozen = load('records/frozen-source-host-v1.json'); old = old_frozen['source']
    assert old == load('records/frozen-source-native-v1.json')['files'] and old_frozen['owned'] == OWNED
    assert len(source) == len(old) == 8107 and source == native_frozen['files'] and frozen['owned'] == native_frozen['owned'] == OWNED
    assert frozen['base_commit'] == native_frozen['base_commit'] == BASE and frozen['base_tree'] == native_frozen['base_tree'] == manifest['base_tree']
    fingerprint = sha(json.dumps(source, sort_keys=True, separators=(',', ':')).encode())
    assert frozen['source_fingerprint'] == native_frozen['source_fingerprint'] == fingerprint
    assert set(old) == set(source) and [n for n in source if old[n] != source[n]] == ['src/core/render.c']
    baseline = load('records/baseline-manifest.json')['files']; assert len(baseline) == 8103 and set(baseline) <= set(source)
    assert sorted(n for n in source if baseline.get(n, {}).get('sha256') != source[n]) == sorted(OWNED)
    paths = {n[7:] for n in data if n.startswith('source/')}
    for n in paths: assert sha(data['source/' + n]) == source[n]
    assert {n[15:] for n in data if n.startswith('refused-source/')} == set(OWNED)
    for n in OWNED: assert sha(data['refused-source/' + n]) == old[n]
    for n in OWNED[:4]: bind(baseline[n], 'baseline/' + n)
    closure = set(OWNED) | {'tools/build_core_tests.py', 'tools/build_diagnostic.py'}
    host_lane = 'host-host-final-v2'; host = load('proof/' + host_lane + '/host-checks.json')
    ha = load('records/root-host-audit-final-v2.json'); na = load('records/root-native-audit-final-v2.json')
    assert ha['passed'] and na['passed'] and ha['source_files'] == 8107 and ha['owned'] == OWNED and na['native'] == na['physical'] == 'NOT_RUN'
    bind(ha['report'], 'proof/' + host_lane + '/host-checks.json')
    bind(ha['source_manifest'], 'records/frozen-source-host-v2.json'); bind(na['source_manifest'], 'records/frozen-source-native-v2.json')
    assert host['passed'] and host['modules'] == MODULES and [g['module'] for g in host['groups']] == MODULES and host['groups'] == ha['groups']
    assert all(g['passed'] and g['tests'] == 1 and g['failures'] == g['errors'] == 0 for g in host['groups'])
    assert len(host['commands']) == ha['commands'] == 19 and len(host['products']) == ha['products'] == 9 and len(host['generated_inputs']) == 5
    hp = load('preparation/host-tools-v1/toolchain-pins.json')['host']
    assert host['compiler'] == hp['compiler'] and host['python'] == hp['python'] and host['sanitizer_runtime_inputs'] == hp['runtime_inputs'] and host['pinned_sdk_inputs'] == hp['sdk_inputs']
    assert host['source_files_before'] == host['source_files_after'] == 8107 and host['export_tree_sha256'] == fingerprint and host['base_commit'] == BASE and host['base_tree'] == manifest['base_tree']
    bind(host['source_manifest'], 'records/frozen-source-host-v2.json'); bind(host['pins'], 'preparation/host-tools-v1/toolchain-pins.json'); saved(host['log'], host_lane)
    for f in host['helper_inputs']: bind(f, 'preparation/host-tools-v1/' + PurePosixPath(f['path']).name)
    for f in host['module_inputs']: bind(f, 'source/tests/' + PurePosixPath(f['path']).name)
    for f in host['products'] + host['generated_inputs']: saved(f, host_lane)
    queries = 0
    for c in host['commands']:
        assert c['stage'] == 'complete' and c['product_returncode'] == 0 and c['directory'] == host['source_export'] and enabled(c['executed_argv'])
        saved(c['output_log'], host_lane)
        if c['executed_argv'][0] == host['compiler']['path']: assert '-fsanitize=address,undefined' in c['executed_argv']
        for q in c['dependency_commands']:
            assert q['returncode'] == 0 and q['argv'][0] == host['compiler']['path'] and q['argv'][-2:] == ['-M', q['unit']] and '-MM' not in q['argv'] and enabled(q['argv'])
            saved(q['output'], host_lane); queries += 1
        for role in ('produced_binary', 'input_binary', 'produced_object'):
            if role in c: saved(c[role], host_lane)
    assert queries == ha['full_M_queries'] == 127
    for i, module in enumerate(MODULES[:2]):
        fixture = OWNED[4 if i == 0 else 6]
        assert data['source/' + fixture].decode().count('puts("' + MARKERS[i] + '")') == 1
        runs = [c for c in host['commands'] if c['group'] == module and 'input_binary' in c]
        assert len(runs) == 1 and saved(runs[0]['output_log'], host_lane).decode().count(MARKERS[i]) == 1
    for i, (lane, target, count) in enumerate([('native-renderer-final-v2', 'PTRenderStartupTest', 10), ('native-paula-final-v2', 'PTPaulaPreflightStartupTest', 12)]):
        r = load('proof/' + lane + '/manifest.json'); audit = na['products'][i]
        bind(audit['report'], 'proof/' + lane + '/manifest.json')
        assert r['passed'] and r['native_NOT_RUN'] and audit['native_NOT_RUN'] and r['source_files_before'] == r['source_files_after'] == 8107 and r['source_fingerprint'] == fingerprint and r['base_commit'] == BASE and r['base_tree'] == manifest['base_tree']
        bind(r['source_manifest'], 'records/frozen-source-native-v2.json'); bind(r['pins'], 'preparation/native-preparation/toolchain-pins.json')
        pin = load('preparation/native-preparation/toolchain-pins.json')['native']
        assert r['compiler'] == pin['compiler'] and r['runtime_inputs'] == pin['runtime_inputs'] and r['flags'] == FLAGS and enabled(r['flags'])
        bind(r['builder'], 'preparation/native-preparation/' + PurePosixPath(r['builder']['path']).name)
        contract_path = 'preparation/native-preparation/' + target + '-contract-v2.json'
        contract = json.loads(bind(r['fixture_contract'], contract_path)); recipe = r['recipe_contract']; t = r['targets'][target]
        assert set(r['targets']) == {target} and contract['target'] == target and contract['marker'] == MARKERS[i]
        assert len(contract['ordered_sources']) == count and recipe['exact_ordered_translation_units'] == t['source_inputs'] == contract['ordered_sources'] and recipe['include_chain'] == contract['include_chain'] and recipe['flags'] == t['flags'] == FLAGS
        for key in ('committed_flag_recipe', 'committed_runtime_recipe', 'host_declaration', 'host_parent_declaration'):
            f = recipe[key]; bind(f, 'source/' + f['path'].split('/candidate-v2/', 1)[1])
        assert len(r['commands']) == audit['commands_RC0'] == count + 11
        for c in r['commands']:
            assert c['returncode'] == 0 and c['argv'][0] == r['compiler']['path'] and c['directory'] == r['source_export'] and enabled(c['argv'])
            saved(c['log'], lane)
        qs = [c for c in r['commands'] if c['label'].startswith('dependencies-')]
        assert len(qs) == audit['full_M_queries_RC0'] == count and [c['argv'] for c in qs] == [[r['compiler']['path'], *FLAGS, '-M', n] for n in contract['ordered_sources']]
        builds = json.loads(saved(r['compile_commands'], lane))['commands']; build = builds[0]
        assert len(builds) == 1 and build['name'] == target and build['ordered_translation_units'] == contract['ordered_sources'] and build['argv'] == [r['compiler']['path'], *FLAGS, *contract['ordered_sources'], '-o', t['binary']['path']] == r['commands'][-1]['argv']
        binary = saved(t['binary'], lane); assert binary[:4] == b'\x00\x00\x03\xf3' and t['format'] == 'HUNK_HEADER0x000003f3' and t['binary'] == audit['binary']
        macros = re.findall(r'^#define assert\(.*$', saved(t['preprocessed_fixture'], lane).decode(), re.M)
        assert macros and macros[-1] == t['last_assert_macro'] and '__assert_func' in macros[-1] and MARKERS[i] in binary.decode('latin1')
        saved(t['build_log'], lane); closure.update(t['dependencies'])
        for n, h in t['dependencies'].items(): assert source[n] == h
    failure_lane = 'host-initial-compile-refusal-v1'; failed = load('proof/' + failure_lane + '/host-checks.json')
    refusal = load('records/initial-compile-refusal-v1.json')
    assert failed['passed'] is False and len(failed['commands']) == refusal['commands'] == 1 and refusal['groups_completed'] == 0 and refusal['source_unchanged']
    assert failed['source_files_before'] == failed['source_files_after'] == 8107 and failed['export_tree_sha256'] == old_frozen['source_fingerprint'] == sha(json.dumps(old, sort_keys=True, separators=(',', ':')).encode())
    c = failed['commands'][0]; assert c['product_returncode'] != 0 and not failed['products'] and not failed['generated_inputs'] and not any(k in c for k in ('input_binary', 'produced_binary', 'produced_object'))
    assert c['executed_argv'][0] == failed['compiler']['path'] == hp['compiler']['path'] and '-fsanitize=address,undefined' in c['executed_argv']
    assert b'-Wuninitialized-const-pointer' in saved(c['output_log'], failure_lane) and len(c['dependency_commands']) == 10
    bind(failed['source_manifest'], 'records/frozen-source-host-v1.json'); saved(failed['log'], failure_lane)
    for q in c['dependency_commands']: assert q['returncode'] == 0 and '-M' in q['argv']; saved(q['output'], failure_lane)
    for r, hashes in ((host, source), (failed, old)):
        closure.update(r['canonical_dependencies'])
        for n, h in r['canonical_dependencies'].items(): assert hashes[n] == h
    for receipt, lane in ((ha, host_lane), (refusal, failure_lane)):
        for f in receipt['durable_copies']:
            suffix = f['copy'].split('/' + lane + '/', 1)[1]; bind(f, 'proof/' + lane + '/' + suffix)
    imports = {'tests/' + n + '.py' for n in MODULES}; todo = list(imports)
    while todo:
        for node in ast.walk(ast.parse(data['source/' + todo.pop()])):
            if isinstance(node, ast.ImportFrom) and node.module and node.module.startswith('test_'):
                n = 'tests/' + node.module + '.py'
                if n not in imports: imports.add(n); todo.append(n)
    assert paths == closure | imports
    print(json.dumps({'passed': True, 'members': len(data), 'source_inventory': 8107, 'archived_source_paths': len(paths), 'host_groups': 8, 'host_commands_RC0': 19, 'host_full_M': queries, 'host_products': 9, 'host_generated_inputs': 5, 'portable_builds': 2, 'original_compiler_refusal_retained': True, 'scope': 'Saved-byte packet verification only; no extraction, compiler or product execution. Recorded host evidence retained; native/emulator/target/physical/backend activation/audio/listening NOT_RUN. External compiler/SDK/runtime bytes are represented by saved pins, not independently re-read.'}, indent=2))

if __name__ == '__main__':
    main()
