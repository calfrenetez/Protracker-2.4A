"""Offline saved-byte verifier. No extraction, imports of producers, or subprocesses.

Run only after root seals READY inputs and authorizes the separate packet build.
Archived SDK bytes are verified; this does not reconstruct their live resolution.
"""
import ast
import difflib
import gzip
import hashlib
import io
import json
import posixpath
import re
import shlex
import struct
import tarfile
from pathlib import Path


def require(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def mode(value):
    return int(value, 8) if isinstance(value, str) else value


def safe(name):
    return (isinstance(name, str) and bool(name) and not name.startswith('/')
            and '\\' not in name and all(x not in ('', '.', '..') for x in name.split('/')))


def records(value):
    if isinstance(value, dict):
        if {'path', 'bytes', 'sha256'} <= value.keys():
            yield value
        else:
            for child in value.values():
                yield from records(child)
    elif isinstance(value, list):
        for child in value:
            yield from records(child)


def canonical_payload(data, metadata):
    raw = io.BytesIO()
    with gzip.GzipFile(filename='', fileobj=raw, mode='wb', mtime=0, compresslevel=9) as zipped:
        with tarfile.open(fileobj=zipped, mode='w', format=tarfile.USTAR_FORMAT) as tar:
            for name in sorted(data):
                entry = tarfile.TarInfo(name)
                entry.size = len(data[name])
                entry.mode = metadata[name]['mode']
                entry.uid = entry.gid = entry.mtime = 0
                entry.uname = entry.gname = ''
                tar.addfile(entry, io.BytesIO(data[name]))
    return raw.getvalue()


def check_evidence(manifest, data):
    members, aliases = manifest['members'], manifest['path_members']

    def mapped(record):
        name = aliases[record['path']]
        value = data[name]
        require(len(value) == record['bytes'] and sha(value) == record['sha256'],
                'record hash/size: ' + record['path'])
        if 'mode' in record:
            require(members[name]['mode'] == mode(record['mode']), 'record mode')
        return value

    def jrec(record):
        return json.loads(mapped(record))

    inputs = json.loads(data[manifest['inputs']])
    require(inputs['state'] == 'READY_ACTUAL_REVIEWED_INPUTS', 'inputs still waiting for root review')
    require(inputs['root_review'] is not None and jrec(inputs['root_review'])['status'] == 'CLEAR_PREPARATION_ONLY',
            'root source-only packet preparation review unbound')
    require((manifest['scope'], manifest['base_commit'], manifest['base_tree'])
            == (inputs['scope'], inputs['base_commit'], inputs['base_tree']), 'manifest scope/base')
    require(manifest['acceptance'] == inputs['acceptance'], 'acceptance changed')
    require(inputs['acceptance'] == {
        'host': 'FOUR_GROUPS_ASAN_UBSAN_PASS', 'native_product': 'COMPILER_LINK_ONLY_NEVER_EXECUTED',
        'new_RAM_and_retirement_fixtures_native': 'NOT_BUILT',
        'emulator_physical_IRQ_placement_DMA_audio_WCET': 'UNQUALIFIED',
        'total_stack': 'UNKNOWN;65536_NOT_CLEARED'}, 'qualification scope changed')
    expected_roles = {'root_host', 'independent_host', 'root_native', 'independent_native',
                      'root_documentation', 'independent_documentation', 'independent_source',
                      'root_native_preparation', 'root_schema', 'independent_schema'}
    require(set(inputs['reviews']) == expected_roles, 'missing exact review roles')
    for role, entry in inputs['reviews'].items():
        review = jrec(entry['record'])
        require(review['status'] == entry['status'], 'review status: ' + role)
        if entry.get('require_passed'):
            require(review.get('passed') is True, 'review not passed: ' + role)

    expected_aliases = set()
    for artifact in inputs['artifacts']:
        mapped(artifact)
        expected_aliases.add(artifact['path'])
        if artifact.get('resolved_path'):
            require(aliases[artifact['path']] == aliases[artifact['resolved_path']], 'lexical tool alias')
            expected_aliases.add(artifact['resolved_path'])
    require(len(inputs['artifacts']) == len({x['path'] for x in inputs['artifacts']}), 'duplicate artifact paths')

    frozen = jrec(inputs['frozen_source'])
    fm, base = frozen['source'], frozen['base_inputs']
    require(len(fm) == 225 and len(base) == 219, 'lean source/base size')
    require((frozen['base_commit'], frozen['base_tree']) == (inputs['base_commit'], inputs['base_tree']),
            'clean Git base binding')
    added = set(fm) - set(base)
    changed = {rel for rel in base if fm[rel] != base[rel]['sha256']}
    require(changed == set(inputs['modified_paths']) and added == set(inputs['added_paths']),
            'two changed production files/six additive fixture files')
    require(changed | added == set(inputs['owned_paths']) and len(inputs['owned_paths']) == 8,
            'owned eight paths')
    def expected_patch(paths):
        rows = []
        for rel in paths:
            old = mapped(inputs['production_beforeimages'][rel]).decode().splitlines(True) if rel in changed else []
            new = data[manifest['source_members'][rel]].decode().splitlines(True)
            rows.extend(difflib.unified_diff(old, new, fromfile='a/'+rel if rel in changed else '/dev/null', tofile='b/'+rel))
        return ''.join(rows).encode()
    require(mapped(inputs['owned_patch']) == expected_patch(inputs['owned_paths'])
            and mapped(inputs['production_patch']) == expected_patch(inputs['modified_paths']), 'exact narrow before/after patches')
    require(set(manifest['source_members']) == set(fm), 'all 225 source bytes required')
    for rel, name in manifest['source_members'].items():
        require(safe(rel) and sha(data[name]) == fm[rel]
                and members[name]['mode'] == mode(frozen['source_modes'][rel]), 'frozen source: ' + rel)
        expected_aliases.add(str(Path(inputs['source_export']) / rel))
        mapped(inputs['source_records'][rel])
        if rel in base and rel not in changed:
            b = data[name]
            require(len(b) == base[rel]['bytes']
                    and hashlib.sha1(b'blob ' + str(len(b)).encode() + b'\0' + b).hexdigest() == base[rel]['git_blob'],
                    'committed clean reference Git blob: ' + rel)
    for rel in changed:
        b = mapped(inputs['production_beforeimages'][rel])
        require(len(b) == base[rel]['bytes'] and sha(b) == base[rel]['sha256']
                and hashlib.sha1(b'blob ' + str(len(b)).encode() + b'\0' + b).hexdigest() == base[rel]['git_blob'],
                'production clean before-image: ' + rel)
    projected = jrec(inputs['native_source_projection'])
    require(projected['files'] == fm and projected['file_modes'] == frozen['source_modes']
            and jrec(projected['host_manifest']) == frozen, 'native files/schema projection')

    # Prior authored overlays and comment-only current bindings remain distinct.
    original_seal = jrec(inputs['fix_seal'])
    for record in records({k: original_seal[k] for k in (
            'owned_files', 'production_beforeimages', 'scaffold_dependencies', 'unchanged_dependency_files')}):
        mapped(record)
    comment = jrec(inputs['comment_revision'])
    old = jrec(inputs['prior_source_freeze'])
    require(old['base_commit'] == frozen['base_commit'], 'historical comment source base')
    require(set(comment['changes']) == {'tests/native_ram_port.c', 'tests/native_ram_port.h'},
            'comment history scope')
    require({rel for rel in fm if fm[rel] != old['source'][rel]} == set(comment['changes']),
            'historical source inventory delta')
    for rel, row in comment['changes'].items():
        require(row['original_sha256'] == old['source'][rel] and row['current_sha256'] == fm[rel],
                'comment before/after pins')

    def lane(spec, native=False):
        report, execution = jrec(spec['report']), jrec(spec['execution'])
        require(execution['state'] == 'completed' and execution['driver_rc'] == 0
                and len(execution['durable_copies']) == spec['pairs'], 'actual lane outcome/count')
        for pair in execution['durable_copies']:
            a, b = pair['original'], pair['durable']
            require(mapped(a) == mapped(b) and mode(a['mode']) == mode(b['mode']), 'durable pair bytes/mode')
        for record in records(report):
            mapped(record)
        for key in ('compiler', 'python'):
            tool = report[key]
            require(sha(data[aliases[tool['path']]]) == tool['sha256'], 'executed tool pin')
        require(report['passed'] is True and report['source_files_before'] == report['source_files_after'] == 225,
                'stable actual source/PASS')
        require((report['base_commit'], report['base_tree']) == (inputs['base_commit'], inputs['base_tree']),
                'actual report base')
        sf = jrec(report['source_manifest'])
        require(sf.get('source', sf.get('files')) == fm, 'actual source map')
        require(execution['argv'][0] == report['python']['path'] and execution['argv'][1] == '-B',
                'actual Python child boundary')
        require(execution['emulator'] == execution['physical'] == 'NOT_RUN', 'target operation scope')
        require(execution['HUNK_execution' if native else 'native'] == 'NOT_RUN', 'native operation scope')
        return report, execution

    host, he = lane(inputs['host'])
    native, ne = lane(inputs['native'], True)
    fingerprint = sha(json.dumps(fm, sort_keys=True, separators=(',', ':')).encode())
    require(host['export_tree_sha256'] == native['source_fingerprint'] == fingerprint, 'source fingerprint')
    require(host['modules'] == [g['module'] for g in inputs['host_groups']]
            and [g['module'] for g in host['groups']] == host['modules']
            and all(g['passed'] and g['failures'] == g['errors'] == 0 for g in host['groups']), 'four actual groups')
    require(len(host['commands']) == 8 and all(c['product_returncode'] == 0 for c in host['commands'])
            and len(host['products']) == 4 and not host['generated_inputs'], 'host eight RC0/four products')
    require(len(host['canonical_dependencies']) == 83 and len(host['external_sdk_dependencies']) == 105,
            'actual host closure')
    require(len(native['commands']) == 43 and all(c['returncode'] == 0 for c in native['commands'])
            and native['native_NOT_RUN'] is True and set(native['targets']) == {inputs['target']},
            'native 43 RC0/one compile-only target')
    target = native['targets'][inputs['target']]
    require(len(target['dependencies']) == 78 and len(target['external_sdk_dependencies']) == 32,
            'native compiled closure')
    protected = {x['path'] for x in jrec(inputs['protected_snapshot'])['preserved_files']}
    require(len(protected) == 16, 'protected16 snapshot metadata')
    compiled = set(host['canonical_dependencies']) | set(target['dependencies'])
    require(not compiled & protected and manifest['protected_compiled_intersection'] == [],
            'protected paths entered actual compiled source')
    require(manifest['clean_reference_paths'] == sorted(set(fm) - compiled)
            and manifest['protected_clean_reference_paths'] == sorted(protected & (set(fm) - compiled)),
            'clean reference/compiled distinction')
    require(manifest['compiled_sources'] == {
        'host': sorted(host['canonical_dependencies']), 'native': sorted(target['dependencies'])},
        'compiled source inventory')
    for deps in (host['canonical_dependencies'], target['dependencies']):
        require(all(fm[rel] == digest for rel, digest in deps.items()), 'compiled source bytes')

    def literal_sources(rel, constant):
        tree = ast.parse(data[manifest['source_members'][rel]])
        values = [ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == constant for t in node.targets)]
        require(len(values) == 1 and isinstance(values[0], list), 'literal declaration: ' + rel)
        return values[0]

    def dependency_map(output, directory):
        # Read the actual Makefile text, but use archived lexical/resolved bindings.
        # No resolve(), SDK traversal or compiler query occurs in this reader.
        text = mapped(output).decode().replace('\\\n', ' ')
        require(':' in text, 'full-M output lacks target separator')
        result = {'canonical': {}, 'external_sdk': {}}
        tokens = shlex.split(text.split(':', 1)[1])
        require(tokens, 'empty full-M dependencies')
        for token in tokens:
            spelling = posixpath.normpath(token if token.startswith('/') else directory + '/' + token)
            binding = inputs['dependency_spellings'][spelling]
            kind, key = binding['kind'], binding['key']
            require(kind in result, 'generated or unknown dependency')
            digest = fm[key] if kind == 'canonical' else sha(data[aliases[key]])
            require(digest == binding['sha256'], 'saved dependency spelling binding')
            result[kind][key] = digest
        return result

    full_m = 0
    reconstructed_host = {'canonical': {}, 'external_sdk': {}}
    for index, group in enumerate(inputs['host_groups']):
        compile_cmd, run = host['commands'][2*index:2*index+2]
        units = literal_sources(group['recipe'], group['constant'])
        require(units == group['units'] and len(units) == group['full_M'], 'per-group declaration/order')
        require(compile_cmd['group'] == run['group'] == group['module'], 'per-group command binding')
        av = compile_cmd['executed_argv']
        require(av[:-2] == [host['compiler']['path'], *group['flags'], *units] and av[-2] == '-o'
                and run['executed_argv'] == [av[-1]], 'compile/runtime argv edge')
        queries = compile_cmd['dependency_commands']
        require(len(queries) == len(units) and not run['dependency_commands'], 'per-group full-M count')
        actual_deps = {'canonical': {}, 'external_sdk': {}}
        for q, unit in zip(queries, units):
            require(q['unit'] == unit and q['returncode'] == 0
                    and q['argv'] == [host['compiler']['path'], *group['flags'], '-M', unit], 'host full-M flags/order')
            found = dependency_map(q['output'], q['directory'])
            for kind in actual_deps:
                actual_deps[kind].update(found[kind])
                reconstructed_host[kind].update(found[kind])
        require(actual_deps['canonical'] == compile_cmd['dependencies']['canonical']
                and actual_deps['external_sdk'] == compile_cmd['dependencies']['external_sdk']
                and not compile_cmd['dependencies']['generated'], 'actual group full-M closure')
        full_m += len(queries)
        product = host['products'][index]
        require(compile_cmd['produced_binary'] == run['input_binary'] == product, 'binary provenance edge')
        b = mapped(dict(product, path=product['archive_path']))
        require(b == data[aliases[product['original_path']]], 'ended executable alias')
        expected_aliases.add(product['original_path'])
        require(b'___assert_rtn' in b and b'___asan' in b and b'___ubsan' in b, 'host asserts/sanitizer symbols')
        lines = mapped(run['output_log']).splitlines()
        require(lines == [x.encode() for x in group['markers']], 'exact per-group runtime markers')
    require(full_m == 44 and sum(len(g['markers']) for g in inputs['host_groups']) == 7, '44 full-M/seven markers')
    require(reconstructed_host['canonical'] == host['canonical_dependencies']
            and reconstructed_host['external_sdk'] == host['external_sdk_dependencies'], 'host full-M union')

    ordered = literal_sources('tests/test_readers_activation.py', 'ACTIVATION_SOURCES')
    require(len(ordered) == 32 and target['source_inputs'] == ordered, 'native unchanged 32-TU fixture')
    flags = inputs['native_flags']
    require(native['flags'] == target['flags'] == native['recipe_contract']['flags'] == flags, 'native original flags')
    queries = [c for c in native['commands'] if '-M' in c['argv']]
    require(len(queries) == 32 and all(c['argv'] == [native['compiler']['path'], *flags, '-M', unit]
                                     for c, unit in zip(queries, ordered)), 'native full-M order/flags')
    reconstructed_native = {'canonical': {}, 'external_sdk': {}}
    for q in queries:
        found = dependency_map(q['log'], q['directory'])
        for kind in reconstructed_native:
            reconstructed_native[kind].update(found[kind])
    require(reconstructed_native['canonical'] == target['dependencies']
            and reconstructed_native['external_sdk'] == target['external_sdk_dependencies'], 'native full-M union')
    build_argv = [native['compiler']['path'], *flags, *ordered, '-o', target['binary']['path']]
    links = jrec(native['compile_commands'])['commands']
    require(len(links) == 1 and links[0]['argv'] == build_argv and links[0]['ordered_translation_units'] == ordered
            and sum(c['argv'] == build_argv for c in native['commands']) == 1, 'native link edge')
    binary, cpp = mapped(target['binary']), mapped(target['preprocessed_fixture'])
    require(len(binary) == 246096 and sha(binary) == inputs['native_binary_sha256']
            and struct.unpack('>I', binary[:4])[0] == 0x3f3, 'current HUNK identity')
    require('__assert_func' in target['last_assert_macro']
            and len(re.findall(rb'\b__assert_func\s*\(', cpp)) == 249
            and len(re.findall(rb'^#define assert\([^\n]*', cpp, re.M)) == 1
            and len(re.findall(rb'void __assert_func\s*\(', cpp)) == 1
            and len(re.findall(rb'__assert_func\s*\(\s*"[^"]+"\s*,', re.sub(rb'^#.*\n', b'', cpp, flags=re.M))) == 247,
            'enabled CPP assertions (compiled, not executed)')
    require(cpp.count(inputs['footer'].encode()) == binary.count(inputs['footer'].encode()) == 1,
            'full CPP/HUNK footer')

    hp, np = jrec(host['pins']), jrec(native['pins'])
    require(host['compiler'] == hp['host']['compiler'] and host['python'] == hp['host']['python']
            and native['compiler'] == np['native']['compiler'] and native['python'] == np['host']['python'], 'tool pin contracts')
    require(host['sanitizer_runtime_inputs'] == hp['host']['runtime_inputs']
            and host['pinned_sdk_inputs'] == hp['host']['sdk_inputs']
            and native['runtime_inputs'] == np['native']['runtime_inputs'], 'runtime contracts')
    for depmap in (host['external_sdk_dependencies'], host['sanitizer_runtime_inputs'],
                   host['pinned_sdk_inputs'], target['external_sdk_dependencies']):
        for path, digest in depmap.items():
            require(sha(data[aliases[path]]) == digest, 'archived SDK/runtime pin: ' + path)
    require(len(native['runtime_inputs']) == len(native['runtime_paths']) == 7
            and set(native['runtime_inputs']) == set(native['runtime_paths']), 'runtime name/path schema')
    for name, digest in native['runtime_inputs'].items():
        require(sha(data[aliases[native['runtime_paths'][name]]]) == digest, 'native runtime ' + name)
        query = [c for c in native['commands'] if c['argv'][-1] == '-print-file-name=' + name]
        require(len(query) == 1, 'unique actual runtime query')
        raw_query = mapped(query[0]['log']).decode().strip()
        require(raw_query.startswith('/') and posixpath.normpath(raw_query) == native['runtime_paths'][name],
                'actual normalized runtime query output')

    refusal = jrec(inputs['schema_failure']['execution'])
    stderr = mapped(inputs['schema_failure']['stderr'])
    require(refusal['state'] == 'completed' and refusal['driver_rc'] == 1 and 'durable_copies' not in refusal
            and b"KeyError: 'files'" in stderr and mapped(inputs['schema_failure']['stdout']) == b'',
            'preserved builder-Python schema failure')
    require(inputs['schema_failure']['classification'] == {
        'builder_Python_invocations': 1, 'compiler_or_tool_subprocesses': 0,
        'output_directory_created': False, 'product_execution': 'NOT_RUN'}, 'schema history classification')
    require(jrec(inputs['schema_failure']['schema_review'])['old_builder_failure']
            == 'KeyError files before builder compiler/subprocess/outmkdir', 'root schema classification')
    require(refusal['argv'][2] == native['builder']['path']
            and refusal['argv'][refusal['argv'].index('--source-manifest')+1] == inputs['frozen_source']['path']
            and ne['argv'][ne['argv'].index('--source-manifest')+1] == inputs['native_source_projection']['path'],
            'distinct schema repair before native-v2')

    require(set(manifest['post_build_overlays']) == {'docs/READERS_ACTIVATION.md', 'docs/SAMPLE_MEMORY.md'},
            'two separate post-build documentation overlays')
    for rel, record in inputs['post_build_overlays'].items():
        require(mapped(record) == data[manifest['post_build_overlays'][rel]], 'post-build overlay bytes')
    for control in inputs['preparation_controls'].values():
        mapped(control)
        expected_aliases.add(control['path'])
    expected_aliases.add(inputs['final_input_path'])
    require(set(aliases) == expected_aliases, 'unexpected alias or unbound artifact')
    return {'status': 'CLEAR_SAVED_BYTES_ONLY', 'source_files': 225, 'host_groups': 4,
            'host_RC0': 8, 'host_full_M': 44, 'native_RC0': 43, 'native_full_M': 32,
            'HUNK_execution': 'NEVER_EXECUTED', 'new_fixtures_native': 'NOT_BUILT',
            'total_stack': 'UNKNOWN;65536_NOT_CLEARED'}


