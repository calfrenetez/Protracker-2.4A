"""Inspect archived saved evidence only; no extraction, imports of helpers or execution.

External compiler/runtime/SDK files are not bundled or reopened. Their captured
identities are bound to archived pins and independent actual saved-byte audits.
Per-function stack reports do not qualify a complete native launcher stack.
"""
import argparse
import ast
import hashlib
import io
import json
import posixpath
import re
import shlex
import struct
import tarfile
from pathlib import Path, PurePosixPath

BASE = 'eed27e97aa40fa92641f92b2e9012a4329a06f40'
TREE = '780b2d6b71007b189d826fdb92ddf075766f3723'
OWNED = ['src/editor/paula_readers_song.c', 'src/editor/paula_readers_song.h',
         'tests/paula_readers_song_workspace_test.c', 'tests/test_paula_readers_song_workspace.py']
MODULES = ['test_paula_readers_song_workspace', 'test_paula_readers_song']
HOST_FLAGS = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
              '-fsanitize=address,undefined', '-Isrc/core']
NATIVE_FLAGS = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
                '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', '-fbbb=-']
MARKERS = [
    'PAULA READERS SONG PASS: exact whole-song schedule, independent actual domains and immutable retained masters; host software only',
    'PAULA READERS SONG FAULTS PASS: explicit acceptance, independent fault drain and original deadlines; host software only',
    'PAULA READERS SONG WORKSPACE PASS: guarded caller scratch released before genuine exact scheduling and independent domain ownership; host software only']
NATIVE_NAME = 'PTPaulaReadersSongWorkspaceTest'
RUNTIME_NAMES = ['ncrt0.o', 'libnix20.a', 'libnixmain.a', 'libnix.a',
                 'libstubs.a', 'libamiga.a', 'libgcc.a']


def sha(b):
    return hashlib.sha256(b).hexdigest()


def safe(n):
    p = PurePosixPath(n)
    return bool(n) and not p.is_absolute() and p.as_posix() == n and '\\' not in n and all(x not in ('', '.', '..') for x in n.split('/'))


def enabled(argv):
    return not any(x.startswith('-DNDEBUG') or (x == '-D' and i + 1 < len(argv) and argv[i + 1].split('=', 1)[0] == 'NDEBUG') for i, x in enumerate(argv))


def su_rows(b):
    rows = []
    for line in b.decode().splitlines():
        f = line.split('\t')
        assert len(f) == 3 and f[0] and re.fullmatch('[0-9]+', f[1])
        q = f[2].split(',')
        assert len(set(q)) == len(q) and set(q) in ({'static'}, {'dynamic'}, {'dynamic', 'bounded'})
        rows.append({'compiler_location_and_function': f[0], 'reported_bytes': int(f[1]),
                     'qualifiers': q, 'numeric_local_bound': 'static' in q or 'bounded' in q})
    assert rows
    return rows


