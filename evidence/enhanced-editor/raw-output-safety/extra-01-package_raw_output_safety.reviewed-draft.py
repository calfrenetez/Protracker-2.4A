#!/usr/bin/env python3
"""Private lossless RAW evidence handoff; no tests/builds/Git/UI/targets invoked."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import stat
import sys
import tempfile
import types

INPUT = Path('/private/tmp/protracker-raw-output-safety-769na_5f')
ROOT = Path('/private/tmp/protracker-raw-output-root-q6vxdl_5')
SUPPORT = Path('/private/tmp/package_project_output_safety_v2.py')
SUPPORT_SHA = 'a9f0e287c8b69b315dce2e097d2548250597c54aebe4743c7bcd0c2891c8f6d8'
PINS = {
    'source-manifest.json': 'b958e2910b011c9329d4786e305edf90a9e32e95f76bf64285ab0c97a800e616',
    'baseline-manifest.json': 'f41e64bed7718992024c5ccb64a7b9343a7233d6ae9c09cf163d3d80fa0367d4',
    'host-final/host-checks.json': '875b762b6816f3e28c624d626f23420f0aa244414b26a1ede457daba316eb3f0',
    'native-final/manifest.json': '93415b404c555f40e838d3d9736fa07b0879850578bfd8eef5b6c561027c6bcb',
    'baseline-probe/result.json': 'dc8bce8915c9d0abe29a32597249f5fee66dcfb78d1ea5fb6402e18d38656bfa',
    'candidate-probe/result.json': 'a196bf7963bb1a4e3eceb067d8242ec29d7d3359f69c385919c2383c014180bd',
    'final.patch': '853de49effe0fc134feeab7245056bf2437bee7c5ca5ff18c8d20425302a8c6f',
}
QUALIFICATION_SHA = 'f0b9302c1f159858c5be91e88d585beaf380a795c40e2e2352c30bb775ce94a5'
QUALIFIED_TREE = '37a852bea9854348c68614989f1d35ec65bbe0fe'
TARGETS = {'raw-core': 'PTRawTest', 'sample-raw': 'PTExecSampleRawFileTest'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=INPUT)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--support-helper', type=Path, default=SUPPORT)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--qualification-helper', type=Path, required=True)
    parser.add_argument('--coordination-record', type=Path, required=True)
    parser.add_argument('--extra-record', type=Path, action='append', default=[])
    parser.add_argument('--qa-out', type=Path)
    args = parser.parse_args()
    support = args.support_helper.resolve()
    support_bytes = support.read_bytes()
    if hashlib.sha256(support_bytes).hexdigest() != SUPPORT_SHA:
        raise ValueError('Reviewed pure archive/identity utility source changed')
    # Execute trusted metadata utility definitions under a non-main name, never its project packaging main.
    # Using exec rather than importlib avoids generating a bytecode file beside the immutable helper.
    module = types.ModuleType('_reviewed_exact_evidence_utilities')
    module.__file__ = str(support)
    exec(compile(support_bytes, str(support), 'exec'), module.__dict__)
    require, sha, fingerprint = module.require, module.sha, module.fingerprint
    safe, files, source_check = module.safe, module.files, module.source_check
    encoded = module.encoded
    verify_recorded_file = module.verify_recorded_file
    write_payload, verify_payload = module.write_payload, module.verify_payload
    input_root, root, out, qualification, qualifier, coordination = [p.resolve() for p in
        (args.input, args.root, args.out, args.qualification, args.qualification_helper, args.coordination_record)]
    extras = [p.resolve() for p in args.extra_record]
    qa_out = (args.qa_out or Path(str(out) + '-qa.json')).resolve()
    origins = [input_root, root, support, qualification.parent, qualifier, coordination, Path(__file__).resolve(), *extras]
    for origin in origins:
        require(out != origin and out not in origin.parents and origin not in out.parents,
                'Output overlaps original input: ' + str(origin))
        require(qa_out != origin and origin not in qa_out.parents,
                'External QA output overlaps original: ' + str(origin))
    require(out != qa_out and out not in qa_out.parents and qa_out not in out.parents,
            'Package and external QA overlap')
    require(not out.exists() and not qa_out.exists(), 'Fresh private package and QA destinations required')
    for relative, expected in PINS.items():
        verify_recorded_file(input_root / relative, expected)
    verify_recorded_file(qualification, QUALIFICATION_SHA)
    source_scopes = [(input_root / 'baseline', input_root / 'baseline-manifest.json'),
                     (input_root / 'source-final', input_root / 'source-manifest.json')]
    source_before = [source_check(*pair) for pair in source_scopes]
    final = json.loads((input_root / 'source-manifest.json').read_bytes())
    baseline_manifest = json.loads((input_root / 'baseline-manifest.json').read_bytes())
    host = json.loads((input_root / 'host-final/host-checks.json').read_bytes())
    native = json.loads((input_root / 'native-final/manifest.json').read_bytes())
    before = json.loads((input_root / 'baseline-probe/result.json').read_bytes())
    after = json.loads((input_root / 'candidate-probe/result.json').read_bytes())
    root_inputs = json.loads((root / 'root-inputs.json').read_bytes())
    top = json.loads(qualification.read_bytes())
    coordination_object = json.loads(coordination.read_bytes())
    native_review = json.loads((root / 'native-root-review.json').read_bytes())
    committed_source = json.loads((root / 'committed-source.json').read_bytes())
    require(committed_source['source_commit'] == '0219362725960d2c5c76b53a0ae4c2bd10289ac7' and
            committed_source['source_tree'] == QUALIFIED_TREE, 'Separate committed source attribution changed')
    document = committed_source['documentation']
    require(document['path'] == str(root / 'committed-source-sample-memory.md') and document['bytes'] == 302604 and
            document['sha256'] == '099e2b0e880de2e3972d06d7d9a5513d388824e211198e8ad186050bb2623ea9',
            'Separate documentation snapshot attribution changed')
    verify_recorded_file(root / 'committed-source-sample-memory.md', document['sha256'], document['bytes'])
    require(host['passed'] is True and len(host['groups']) == 2 and
            host['modules'] == ['test_raw', 'test_sample_raw_file'], 'Final host scope changed')
    require(native['passed'] is True and set(native['targets']) == set(TARGETS.values()), 'Native target scope changed')
    require(len(final['raw_overlays']) == 3 and len(final['prior_overlays']) == 10,
            'Three RAW overlays/ten inherited project-SVX overlays changed')
    require(top['passed'] is True and len(top['cases']) == 2 and top['source_tree'] == QUALIFIED_TREE,
            'Exact supplied two-case native result changed')
    require(top['verified_manifest'] == native, 'Top qualification embeds a different native build')
    require(native_review['qualification_sha256'] == QUALIFICATION_SHA and native_review['source_tree'] == QUALIFIED_TREE,
            'Copied root native review references another qualification')
    for record in (host, native, after):
        require(record['source_manifest']['sha256'] == PINS['source-manifest.json'], 'Final proof references another source manifest')
    require(before['source_manifest']['sha256'] == PINS['baseline-manifest.json'], 'Baseline proof references another manifest')
    require(before['passed'] is True and before['commands'][-1]['returncode'] == 20 and
            after['passed'] is True and after['commands'][-1]['returncode'] == 0,
            'Original expected before/after probe statuses changed')
    require((input_root / 'baseline-probe/probe.c').read_bytes() == (input_root / 'candidate-probe/probe.c').read_bytes(),
            'Independent before/after probe source differs')
    require(host['generated_inputs'] == [], 'Do not invent a generated fixture absent from the original host report')
    for item in host['products']:
        verify_recorded_file(Path(item['archive_path']), item['sha256'], item['bytes'])
    for record in (host, native):
        for helper in record['helper_inputs']:
            verify_recorded_file(Path(helper['path']), helper['sha256'], helper['bytes'])
    verify_recorded_file(Path(native['compile_commands']), native['compile_commands_sha256'], native['compile_commands_bytes'])
    verify_recorded_file(Path(native['canonical_recipe']['path']), native['canonical_recipe']['sha256'], native['canonical_recipe']['bytes'])
    for command in native['commands']:
        log = command['log']
        verify_recorded_file(Path(log['path']), log['sha256'], log['bytes'])
    for target_name, target in native['targets'].items():
        verify_recorded_file(input_root / 'native-final' / target_name, target['binary_sha256'], target['binary_bytes'])
    verify_recorded_file(root / 'shared_infra_render_files.py', root_inputs['runner_after_sha256'])
    verify_recorded_file(qualifier, root_inputs['qualifier_sha256'])
    for path, digest in final['prior_overlays'].items():
        require(baseline_manifest['source'][path] == digest, 'Inherited project/SVX fix differs from baseline inventory')
    # Require an actual captured release response, not just a dispatch/prelaunch coordination record.
    require('release_response' in coordination_object,
            'Actual original release acknowledgment is required before final packaging')
    acknowledgment = coordination_object['release_response']
    require(acknowledgment.get('isError') is False and acknowledgment.get('content'),
            'Original release acknowledgment is empty or failed')
    acknowledgment_text = next((item['text'] for item in acknowledgment['content']
                                if item.get('type') == 'text'), None)
    require(acknowledgment_text is not None, 'Original release acknowledgment lacks its captured response')
    acknowledgment_record = json.loads(acknowledgment_text)
    require(any(item.get('revision') == 55 and item.get('latestTurn', {}).get('status') == 'completed'
                for item in acknowledgment_record.get('polls', [])),
            'Exact completed original release acknowledgment revision55 is required')
    members = {}
    def add(name, origin, scope, expected=None):
        safe(name)
        require(name not in members, 'Duplicate payload member destination: ' + name)
        require(not origin.is_symlink() and stat.S_ISREG(origin.stat().st_mode), 'Nonregular original refused')
        identity = fingerprint(origin)
        if expected is not None:
            require(identity['sha256'] == expected, 'Scoped original hash mismatch: ' + str(origin))
        members[name] = {'path': name, 'bytes': identity['bytes'], 'sha256': identity['sha256'],
                         'mode': stat.S_IMODE(origin.stat().st_mode), 'kind': 'exact_original_archive_member',
                         'origin': identity, 'scope': scope}
    def tree(prefix, directory, scope, skip=()):
        skips = {p.resolve() for p in skip}
        for path in files(directory):
            if path.resolve() not in skips:
                add(prefix + '/' + path.relative_to(directory).as_posix(), path, scope)
    for prefix, sub, scope in [
        ('host-final', 'host-final', 'Exact two-group ASan/UBSan report/commands/logs/products; no generated fixture exists'),
        ('native-final', 'native-final', 'Exact two cross-build recipes/logs/binaries/SDK/compiler/runtime identities; portable versus Exec scopes retained'),
        ('regression/before', 'baseline-probe', 'Original expected RC20 preservation failure before three RAW fixes'),
        ('regression/after', 'candidate-probe', 'Identical probe source after three fixes; original RC0/master preservation'),
        ('helpers/host', 'tools', 'Exact host provenance/probe helpers/toolchain pins'),
        ('helpers/native', 'tools-native', 'Exact native build/provenance helpers/toolchain pins/declared preparation inputs'),
    ]:
        tree(prefix, input_root / sub, scope)
    for name, destination, scope in [
        ('baseline-manifest.json', 'source/baseline-manifest.json', 'Original full7531 baseline inventory with ten inherited project/SVX overlays'),
        ('source-manifest.json', 'source/source-manifest.json', 'Original full7531 final inventory/three RAW overlays plus ten inherited project/SVX overlays'),
        ('final.patch', 'source/final-three-file.patch', 'Exact reviewed three-file RAW product/test patch'),
        ('baseline-probe.c', 'regression/original-probe.c', 'Original independent probe source retained alongside identical before/after copies'),
    ]:
        add(destination, input_root / name, scope)
    inherited_manifest = baseline_manifest['input_source_manifest']
    verify_recorded_file(Path(inherited_manifest['path']), inherited_manifest['sha256'], inherited_manifest['bytes'])
    add('source/inherited-svx-project-source-manifest.json', Path(inherited_manifest['path']),
        'Exact inherited project/SVX source manifest with ten overlays; no completed evidence duplication or new acceptance')
    dependencies = {}
    for item in [host['canonical_dependencies'], after['canonical_dependencies'], final['overlays'], native['python_declaration_inputs'],
                 *[target['dependencies'] for target in native['targets'].values()]]:
        for path, digest in item.items():
            require(path not in dependencies or dependencies[path] == digest, 'Conflicting execution closure hash')
            dependencies[path] = digest
    recipe = Path(native['canonical_recipe']['path']).relative_to(input_root / 'source-final').as_posix()
    dependencies[recipe] = final['source'][recipe]
    for module_name in host['modules']:
        path = 'tests/' + module_name + '.py'
        dependencies[path] = final['source'][path]
    if 'tests/__init__.py' in final['source']:
        dependencies['tests/__init__.py'] = final['source']['tests/__init__.py']
    for path, digest in sorted(dependencies.items()):
        add('source/final-scoped/' + path, input_root / 'source-final' / path,
            'Exact final canonical execution/module/recipe closure and product overlay bytes', digest)
    baseline_dependencies = dict(before['canonical_dependencies'])
    baseline_dependencies.update({path: baseline_manifest['source'][path] for path in final['raw_overlays']})
    baseline_dependencies.update(final['prior_overlays'])
    for path, digest in sorted(baseline_dependencies.items()):
        add('source/baseline-scoped/' + path, input_root / 'baseline' / path,
            'Exact baseline probe closure and before-fix source overlays, inherited project/SVX guards retained', digest)
    for path in sorted(final['raw_overlays']):
        add('history/working-candidate-scoped/' + path, input_root / 'candidate' / path,
            'Working private candidate bytes observed at packaging time, may equal final; not an immutable initial draft')
    tree('root', root, 'Original root RAW runner/qualifier/reviews/committed documentation/coordination; original scopes and pending wording remain unmodified')
    add('qualification/top/qualification-result.json', qualification,
        'Exact actual top result original status/type/fields; separate from nested originals')
    tree('qualification/records', qualification.parent,
         'Exact nested runners/results/logs/identities/independent-cleanup records, retained separately from top', skip=(qualification,))
    add('qualification/historical-orchestration.py', qualifier,
        'Exact supplied historical qualifier bytes; execution attribution only from original/root reports')
    add('qualification/root-reported-coordination.json', coordination,
        'Exact captured root-reported request/take/release acknowledgment records; no live packaging verification')
    for n, path in enumerate(extras, 1):
        add('qualification/extra/%02d-%s' % (n, path.name), path, 'Additional exact root-reported control/review')
    add('helpers/inherited-svx-packager.py', Path('/private/tmp/package_svx_output_safety.py'),
        'Exact previously reviewed SVX helper used as preparation reference only; not executed, no completed evidence duplication',
        '5c02b2939a9235cfca720b69bd388eaafbff2d542bd4c787c2db26b53a7cf971')
    add('helpers/packager.py', Path(__file__).resolve(), 'Exact private RAW packaging helper; no tests/builds/Git/UI/target calls')
    add('helpers/packager-support-project-v2.py', support,
        'Reviewed pure identity/archive utility definitions only; its project main is never invoked by this RAW package')
    case_consistency = []
    seen_labels = set()
    for case in top['cases']:
        label, target_name = case['candidate']['label'], case['candidate']['binary']
        require(label not in seen_labels and TARGETS.get(label) == target_name, 'Unexpected/duplicate native case')
        seen_labels.add(label)
        target = native['targets'][target_name]
        require(case['candidate']['sha256'] == target['binary_sha256'] and case['candidate']['bytes'] == target['binary_bytes'],
                'Native executed candidate does not match original cross-build')
        run = Path(case['run']).resolve()
        require(qualification.parent in run.parents, 'Original nested run outside qualification tree')
        require(json.loads((run / 'result.json').read_bytes()) == case['result'], 'Top/nested original result mismatch')
        require(json.loads((run / 'independent-cleanup.json').read_bytes()) == case['independent_cleanup'], 'Independent original cleanup mismatch')
        result = case['result']
        require(result[label + '_returncode'] == '0' and result['passed'] is True, 'Recorded exact native case status changed')
        require(len(case['independent_cleanup']['observations']) == 11, 'Original independent observation count changed')
        require(result.get('run_files_cleaned') is True and
                all('ch%d_dma=0' % i in result.get('cleanup_audio', '').split('\t') for i in range(4)),
                'Original exact cleanup/fresh DMA-off evidence changed')
        independent = case['independent_cleanup']
        require(independent.get('passed') is True and
                all(all(value is False for value in row.values()) for row in independent['observations']) and
                len(independent['processes']) == 1 and independent['processes'][0][0] == 19081 and
                'Paused=false' in independent['status'].split('\t') and
                all('ch%d_dma=0' % i in independent['audio'].split('\t') for i in range(4)) and
                'Paused=false' in independent['final_idle']['status'].split('\t') and
                all('ch%d_dma=0' % i in independent['final_idle']['audio'].split('\t') for i in range(4)),
                'Original independent absence/process/running/final-idle evidence changed')
        log = case['native_log']
        require((run / (label + '.log')).read_text() == log, 'Top/native original log mismatch')
        require(case['candidate']['marker'] in log, 'Original fixture marker missing')
        if label == 'sample-raw':
            require('35 Fast allocations, zero owned bytes' in log and 'fixed workspace=11240' in log and
                    result.get('sample_staging_clean') is True, 'Exact Exec file proof/cleanup changed')
            scope = 'Exec Fast allocation and streamed file workspace/staging fixture'
        else:
            require('EXEC MEMORY PASS:' not in log and 'no Exec Fast allocator' in result['scope'],
                    'Portable assertion fixture must not imply Exec allocation proof')
            scope = 'Portable RAW codec assertions, no Exec Fast/TypeOfMem acceptance'
        case_consistency.append({'label': label, 'binary': target_name, 'sha256': target['binary_sha256'],
                                 'scope': scope, 'original_top_nested_independent_equal': True,
                                 'independent_observations': 11})
    require(seen_labels == set(TARGETS), 'Native case coverage mismatch')
    out.mkdir()
    payload = out / 'payload.tar.gz'
    with payload.open('xb') as writer:
        write_payload(members, writer)
    archive_verification = verify_payload(payload, members)
    with tempfile.TemporaryFile() as second:
        write_payload(members, second)
        second.seek(0)
        with payload.open('rb') as first:
            while True:
                a, b = first.read(1024 * 1024), second.read(1024 * 1024)
                require(a == b, 'Repeated deterministic payload differs')
                if not a:
                    break
    archive_verification['second_archive_write_byte_identical'] = True
    controls = []
    def record(name, kind, scope, origin=None, derivation=None):
        row = fingerprint(out / name)
        row['path'] = name
        row.update(kind=kind, scope=scope)
        if origin is not None:
            row['origin'] = fingerprint(origin)
        if derivation is not None:
            row['derivation'] = derivation
        controls.append(row)
    def copy(name, origin, scope):
        require(not (out / name).exists(), 'Duplicate readable control')
        original = origin.read_bytes()
        (out / name).write_bytes(original)
        require((out / name).read_bytes() == original, 'Exact control byte copy failed')
        record(name, 'exact_byte_copy', scope, origin)
    def compressed(name, origin, scope):
        require(not (out / name).exists(), 'Duplicate readable gzip control')
        original = origin.read_bytes()
        with (out / name).open('xb') as writer:
            with gzip.GzipFile(fileobj=writer, filename='', mode='wb', mtime=0, compresslevel=9) as gz:
                gz.write(original)
        require(gzip.decompress((out / name).read_bytes()) == original, 'Exact control original gzip failed')
        record(name, 'gzip_exact_byte_derivation', scope, origin,
               {'method': 'gzip level9/mtime0/emptyfilename; no source JSON fields/order/whitespace altered'})
    record('payload.tar.gz', 'lossless_archive_derivation', 'Every selected raw original byte/source closure/helper/history',
           derivation=archive_verification)
    for name, origin, scope in [
        ('source-manifest.json.gz', input_root / 'source-manifest.json', 'Original complete7531 source manifest/three RAW plus ten inherited project/SVX overlays'),
        ('baseline-manifest.json.gz', input_root / 'baseline-manifest.json', 'Original complete7531 before-RAW manifest'),
        ('host-checks.json.gz', input_root / 'host-final/host-checks.json', 'Original two-group host sanitizer report/allcommands/products; generated_inputs[] retained'),
        ('native-build.json.gz', input_root / 'native-final/manifest.json', 'Original two-target cross-build report/orderedTUs/SDKclosures/pinpaths/scopes'),
        ('qualification-result.json.gz', qualification, 'Original actual top native report fields/status/scopes, distinct nested copies in payload'),
        ('committed-sample-memory.md.gz', root / 'committed-source-sample-memory.md', 'Exact committed documentation only, separate from immutable7531 compiled source/dependencies'),
    ]:
        compressed(name, origin, scope)
    for name, origin, scope in [
        ('baseline-before.json', input_root / 'baseline-probe/result.json', 'Original expected before-RAW RC20 preservation failure'),
        ('probe-after.json', input_root / 'candidate-probe/result.json', 'Original identical-source RC0 preservation probe after three fixes'),
        ('root-host-review.json', root / 'root-host-review.json', 'Original root host review including preserved pending wording/schema correction'),
        ('root-native-build-review.json', root / 'root-native-build-review.json', 'Original root cross-build review, unmodified portable/Exec scopes'),
        ('root-native-review.json', root / 'native-root-review.json', 'Original root actual native review: portable RAW codec separate from Exec file'),
        ('committed-source.json', root / 'committed-source.json', 'Original source commit/tree and separate documentation-only identity; outside compiled7531 snapshot'),
        ('root-inputs.json', root / 'root-inputs.json', 'Original root separate runner/qualifier fingerprints'),
        ('root-reported-coordination.json', coordination, 'Original root-captured release acknowledgment; packaging does not verify release live'),
    ]:
        copy(name, origin, scope)
    for n, path in enumerate(extras, 1):
        copy('extra-%02d-%s' % (n, path.name), path, 'Original supplied extra control/review, no changed field')
    source_after = [source_check(*pair) for pair in source_scopes]
    require(source_before == source_after, 'Immutable source inventories changed during packaging')
    for member in members.values():
        require(fingerprint(Path(member['origin']['path'])) == member['origin'], 'Original archived input changed during packaging')
    provenance = {
        'schema': 'protracker-raw-output-safety-member-provenance-v1',
        'scope': 'Private archive/control/source integrity only; no new product/build/test/target/cleanup/release/device/audio/physical/timing/listening acceptance',
        'source_before': source_before, 'source_after': source_after, 'source_inventories_before_after_identical': True,
        'raw_new_product_overlays': final['raw_overlays'], 'inherited_project_svx_overlays': final['prior_overlays'],
        'separate_runner_overlay': fingerprint(root / 'shared_infra_render_files.py'),
        'separate_committed_documentation': fingerprint(root / 'committed-source-sample-memory.md'),
        'committed_source_attribution': committed_source,
        'actual_qualified_tree': top['source_tree'], 'actual_native_cases': case_consistency,
        'generated_fixture_retention': 'Original host report generated_inputs is empty; streamed fixture was deleted by its test, no generated fixture invented/copied',
        'coordination_attribution': 'Exact copied original/root-reported records only, including actual release acknowledgment; no live verification by packaging',
        'archive': fingerprint(payload), 'archive_verification': archive_verification,
        'members': [members[name] for name in sorted(members)],
    }
    raw_provenance = encoded(provenance)
    with (out / 'member-provenance.json.gz').open('xb') as writer:
        with gzip.GzipFile(fileobj=writer, filename='', mode='wb', mtime=0, compresslevel=9) as gz:
            gz.write(raw_provenance)
    require(gzip.decompress((out / 'member-provenance.json.gz').read_bytes()) == raw_provenance, 'Member provenance gzip failed')
    record('member-provenance.json.gz', 'authored_member_provenance_gzip', 'All selected original member/origin/byte/hash/mode identities and source inventories',
           derivation={'uncompressed_bytes': len(raw_provenance), 'uncompressed_sha256': sha(raw_provenance),
                       'method': 'Authored full original-member index, gzip level9 mtime0 emptyfilename'})
    readme = '''# RAW output preservation evidence

`payload.tar.gz` preserves the exact selected raw host/cross-build/probe/qualification/root records, commands/products/toolchain/helper pin paths, canonical execution source closures, both executed host test modules/native build recipe and three RAW overlays. The complete 7,531-input baseline/final source manifests preserve all original fields, including ten inherited project/SVX source overlays. The root fourth runner overlay and fifth documentation snapshot remain separate from the compiled source snapshot. The documentation is exact committed source attribution only; it is not a compiled input. The inherited SVX helper/source manifest is retained for derivation and source history, without duplicating completed SVX evidence. Working candidate copies are labelled as observed at packaging time and may equal final source.

Two host ASan/UBSan groups and two pinned native builds preserve their original reports. The identical independent probe retains its expected baseline RC20 and final RC0, including both authoritative master and borrowed format settings. Portable `PTRawTest` native codec assertions are distinct from `PTExecSampleRawFileTest`, the streamed file/Exec Fast allocator fixture. Only the latter records 35 Fast allocations, zero owned bytes, fixed workspace 11240, 16 exact formats and transactional staging cleanup. Both original native cases retain separate 11-observation independent cleanup records. No host generated fixture is invented: the original report has `generated_inputs:[]` because its streamed file test deleted the temporary fixture.

`qualification-result.json.gz` is the unchanged actual top native report, also stored under `qualification/top/` in the archive. Separate original nested runner/result/log/identity/independent records remain under `qualification/records/`. Root host/build/native reviews keep their original scopes and pending wording. Request/take/release acknowledgments are copied original/root-reported coordination only; packaging does not operate or verify targets/live control/release.

`member-provenance.json.gz` lists every selected member/origin/size/hash/mode. Safe unique regular paths, fixed uid/gid/mtime0 and original modes are retained; every member was verified against original bytes through a tar reader and a second archive write was byte-identical. Complete immutable source inventories and original selected inputs remained unchanged. `compact-index.json` identifies each readable copy/derivation; `SHA256SUMS` covers every package file except itself. Gzip original controls preserve every byte without reformatting JSON. No actual device/card/MMIO, audio, physical hardware, timing or human listening acceptance is claimed. No original helper, product, repository, target or shared lock was changed by this private packager.
'''
    (out / 'README.md').write_text(readme)
    record('README.md', 'authored_scope_note', 'Precise portable/Exec/host/native/history/coordination tiers and selected-payload navigation')
    index = {'schema': 'protracker-raw-output-safety-compact-controls-v1',
             'scope': 'Copy/archive/derivation integrity only; no new execution or live coordination acceptance',
             'items': controls, 'control_exclusions': ['compact-index.json', 'SHA256SUMS'],
             'archive_verification': archive_verification, 'original_native_status': top['passed'],
             'source_inventories_before_after_identical': True}
    (out / 'compact-index.json').write_bytes(encoded(index))
    package_files = files(out)
    (out / 'SHA256SUMS').write_text(''.join(sha(p.read_bytes()) + '  ' + p.relative_to(out).as_posix() + '\n' for p in package_files))
    final_files = files(out)
    expected_paths = {row['path'] for row in controls} | {'compact-index.json', 'SHA256SUMS'}
    require({p.relative_to(out).as_posix() for p in final_files} == expected_paths, 'Unindexed package member')
    sums = {}
    for line in (out / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        safe(name)
        require(name not in sums, 'Duplicate checksum path')
        sums[name] = digest
    require(set(sums) == expected_paths - {'SHA256SUMS'}, 'Checksum coverage mismatch')
    for name, digest in sums.items():
        require(sha((out / name).read_bytes()) == digest, 'Checksum identity mismatch')
    for row in controls:
        data = (out / row['path']).read_bytes()
        require(len(data) == row['bytes'] and sha(data) == row['sha256'], 'Indexed control bytes changed')
        if 'origin' in row:
            original = Path(row['origin']['path']).read_bytes()
            require(len(original) == row['origin']['bytes'] and sha(original) == row['origin']['sha256'], 'Control origin changed')
            observed = gzip.decompress(data) if row['kind'] == 'gzip_exact_byte_derivation' else data
            require(observed == original, 'Exact original control bytes changed')
    require([source_check(*pair) for pair in source_scopes] == source_before, 'Source changed during final control verification')
    qa = {'scope': 'Private archive/control/source integrity only; no targets or new acceptance',
          'package': str(out), 'files': len(final_files), 'bytes': sum(p.stat().st_size for p in final_files),
          'archive_verification': archive_verification, 'payload': fingerprint(payload),
          'compact_index': fingerprint(out / 'compact-index.json'), 'sha256sums': fingerprint(out / 'SHA256SUMS'),
          'checksum_rows': len(sums), 'all_original_member_control_bytes_and_gzip_hashes_verified': True,
          'all_source_inventories_before_after_identical': True, 'native_cases': case_consistency,
          'portable_assertions_distinct_from_exec_allocation_proof': True,
          'no_missing_generated_fixture_invented': True, 'historical_coordination_copied_root_report_only': True}
    qa_out.write_bytes(encoded(qa))
    print(json.dumps({'package': str(out), 'qa': fingerprint(qa_out), 'files': qa['files'], 'bytes': qa['bytes'],
                      'payload': qa['payload'], 'archive_verification': archive_verification,
                      'index': qa['compact_index'], 'sha256sums': qa['sha256sums']}, indent=2))


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print('RAW PACKAGING FAILED; originals and any new output preserved: ' + repr(error), file=sys.stderr)
        raise