def main():
    root = Path(__file__).resolve().parent
    expected = {'README.md', 'manifest.json', 'payload.tar.gz', 'verify_payload.py', 'SHA256SUMS'}
    require({p.name for p in root.iterdir()} == expected, 'five control files only')
    sums = (root / 'SHA256SUMS').read_text().splitlines()
    require(len(sums) == 4 and len(set(sums)) == 4, 'four checksum rows')
    for row in sums:
        digest, name = row.split('  ')
        require(name in expected - {'SHA256SUMS'} and sha((root/name).read_bytes()) == digest, 'control checksum')
    manifest = json.loads((root/'manifest.json').read_bytes())
    payload = (root/'payload.tar.gz').read_bytes()
    require(len(payload) == manifest['payload']['bytes'] and sha(payload) == manifest['payload']['sha256'], 'payload pin')
    data = {}
    with tarfile.open(fileobj=io.BytesIO(payload), mode='r:gz') as tar:
        for entry in tar.getmembers():
            require(entry.isfile() and safe(entry.name) and entry.name not in data
                    and entry.name in manifest['members'], 'safe unique regular members')
            require(entry.uid == entry.gid == entry.mtime == 0 and not entry.uname and not entry.gname
                    and not entry.pax_headers and not entry.linkname, 'USTAR ordinary metadata')
            value = tar.extractfile(entry).read()
            pin = manifest['members'][entry.name]
            require(len(value) == pin['bytes'] and sha(value) == pin['sha256'] and entry.mode == pin['mode'], 'member pin')
            require(pin['origins'] and pin['roles'], 'member origin/role attribution')
            data[entry.name] = value
    require(set(data) == set(manifest['members']) and all(n in data for n in manifest['path_members'].values()), 'membership/aliases')
    require(canonical_payload(data, manifest['members']) == payload, 'deterministic USTAR/gzip bytes')
    result = check_evidence(manifest, data)
    inputs = json.loads(data[manifest['inputs']])
    require((root/'README.md').read_bytes() == inputs['readme'].encode(), 'README not exact root input prose')
    control = inputs['preparation_controls']['verify_payload.py']
    require((root/'verify_payload.py').read_bytes() == data[manifest['path_members'][control['path']]],
            'outer verifier not exact archived preparation source')
    result.update({'members': len(data), 'payload_sha256': sha(payload), 'reader_scope': 'ARCHIVED_BYTES_NO_LIVE_SDK_RESOLUTION'})
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