def main():
    if not __debug__:
        raise RuntimeError('Optimized evidence-reader invocation refused')
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('directory', nargs='?', type=Path, default=Path(__file__).resolve().parent)
    d = ap.parse_args().directory
    m = json.loads((d / 'manifest.json').read_bytes())
    payload = (d / 'payload.tar.gz').read_bytes()
    assert m['base_commit'] == BASE and m['base_tree'] == TREE
    assert m['payload'] == {'bytes': len(payload), 'sha256': sha(payload)}
    expected = {f['path']: f for f in m['files']}
    assert len(expected) == len(m['files']) and all(safe(n) for n in expected)
    assert all(f['mode'] == 0o644 and type(f['bytes']) is int and 0 <= f['bytes'] <= 64 * 1024 * 1024 for f in expected.values())
    assert sum(f['bytes'] for f in expected.values()) <= 512 * 1024 * 1024
    data = {}
    with tarfile.open(fileobj=io.BytesIO(payload), mode='r:gz') as tf:
        for f in tf:
            assert safe(f.name) and f.isfile() and not f.issym() and not f.islnk()
            assert f.name in expected and f.name not in data and f.mode == expected[f.name]['mode'] == 0o644
            assert f.size == expected[f.name]['bytes']
            b = tf.extractfile(f).read()
            assert len(b) == f.size == expected[f.name]['bytes'] and sha(b) == expected[f.name]['sha256']
            data[f.name] = b
    assert set(data) == set(expected)
    assert not any('readers-startup-independent-packet-review' in n for n in data)
    assert not any(n.startswith('failed-source/') or n.startswith('proof/host-failed') for n in data)
    assert data['preparation/root/verify_workspace_payload_v3.py'] == Path(__file__).read_bytes()

    def load(n):
        return json.loads(data[n])

    h = load('proof/host-final-v3/host-checks.json')
    n = load('proof/native-final-v3/manifest.json')
    s = load('proof/stack-final-v3/manifest.json')
    source_root = posixpath.normpath(h['source_export'])
    root = source_root.rsplit('/', 1)[0]
    proof_roots = {posixpath.dirname(h['log']['path']): 'proof/host-final-v3',
                   posixpath.dirname(n['compile_commands']['path']): 'proof/native-final-v3',
                   posixpath.dirname(s['translation_units'][0]['object']['path']): 'proof/stack-final-v3'}

    def member(path):
        path = posixpath.normpath(path)
        for original, prefix in proof_roots.items():
            if path.startswith(original + '/'):
                return prefix + path[len(original):]
        if path.startswith(source_root + '/'):
            return 'source/' + path[len(source_root) + 1:]
        if path.startswith(root + '/'):
            rel = path[len(root) + 1:]
            for lane in ('host-final-v3', 'native-final-v3', 'stack-final-v3'):
                if rel.startswith(lane + '/'):
                    return 'proof/' + rel
            for lane in ('host-tools-v1', 'native-preparation', 'stack-preparation', 'annotation-independent-preparation'):
                if rel.startswith(lane + '/'):
                    return 'preparation/' + rel
            if '/' not in rel:
                return ('preparation/root/' if rel.endswith('.py') else 'records/') + rel
        raise ValueError('Unbundled record must not be read as an archived file: ' + path)

    def bind(f, name=None):
        name = name or member(f['path'])
        assert len(data[name]) == f['bytes'] and sha(data[name]) == f['sha256'], name
        return data[name]

    # Preserve the exact first packet and its refused reader as history; this
    # does not execute, reinterpret as PASS, or silently replace that reader.
    refused = load('records/independent-workspace-packet-reader-first-refusal-classification-v1.json')
    assert refused['status'] == 'FAILED_READER_REFUSAL' and refused['reader_attempts'] == 1 and not refused['product_failures']
    refusal = load('records/packet-reader-first-refusal-v1.json')
    assert refusal['writer_first_invocation']['returncode'] == 0 and refusal['reader_first_invocation']['returncode'] == 1
    assert "assert n['runtime_paths'][runtime] in n['runtime_inputs']" in refusal['reader_first_invocation']['output']
    bind(refused['original_refusal_record'])
    history_prefix = 'packet-history/first-reader-refusal-v1/'
    assert {name[len(history_prefix):] for name in data if name.startswith(history_prefix)} == {'README.md', 'manifest.json', 'payload.tar.gz', 'verify_payload.py'}
    for name, record in refused['original_packet_files'].items(): bind(record, history_prefix + name)
    history_manifest = load(history_prefix + 'manifest.json')
    assert history_manifest['payload'] == {'bytes': len(data[history_prefix + 'payload.tar.gz']), 'sha256': sha(data[history_prefix + 'payload.tar.gz'])}
    assert data[history_prefix + 'verify_payload.py'] == data['preparation/root/verify_workspace_payload_v2.py']

    maps = {v: load('records/frozen-source-host-v%d.json' % v)['source'] for v in (1, 2, 3)}
    assert all(len(v) == 8122 for v in maps.values())
    assert maps[3] == load('records/frozen-source-native-v3.json')['files']
    baseline = load('records/baseline-manifest.json')
    assert baseline['base_commit'] == BASE and baseline['base_tree'] == TREE and len(baseline['files']) == 8120
    assert set(maps[3]) - set(baseline['files']) == set(OWNED[2:])
    assert {k for k, v in baseline['files'].items() if maps[3][k] != v['sha256']} == set(OWNED[:2])
    assert {k for k in maps[3] if maps[3][k] != maps[2][k]} == {OWNED[0]}
    assert {k for k in maps[2] if maps[2][k] != maps[1][k]} == {OWNED[2]}
    for v in (1, 2):
        for rel in OWNED:
            assert sha(data['unexecuted-source/v%d/' % v + rel]) == maps[v][rel]
    for name, b in data.items():
        if name.startswith('source/'):
            rel = name[7:]
            assert safe(rel) and sha(b) == maps[3][rel]
            if rel in baseline['files'] and rel not in OWNED:
                assert hashlib.sha1(b'blob ' + str(len(b)).encode() + b'\0' + b).hexdigest() == baseline['files'][rel]['git_blob']
    fingerprint = sha(json.dumps(maps[3], sort_keys=True, separators=(',', ':')).encode())

    def closure(log, directory, recorded_system, sdk_alias=None):
        canonical, external = {}, {}
        text = log.decode().replace('\\\n', ' ')
        for dep in shlex.split(text.split(':', 1)[1]):
            p = posixpath.normpath(dep if dep.startswith('/') else posixpath.join(directory, dep))
            # Only this captured host SDK spelling is rebound to its archived
            # pinned SDK root. Original audits resolved it; no external path is
            # resolved or reopened by this reader.
            if sdk_alias and p.startswith(sdk_alias[0] + '/'):
                p = sdk_alias[1] + p[len(sdk_alias[0]):]
            if p.startswith(source_root + '/'):
                rel = p[len(source_root) + 1:]
                assert sha(data['source/' + rel]) == maps[3][rel]
                canonical[rel] = maps[3][rel]
            else:
                assert p in recorded_system and re.fullmatch('[0-9a-f]{64}', recorded_system[p])
                external[p] = recorded_system[p]
        return canonical, external

    host_pins = load('preparation/host-tools-v1/toolchain-pins.json')['host']
    assert list(host_pins['sdk_inputs']) == ['/Library/Developer/CommandLineTools/SDKs/MacOSX27.0.sdk/SDKSettings.json']
    sdk_root = posixpath.dirname(next(iter(host_pins['sdk_inputs'])))
    sdk_alias = ('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk', sdk_root)
    assert h['passed'] and h['modules'] == MODULES and [g['module'] for g in h['groups']] == MODULES
    assert all(g['passed'] and g['tests'] == 1 and not g['errors'] and not g['failures'] for g in h['groups'])
    assert h['source_files_before'] == h['source_files_after'] == 8122 and h['export_tree_sha256'] == fingerprint
    assert h['base_commit'] == BASE and h['base_tree'] == TREE
    assert h['compiler'] == host_pins['compiler'] and h['python'] == host_pins['python']
    assert h['sanitizer_runtime_inputs'] == host_pins['runtime_inputs'] and h['pinned_sdk_inputs'] == host_pins['sdk_inputs']
    for f in h['helper_inputs'] + h['module_inputs'] + [h['source_manifest'], h['pins'], h['log']]:
        bind(f)
    contract = load('preparation/native-preparation/PTPaulaReadersSongWorkspaceTest-contract-v1.json')
    units = contract['ordered_sources']
    assert len(set(units)) == len(units) == 29 and contract['target'] == NATIVE_NAME and contract['fixture'] == OWNED[2]
    assert len(h['commands']) == 4 and len(h['products']) == 2 and not h['generated_inputs']
    hc, he, host_queries = {}, {}, 0
    for i in range(2):
        build, run = h['commands'][2 * i:2 * i + 2]
        order = units if not i else ['tests/paula_readers_song_test.c', *units[1:]]
        assert build['stage'] == run['stage'] == 'complete' and build['product_returncode'] == run['product_returncode'] == 0
        assert build['group'] == run['group'] == MODULES[i] and build['directory'] == run['directory'] == source_root
        assert build['executed_argv'] == [h['compiler']['path'], *HOST_FLAGS, *order, '-o', build['argv'][-1]] and enabled(build['executed_argv'])
        assert build['canonical_inputs'] == [{'path': rel, 'sha256': maps[3][rel]} for rel in order] and not build['generated_inputs']
        assert len(build['dependency_commands']) == 29
        cc, ce = {}, {}
        for unit, q in zip(order, build['dependency_commands']):
            host_queries += 1
            assert q['returncode'] == 0 and q['unit'] == unit and q['directory'] == source_root
            assert q['argv'] == [h['compiler']['path'], *HOST_FLAGS, '-M', unit]
            c1, e1 = closure(bind(q['output']), q['directory'], build['dependencies']['external_sdk'], sdk_alias)
            cc.update(c1); ce.update(e1)
        assert build['dependencies'] == {'canonical': cc, 'external_sdk': ce, 'generated': {}}
        hc.update(cc); he.update(ce)
        assert not bind(build['output_log'])
        artifact = build['produced_binary']
        assert artifact == run['input_binary'] == h['products'][i] and artifact['first_command'] == build['id']
        output = posixpath.normpath(build['executed_argv'][-1])
        assert output.startswith('/var/folders/')
        assert posixpath.normpath(artifact['original_path']) == '/private' + output
        assert run['argv'] == run['executed_argv'] == [build['executed_argv'][-1]]
        assert not run['dependency_commands'] and not run['canonical_inputs'] and not run['generated_inputs']
        binary = bind({'path': artifact['archive_path'], 'bytes': artifact['bytes'], 'sha256': artifact['sha256']})
        assert b'__assert_rtn' in binary
        expected_markers = MARKERS if not i else MARKERS[:2]
        assert bind(run['output_log']) == ''.join(x + '\n' for x in expected_markers).encode()
        assert all(binary.count(x.encode()) == 1 for x in expected_markers)
    assert host_queries == 58 and hc == h['canonical_dependencies'] and he == h['external_sdk_dependencies'] and len(hc) == 80 and len(he) == 105

    np = load('preparation/native-preparation/toolchain-pins.json')
    assert n['passed'] and n['native_NOT_RUN'] and n['base_commit'] == BASE and n['base_tree'] == TREE
    assert n['source_files_before'] == n['source_files_after'] == 8122 and n['source_fingerprint'] == fingerprint
    assert n['compiler'] == np['native']['compiler'] and n['python'] == np['host']['python'] and n['runtime_inputs'] == np['native']['runtime_inputs'] and len(n['runtime_inputs']) == 7
    for role in ('source_manifest', 'pins', 'fixture_contract', 'builder', 'compile_commands'):
        bind(n[role])
    flags = [x.value for x in ast.parse(bind(n['builder'])).body if isinstance(x, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'FLAGS' for t in x.targets)]
    assert len(flags) == 1 and ast.literal_eval(flags[0]) == n['flags'] == NATIVE_FLAGS and enabled(n['flags'])
    assert list(n['targets']) == [NATIVE_NAME] and len(n['commands']) == 40
    target = n['targets'][NATIVE_NAME]
    assert target['source_inputs'] == units and target['flags'] == NATIVE_FLAGS
    recipe = n['recipe_contract']
    assert recipe['flags'] == NATIVE_FLAGS and recipe['runtime_names'] == RUNTIME_NAMES
    assert recipe['exact_ordered_translation_units'] == units and recipe['include_chain'] == contract['include_chain']
    assert recipe['source_footer_gates'] == MARKERS
    for role in ('committed_flag_recipe', 'committed_runtime_recipe', 'host_declaration',
                 'host_parent_declaration', 'host_inherited_declaration',
                 'host_boundary_declaration', 'host_song_declaration'):
        bind(recipe[role])
    for f in recipe['included_sources']: bind(f)
    assert len(recipe['included_sources']) == 3
    assert b'pt_paula_readers_song_begin(&' in data['source/' + OWNED[2]]
    runtime_labels = ['runtime-' + x.replace('.', '-') for x in RUNTIME_NAMES]
    assert [c['label'] for c in n['commands']] == [
        'compiler-version', 'compiler-target-help', *runtime_labels,
        *['dependencies-%02d' % i for i in range(29)],
        'preprocessed-fixture', NATIVE_NAME + '.build']
    cc_path = n['compiler']['path']
    assert n['commands'][0]['argv'] == [cc_path, '--version']
    assert bind(n['commands'][0]['log']).decode().splitlines()[0] == n['compiler']['version']
    assert n['commands'][1]['argv'] == [cc_path, '--help=target'] and b'-fbbb' in bind(n['commands'][1]['log'])
    for runtime, command in zip(RUNTIME_NAMES, n['commands'][2:9]):
        assert command['argv'] == [cc_path, '-m68000', '-msoft-float', '-mcrt=nix20', '-print-file-name=' + runtime]
        assert posixpath.normpath(bind(command['log']).decode().strip()) == n['runtime_paths'][runtime]
        assert runtime in n['runtime_inputs'] and re.fullmatch('[0-9a-f]{64}', n['runtime_inputs'][runtime])
        assert n['runtime_inputs'][runtime] == np['native']['runtime_inputs'][runtime]
    assert n['commands'][-2]['argv'] == [cc_path, *NATIVE_FLAGS, '-E', '-dD', OWNED[2]]
    nc, ne, native_order = {}, {}, []
    for command in n['commands']:
        assert command['returncode'] == 0 and command['directory'] == source_root and command['argv'][0] == n['compiler']['path']
        b = bind(command['log'])
        if '-M' in command['argv']:
            assert command['argv'][1:-2] == NATIVE_FLAGS and '-MM' not in command['argv']
            native_order.append(command['argv'][-1])
            c1, e1 = closure(b, source_root, target['external_sdk_dependencies']); nc.update(c1); ne.update(e1)
    assert native_order == units and nc == target['dependencies'] and ne == target['external_sdk_dependencies'] and len(nc) == 79 and len(ne) == 32
    edge = load(member(n['compile_commands']['path']))['commands']
    assert len(edge) == 1 and edge[0]['ordered_translation_units'] == units
    assert edge[0]['argv'] == n['commands'][-1]['argv'] == [n['compiler']['path'], *NATIVE_FLAGS, *units, '-o', target['binary']['path']]
    binary, cpp = bind(target['binary']), bind(target['preprocessed_fixture'])
    assert len(binary) == 273684 and struct.unpack('>I', binary[:4])[0] == 0x3f3 and not bind(target['build_log'])
    assert cpp.count(b'__assert_func (') == 425 and '__assert_func' in target['last_assert_macro']
    assert re.findall(rb'^#define assert\([^\n]*', cpp, re.MULTILINE)[-1].decode() == target['last_assert_macro']
    assert all(binary.count(x.encode()) == cpp.count(x.encode()) == 1 for x in MARKERS)

    assert s['annotation_complete'] and not s['stack_qualified'] and s['total_030_stack'] == s['WCET'] == 'UNKNOWN'
    assert s['native_NOT_RUN'] and not s['link_performed'] and not s['product_execution_performed']
    assert s['source_files_before'] == s['source_files_after'] == 8122 and s['source_after_matches_before'] and s['source_inventory_fingerprint'] == fingerprint
    assert s['base_commit'] == BASE and s['base_tree'] == TREE and s['recipe_contract']['flags'] == NATIVE_FLAGS
    assert s['recipe_contract']['annotation_flags'] == ['-fstack-usage', '-c'] and s['recipe_contract']['ordered_translation_units'] == units
    for role in ('helper', 'source_manifest', 'stack_pins', 'design', 'functional_builder_origin', 'original_pins'):
        bind(s[role])
    for f in s['current_functional_contract'].values():
        if isinstance(f, dict): bind(f)
    for f in s['recipe_contract']['inputs']: bind(f)
    sp = load(member(s['stack_pins']['path']))
    assert s['tool_pins'] == sp['tools'] and s['tool_pins']['compiler'] == n['compiler'] and s['tool_pins']['python'] == n['python']
    assert s['selected_cc1']['sha256'] == sp['tools']['cc1']['sha256']
    assert s['selected_as']['sha256'] in {sp['tools']['assembler']['sha256'], sp['tools']['assembler_alternate']['sha256']}
    assert posixpath.normpath(s['selected_cc1']['path']) == sp['tools']['cc1']['path']
    assert posixpath.normpath(s['selected_as']['path']) in {sp['tools']['assembler']['path'], sp['tools']['assembler_alternate']['path']}
    assert s['runtime_paths'] == n['runtime_paths']
    assert len(s['commands']) == 131 and len(s['translation_units']) == 29
    by_label = {x['label']: x for x in s['commands']}
    assert len(by_label) == 131
    assert [c['label'] for c in s['commands']] == [
        'compiler-version', 'compiler-target-help', 'selected-cc1', 'selected-as',
        'assembler-version', 'objdump-version', 'nm-version', *runtime_labels,
        *['dependencies-%02d' % i for i in range(29)], 'preprocessed-fixture',
        *[label for i in range(29) for label in
          ('tu%02d-annotation' % i, 'tu%02d-objdump' % i, 'tu%02d-nm' % i)]]
    query_args = [[cc_path, '--version'], [cc_path, '--help=target'],
                  [cc_path, '-print-prog-name=cc1'], [cc_path, '-print-prog-name=as'],
                  [sp['tools']['assembler']['path'], '--version'],
                  [sp['tools']['objdump']['path'], '--version'], [sp['tools']['nm']['path'], '--version']]
    assert [c['argv'] for c in s['commands'][:7]] == query_args
    assert bind(by_label['compiler-version']['stdout']).decode().splitlines()[0] == n['compiler']['version']
    assert b'-fbbb' in bind(by_label['compiler-target-help']['stdout'])
    for role, selected in [('selected-cc1', s['selected_cc1']), ('selected-as', s['selected_as'])]:
        assert posixpath.normpath(bind(by_label[role]['stdout']).decode().strip()) == posixpath.normpath(selected['path'])
    for runtime, command in zip(RUNTIME_NAMES, s['commands'][7:14]):
        assert command['argv'] == [cc_path, '-m68000', '-msoft-float', '-mcrt=nix20', '-print-file-name=' + runtime]
        assert posixpath.normpath(bind(command['stdout']).decode().strip()) == n['runtime_paths'][runtime]
    assert by_label['preprocessed-fixture']['argv'] == [cc_path, *NATIVE_FLAGS, '-E', '-dD', OWNED[2]]
    sc, se, stack_order = {}, {}, []
    for command in s['commands']:
        assert command['returncode'] == 0 and command['directory'] == source_root
        stdout, stderr = bind(command['stdout']), bind(command['stderr'])
        assert not stderr
        if command['label'].startswith('dependencies-'):
            assert command['argv'][0] == n['compiler']['path'] and command['argv'][1:-2] == NATIVE_FLAGS and command['argv'][-2] == '-M'
            stack_order.append(command['argv'][-1])
            c1, e1 = closure(stdout, source_root, s['system_dependencies']); sc.update(c1); se.update(e1)
    assert stack_order == units and sc == s['canonical_dependencies'] == nc and se == s['system_dependencies'] == ne
    stack_cpp = bind(by_label['preprocessed-fixture']['stdout'])
    assert stack_cpp.count(b'__assert_func (') == 425 and all(stack_cpp.count(x.encode()) == 1 for x in MARKERS)
    rows = []
    for index, unit in enumerate(s['translation_units']):
        assert unit['index'] == index and unit['source'] == units[index]
        stem = 'tu%02d' % index
        command = by_label[stem + '-annotation']
        assert command['argv'] == [n['compiler']['path'], *NATIVE_FLAGS, '-fstack-usage', '-c', units[index], '-o', unit['object']['path']]
        assert bind(unit['object']) and not bind(command['stdout'])
        parsed = su_rows(bind(unit['stack_usage']))
        assert parsed == unit['annotations']; rows.extend(parsed)
        disassembly, symbols = bind(unit['disassembly']), bind(unit['symbols'])
        assert 'file format' in disassembly.decode() and 'Disassembly of section' in disassembly.decode() and symbols.strip()
        assert by_label[stem + '-objdump']['argv'] == [sp['tools']['objdump']['path'], '-dr', unit['object']['path']]
        assert by_label[stem + '-nm']['argv'] == [sp['tools']['nm']['path'], '-an', unit['object']['path']]
        assert bind(by_label[stem + '-objdump']['stdout']) == disassembly and bind(by_label[stem + '-nm']['stdout']) == symbols
        assert unit['raw_call_or_relocation_lines'] == [line for line in disassembly.decode().splitlines() if re.search(r'\b(?:jsr|bsr|jmp)\b|\bR_[A-Z0-9_]+\b', line)]
    assert len(rows) == s['function_rows'] == 627 and [x for x in rows if not x['numeric_local_bound']] == s['dynamic_unbounded_rows'] == []
    measured = {}
    for function, size in [('pt_paula_readers_song_begin_in_workspace', 16), ('song_begin_in_workspace', 124), ('pt_paula_readers_song_begin', 70352)]:
        values = [x for x in rows if x['compiler_location_and_function'].rsplit(':', 1)[-1] == function]
        assert len(values) == 1 and values[0]['reported_bytes'] == size
        measured[function] = values[0]
    for f in s['retained_output_files']: bind(f)
    assert len(s['retained_output_files']) == 320

    for lane, report in [('host', h), ('native', n)]:
        audit = load('records/root-%s-audit-v3.json' % lane)
        assert audit['passed']
        bind(audit['source_manifest'])
        bind(audit['report'] if lane == 'host' else audit['products'][0]['report'])
    for name in ('independent-saved-host-proof-review-v3.json', 'independent-saved-native-proof-review-v3.json', 'independent-stack-annotation-review-author-v1.json'):
        review = load('records/' + name)
        assert review['status'].startswith(('PASS', 'CLEAR'))
    peer = load('records/independent-stack-annotation-review-author-v1.json')
    assert peer['integrity_passed'] and peer['annotation_complete'] and not peer['stack_qualified']
    assert peer['total_030_stack'] == peer['WCET'] == 'UNKNOWN' and peer['Stack65536'] == 'NOT_CLEARED' and peer['native_target_NOT_RUN']
    assert peer['source_files'] == 8122 and peer['source_current_unchanged']
    assert peer['commands'] == 131 and peer['commands_all_RC0'] and peer['ordered_TUs'] == units
    assert peer['full_M_queries'] == 29 and peer['full_function_rows'] == 627
    for role in ('annotation_manifest', 'outer', 'source_manifest'): bind(peer[role])
    copies = load('preparation/annotation-independent-preparation/byte-exact-copy-receipt-v1.json')
    assert copies['files'] == len(copies['copies']) == 3 and copies['private_originals_preserved_exact'] and copies['no_reruns_compiler_products_targets']
    for item in copies['copies']:
        original, durable = item['original'], item['durable_copy']
        assert all(original[k] == durable[k] for k in ('bytes', 'sha256', 'mode'))
        bind(durable)
    auditor_copy = next(item for item in copies['copies'] if item['original']['path'] == peer['auditor']['path'])
    assert all(peer['auditor'][k] == auditor_copy['original'][k] for k in ('bytes', 'sha256', 'mode'))
    peer_run = load('preparation/annotation-independent-preparation/audit-execution-v1.json')
    assert peer_run['actual_RC'] == 0 and not peer_run['stderr']
    bind(peer_run['report'])
    for key in ('auditor', 'input_bindings'):
        copy = next(item for item in copies['copies'] if item['original']['path'] == peer_run[key]['path'])
        assert all(peer_run[key][k] == copy['original'][k] for k in ('bytes', 'sha256'))
    peer_controls = load('preparation/annotation-independent-preparation/prepared-input-bindings.json')
    assert peer_controls['source_export'] == source_root and peer_controls['source_files'] == 8122
    assert peer_controls['constructor_sha256'] == maps[3][OWNED[0]] and peer_controls['fixture_sha256'] == maps[3][OWNED[2]]
    for f in peer_controls['controls'].values(): bind(f)
    for lane in ('native', 'stack'):
        outer = load('records/%s-outer-final-v3.json' % lane)
        assert outer['returncode'] == 0 and outer['helper_invocations'] == 1 and outer['controls_before'] == outer['controls_after']
        for f in outer['controls_before'] + [outer['wrapper'], outer['stdout'], outer['stderr']]: bind(f)
        for mapping in outer['durable_mapping']:
            assert bind(mapping['original']) == bind(mapping['durable'])
    required = set(hc) | set(nc) | set(sc) | set(OWNED) | {'tools/build_core_tests.py', 'tools/build_diagnostic.py'}
    todo = ['tests/' + name + '.py' for name in MODULES]
    imports = set(todo)
    while todo:
        rel = todo.pop()
        for node in ast.walk(ast.parse(data['source/' + rel])):
            if isinstance(node, ast.ImportFrom) and node.module and node.module.startswith('test_'):
                dep = 'tests/' + node.module + '.py'
                if dep not in imports: imports.add(dep); todo.append(dep)
    required.update(imports)
    assert {name[7:] for name in data if name.startswith('source/')} == required
    docs = load('documentation/final/docs-source-seal-v3.json')
    assert docs['bound_source'] and len(docs['bound_source']) == 4
    for f in docs['bound_source'] + docs['saved_proof_bindings']: bind(f)
    for f in docs['owned_docs']:
        assert posixpath.dirname(f['path']) == root + '/docs-draft/docs-revision-v3'
        bind(f, 'documentation/final/' + posixpath.basename(f['path']))
    assert load('records/independent-workspace-final-docs-review-author-v1.json')['status'].startswith(('CLEAR', 'PASS'))
    for v in (1, 2, 3):
        freeze = load('records/candidate-freeze-v%d.json' % v)
        assert freeze['target_execution'] == 'NONE' and freeze['protected16_exact'] and freeze['source_files'] == 8122
        assert freeze['base_commit'] == BASE and freeze['base_tree'] == TREE and freeze['owned'] == OWNED
    scope = load('records/scope-final-v3.json')
    assert scope['base_commit'] == BASE and scope['source_files'] == 8122
    assert scope['host'] == {'groups': 2, 'commands_RC0': 4, 'full_M': 58, 'canonical_inputs': 80,
                             'system_inputs': 105, 'retained_products': 2, 'full_runtime_markers': 5, 'state': 'PASS'}
    assert scope['native_compiler']['state'] == 'COMPILER_ONLY_PASS'
    assert {key: scope['native_compiler'][key] for key in ('commands_RC0', 'ordered_full_M', 'canonical_inputs', 'SDK_inputs', 'runtime_inputs')} == {
        'commands_RC0': 40, 'ordered_full_M': 29, 'canonical_inputs': 79, 'SDK_inputs': 32, 'runtime_inputs': 7}
    bind(scope['native_compiler']['HUNK'])
    assert scope['separate_annotation'] == {'commands_RC0': 131, 'ordered_full_M': 29, 'ordinary_objects': 29,
        'compiler_function_rows': 627, 'public_workspace_own_frame_bytes': 16, 'private_constructor_own_frame_bytes': 124,
        'legacy_begin_own_frame_bytes': 70352, 'state': 'PER_FUNCTION_ONLY', 'total_stack': 'UNKNOWN',
        'launcher_65536': 'NOT_CLEARED', 'link': False, 'product_run': False}
    assert all(value == 'NOT_RUN' for value in scope['runtime'].values())
    assert scope['preserved']['unrelated16'] == 'EXACT_UNSTAGED' and scope['preserved']['previous_versions1_2'] == 'UNEXECUTED_SOURCE'
    for f in scope['records']:
        assert safe(f['path']) and '/' not in f['path']
        bind(f, 'records/' + f['path'])
    print(json.dumps({'passed': True, 'members': len(data), 'archived_source_paths': len(required),
                     'full_source_inventory': 8122, 'base_commit': BASE, 'base_tree': TREE,
                     'host_commands_RC0': 4, 'host_full_M': 58, 'host_full_markers': 5,
                     'native_compiler_commands_RC0': 40, 'native_full_M': 29,
                     'annotation_commands_RC0': 131, 'annotation_TUs': 29, 'function_rows': 627,
                     'per_function_observations': measured, 'total_native_stack': 'UNKNOWN',
                     'launcher_65536': 'NOT_CLEARED', 'unexecuted_V1_V2_preserved': True,
                     'first_packet': 'FAILED_READER_REFUSAL; exact original four files preserved',
                     'recorded_host_SDK_alias': dict([sdk_alias]),
                     'scope': 'Archived saved-byte reconstruction only. External tool/runtime/SDK bytes represented by pinned records and actual independent audits, not bundled or reopened. Archive mode0644 normalized separately from original proof modes. No native execution/emulator/physical/placement/IRQ/WCET/timing/audio/listening acceptance.'}, indent=2))


if __name__ == '__main__':
    main()
