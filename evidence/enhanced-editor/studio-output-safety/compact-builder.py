#!/usr/bin/env python3
"""Create a lossless private compact handoff; no target or repository operations."""
import argparse
import gzip
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import stat
import sys
import tarfile
import tempfile

FULL = Path('/private/tmp/protracker-studio-output-safety-evidence-final')
OUT = Path('/private/tmp/protracker-studio-output-safety-evidence-compact')
EXTERNAL_QA = Path('/private/tmp/protracker-studio-output-safety-evidence-final-qa.json')
PROVENANCE = 'provenance-1790992504159407000.json'
EXPECTED = {
    PROVENANCE: '742799510b670a0c6ef94c169bdb1a9d2c379d76f7ad543896833127aaad0c07',
    'SHA256SUMS': 'fd98cead32924f4f87943cdf0d21d9cccecb3de9ebfe9d057a02a4014142959c',
}
EXPECTED_QA = '8b0f36a531fa5e5b5e41834c4279d69e4746ad4fa794789327171d0be4e6bea4'


def require(condition, detail):
    if not condition:
        raise ValueError(detail)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def identity(path):
    data = path.read_bytes()
    return {'path': str(path), 'bytes': len(data), 'sha256': sha(data)}


def safe_name(name):
    parts = PurePosixPath(name).parts
    require(name and not name.startswith('/') and '\\' not in name,
            'Unsafe archive/member path: ' + repr(name))
    require(all(p not in ('', '.', '..') for p in parts),
            'Unsafe archive/member parts: ' + repr(name))
    require(PurePosixPath(name).as_posix() == name and '\n' not in name and '\r' not in name,
            'Noncanonical archive/member path: ' + repr(name))


def inventory(root):
    require(root.is_dir() and not root.is_symlink(), 'Input must be a real directory')
    rows = []
    for path in sorted(root.rglob('*')):
        require(not path.is_symlink(), 'Symlink refused: ' + str(path))
        st = path.stat()
        if stat.S_ISDIR(st.st_mode):
            continue
        require(stat.S_ISREG(st.st_mode), 'Nonregular input refused: ' + str(path))
        name = path.relative_to(root).as_posix()
        safe_name(name)
        data = path.read_bytes()
        rows.append({'path': name, 'bytes': len(data), 'sha256': sha(data),
                     'mode': stat.S_IMODE(st.st_mode)})
    require(len({r['path'] for r in rows}) == len(rows), 'Duplicate input member')
    return rows


def verify_full(root, rows):
    lookup = {r['path']: r for r in rows}
    require(len(rows) == 1883 and sum(r['bytes'] for r in rows) == 29547045,
            'Full package inventory differs from reviewed 1,883-file package')
    for path, digest in EXPECTED.items():
        require(lookup[path]['sha256'] == digest, 'Reviewed full control changed: ' + path)
    sums = {}
    for line in (root / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        safe_name(name)
        require(name not in sums, 'Duplicate full checksum path')
        sums[name] = digest
    require(set(sums) == set(lookup) - {'SHA256SUMS'}, 'Full checksum coverage mismatch')
    require(all(lookup[p]['sha256'] == s for p, s in sums.items()), 'Full checksum mismatch')
    provenance = json.loads((root / PROVENANCE).read_bytes())
    items = provenance['items']
    require(len(items) == 1881 and len({r['path'] for r in items}) == 1881,
            'Full provenance count or uniqueness mismatch')
    counts = {}
    for row in items:
        name = row['path']
        require(name in lookup, 'Full provenance member missing')
        require(row['bytes'] == lookup[name]['bytes'] and row['sha256'] == lookup[name]['sha256'],
                'Full provenance member identity mismatch: ' + name)
        kind = row['kind']
        counts[kind] = counts.get(kind, 0) + 1
        if kind in ('exact_byte_copy', 'gzip_exact_byte_derivation'):
            origin = row['origin']
            original = Path(origin['path']).read_bytes()
            require(len(original) == origin['bytes'] and sha(original) == origin['sha256'],
                    'Full provenance original identity changed: ' + origin['path'])
            packaged = (root / name).read_bytes()
            if kind == 'gzip_exact_byte_derivation':
                packaged = gzip.decompress(packaged)
            require(packaged == original, 'Full provenance original byte mismatch: ' + name)
    require(counts.get('exact_byte_copy') == 1871 and counts.get('gzip_exact_byte_derivation') == 7,
            'Full original copy/gzip counts mismatch')
    return {'checksum_rows': len(sums), 'provenance_items': len(items), 'kind_counts': counts,
            'originals_and_exact_gzip_bytes_verified': True}


def write_archive(root, rows, fileobj):
    with gzip.GzipFile(filename='', fileobj=fileobj, mode='wb', mtime=0, compresslevel=9) as gz:
        with tarfile.open(mode='w|', fileobj=gz, format=tarfile.PAX_FORMAT) as archive:
            for row in rows:
                data = (root / row['path']).read_bytes()
                require(len(data) == row['bytes'] and sha(data) == row['sha256'],
                        'Input changed during archive read: ' + row['path'])
                entry = tarfile.TarInfo(row['path'])
                entry.size = row['bytes']
                entry.mode = row['mode']
                entry.uid = entry.gid = 0
                entry.uname = entry.gname = ''
                entry.mtime = 0
                entry.type = tarfile.REGTYPE
                entry.pax_headers = {}
                archive.addfile(entry, io.BytesIO(data))


def verify_archive(path, rows):
    lookup = {r['path']: r for r in rows}
    seen = set()
    with tarfile.open(path, mode='r:gz') as archive:
        for entry in archive:
            safe_name(entry.name)
            require(entry.name not in seen and entry.name in lookup, 'Unexpected/duplicate member')
            seen.add(entry.name)
            row = lookup[entry.name]
            require(entry.isfile() and not entry.issym() and not entry.islnk(), 'Nonregular archive member')
            require(entry.uid == entry.gid == entry.mtime == 0 and entry.uname == entry.gname == '',
                    'Archive metadata is not fixed')
            require(entry.mode == row['mode'] and entry.size == row['bytes'], 'Archive metadata mismatch')
            require(set(entry.pax_headers).issubset({'path'}), 'Unexpected archive PAX metadata')
            handle = archive.extractfile(entry)
            require(handle is not None, 'Archive member could not be read')
            data = handle.read()
            require(len(data) == row['bytes'] and sha(data) == row['sha256'], 'Archive bytes mismatch')
    require(seen == set(lookup), 'Archive completeness mismatch')
    return {'regular_members': len(seen), 'member_bytes': sum(r['bytes'] for r in rows),
            'all_member_bytes_hashes_modes_verified': True, 'safe_unique_paths_only': True,
            'uid_gid_mtime_zero': True, 'no_symlinks_hardlinks_or_truncated_members': True}


def json_bytes(value):
    return (json.dumps(value, indent=2, ensure_ascii=False) + '\n').encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--full', type=Path, default=FULL)
    parser.add_argument('--out', type=Path, default=OUT)
    parser.add_argument('--external-qa', type=Path, default=EXTERNAL_QA)
    parser.add_argument('--qa-out', type=Path, default=Path(str(OUT) + '-qa.json'))
    args = parser.parse_args()
    root, out, qa_origin, qa_out = [p.resolve() for p in
                                  (args.full, args.out, args.external_qa, args.qa_out)]
    for origin in (root, qa_origin, Path(__file__).resolve()):
        require(out != origin and out not in origin.parents and origin not in out.parents,
                'Output overlaps an original input')
    require(qa_out != out and out not in qa_out.parents and qa_out not in out.parents,
            'External QA output overlaps package')
    require(root not in qa_out.parents and qa_out != root, 'QA output overlaps full package')
    require(not out.exists() and not qa_out.exists(), 'New compact output or QA already exists')
    rows = inventory(root)
    full_verification = verify_full(root, rows)
    require(identity(qa_origin)['sha256'] == EXPECTED_QA, 'External original QA changed')
    summary = json.loads((root / 'package-summary.json').read_bytes())
    require(summary['source_files'] == 7531 and summary['overlays'] == 22 and
            summary['excluded_dirty_versions'] == 16, 'Source scope mismatch')
    require(summary['host_original_result']['passed'] is True and
            summary['host_original_result']['groups'] == 10 and
            summary['native_build_original_result']['passed'] is True and
            len(summary['native_build_original_result']['targets']) == 5,
            'Host/build scope mismatch')
    top = json.loads((root / 'native-qualification/qualification-result.json').read_bytes())
    require(top['passed'] is True and len(top['cases']) == 5, 'Original qualification scope mismatch')
    prelaunch_path = 'host-prelaunch-not-run/01-studio-output-safety-qualification-1790991832570530000/qualification-result.json'
    prelaunch = json.loads((root / prelaunch_path).read_bytes())
    require(prelaunch['passed'] is False and prelaunch['cases'] == [], 'Original NOT RUN record changed')
    out.mkdir()
    controls = []
    def record(name, kind, scope, origin=None, details=None):
        data = (out / name).read_bytes()
        row = {'path': name, 'bytes': len(data), 'sha256': sha(data), 'kind': kind, 'scope': scope}
        if origin is not None:
            row['origin'] = identity(origin)
        if details is not None:
            row['derivation'] = details
        controls.append(row)
    def copy(name, origin, scope):
        require(not (out / name).exists(), 'Duplicate compact destination')
        data = origin.read_bytes()
        (out / name).write_bytes(data)
        require((out / name).read_bytes() == data, 'Compact byte copy failed')
        record(name, 'exact_byte_copy', scope, origin)
    def exact_gzip(name, origin, scope):
        require(not (out / name).exists(), 'Duplicate compact gzip destination')
        original = origin.read_bytes()
        with (out / name).open('xb') as stream:
            with gzip.GzipFile(filename='', fileobj=stream, mode='wb', mtime=0, compresslevel=9) as gz:
                gz.write(original)
        require(gzip.decompress((out / name).read_bytes()) == original, 'Compact exact gzip failed')
        record(name, 'gzip_exact_byte_derivation', scope, origin,
               {'method': 'gzip level9, mtime0, empty filename; original bytes/fields retained without reformatting'})
    archive_path = out / 'payload.tar.gz'
    with archive_path.open('xb') as stream:
        write_archive(root, rows, stream)
    archive_verification = verify_archive(archive_path, rows)
    with tempfile.TemporaryFile() as second:
        write_archive(root, rows, second)
        second.seek(0)
        with archive_path.open('rb') as first:
            while True:
                a, b = first.read(1024 * 1024), second.read(1024 * 1024)
                require(a == b, 'Second deterministic archive is not byte-identical')
                if not a:
                    break
    archive_verification['second_independent_archive_write_byte_identical'] = True
    record('payload.tar.gz', 'lossless_archive_derivation', 'Every full-package regular member, including all controls and historical failures',
           details={'input_directory': str(root), 'member_count': len(rows),
                    'member_bytes': sum(r['bytes'] for r in rows), 'format': 'POSIX PAX tar + gzip level9',
                    'metadata': 'sorted paths; uid/gid/mtime0; empty uname/gname/gzip filename; original file modes',
                    'verification': archive_verification})
    member_index = {
        'schema': 'protracker-studio-output-safety-compact-member-index-v1',
        'scope': 'Lossless logical archive membership/byte identities only; no new product/target acceptance',
        'full_package_directory': str(root), 'full_package_regular_members': len(rows),
        'full_package_member_bytes': sum(r['bytes'] for r in rows),
        'full_package_provenance': identity(root / PROVENANCE),
        'full_package_sha256sums': identity(root / 'SHA256SUMS'),
        'payload': identity(archive_path), 'members': rows,
        'archive_verification': archive_verification,
    }
    member_bytes = json_bytes(member_index)
    with (out / 'member-index.json.gz').open('xb') as stream:
        with gzip.GzipFile(filename='', fileobj=stream, mode='wb', mtime=0, compresslevel=9) as gz:
            gz.write(member_bytes)
    require(gzip.decompress((out / 'member-index.json.gz').read_bytes()) == member_bytes, 'Member index gzip mismatch')
    record('member-index.json.gz', 'authored_index_gzip_derivation', 'Every original member path/size/hash/mode and original full control identities',
           details={'uncompressed_bytes': len(member_bytes), 'uncompressed_sha256': sha(member_bytes),
                    'gzip_method': 'level9, mtime0, empty filename', 'inventory_source': str(root)})
    exact_gzip('full-provenance.json.gz', root / PROVENANCE, 'Every original full-package provenance field/byte retained')
    for name, relative, scope in [
        ('source-manifest.json.gz', 'source/source-manifest.json.gz', 'Exact existing compressed7531-input/22-overlay/16-dirty-exclusion original manifest'),
        ('host-checks.json.gz', 'host/host-checks.json.gz', 'Exact existing compressed ten-group host ASan/UBSan original report'),
        ('native-build.json.gz', 'native-build/manifest.json.gz', 'Exact existing compressed five-target pinned cross-build original report'),
        ('baseline-before.json', 'regression/before/result.json', 'Exact expected before-fix regressionRC20 report'),
        ('baseline-after.json', 'regression/after/result.json', 'Exact identical-source after-fix regressionRC0 report'),
        ('prelaunch-not-run.json', prelaunch_path, 'Exact first original prelaunchfalse/emptycases record; no guest launch'),
        ('root-reported-coordination.json', 'native-qualification/root-reported-coordination.json', 'Exact historical/root-reported coordination including releaseACK43; no packaging live verification'),
        ('native-root-review.json', 'native-qualification/extra/native-root-review.json', 'Exact root-reported original copied qualification review'),
        ('prelaunch-root-report.json', 'native-qualification/extra/prelaunch-root-report.json', 'Exact root-reported first attempt HOST PRELAUNCH NOT RUN/releaseACK35'),
        ('full-package-summary.json', 'package-summary.json', 'Exact original full-package authored scope summary'),
        ('ORIGINAL_README.md', 'README.md', 'Exact original full-package README; paths refer to payload archive members'),
        ('original-gzip-derivations.json', 'DERIVATIONS.json', 'Exact original uncompressed identities for existing source/host/build gzip controls'),
    ]:
        copy(name, root / relative, scope)
    copy('full-package-qa.json', qa_origin, 'Exact external read-only full-package integrity/copy consistency QA, not target acceptance')
    copy('compact-builder.py', Path(__file__).resolve(), 'Exact executed private compact packager helper; no repository or target operations')
    exact_gzip('qualification-result.json.gz', root / 'native-qualification/qualification-result.json',
               'Exact original top qualification bytes; distinct nested records remain separate in payload')
    exact_gzip('coordination-history.json.gz', root / 'native-qualification/extra/coordination-history.json',
               'Exact original historical/root-reported coordination fields including prior releaseACK35')
    readme = '''# Studio output preservation: compact evidence handoff

`payload.tar.gz` contains **all 1,883 regular files / 29,547,045 original bytes** from the verified full evidence package. No member is truncated or omitted. `member-index.json.gz` records every member path, size, SHA256 and original mode. Archive members have safe relative names and fixed uid/gid/mtime0; original modes are retained. A second archive write was byte-identical, and every member was verified through a tar reader without extraction. `full-provenance.json.gz` preserves the original full provenance bytes/fields, including original source paths and hashes. The full package remains unchanged.

The combined immutable candidate has 7,531 source inputs, 22 authorized overlays and 16 excluded unrelated dirty versions, based on commit e3cf67d30d13813b959b0e28acbc503927a578a5. Exact source/compiler/runtime/dependency/command records, generated sources and objects, test binaries, and logs remain in the archive. Ten host ASan/UBSan groups passed; five pinned Exec targets cross-built and five injected native fixtures passed. The original top qualification is copied separately as `qualification-result.json.gz`; its distinct nested result/identity/cleanup/independent records remain separate under `native-qualification/records/` inside the archive.

The identical independent regression probe records the expected before-fix RC20 and after-fix RC0. Core/integration development history and failures remain under `development/` in the archive. The first sandbox ps EPERM attempt remains **HOST PRELAUNCH NOT RUN**, with original `passed:false` / `cases:[]`, separate from the later native PASS. Historical/root-reported coordination records preserve release ACK35 for that attempt and release ACK43 after the later five-case cleanup; packaging does not verify control/release live or convert coordination reports into product acceptance.

Native allocation counts were 301/300/183/530/278 Fast allocations, each with zero owned bytes; the original reports preserve separate locked independent cleanup checks, 11 absence observations, original sole PID19081/profile, running guest and all four Paula DMA channels off. These are injected source/output ownership fixtures. **No actual device/card/MMIO, audio, timing, physical hardware or human listening acceptance is claimed.**

Readable controls are exact copies or exact-byte gzip encodings, never replacements for the complete archive. `original-gzip-derivations.json` preserves uncompressed original identities for the already-compressed source/host/build controls. `full-package-qa.json` is the copied external integrity QA. `ORIGINAL_README.md` is unchanged; all paths it names refer to archive members after unpacking into a new private directory. `compact-index.json` identifies every compact copy/derivation and its origin; `SHA256SUMS` covers every compact file except itself. No product source, repository evidence, original helper, target or shared lock was changed by this compact handoff preparation.
'''
    (out / 'README.md').write_text(readme)
    record('README.md', 'authored_scope_note', 'Readable scope and lossless archive/control navigation; historical coordination explicitly root-reported')
    require(inventory(root) == rows, 'Full originals changed during compact packaging')
    index = {
        'schema': 'protracker-studio-output-safety-compact-provenance-v1',
        'scope': 'Private lossless archive/copy/derivation integrity only; no new execution/cleanup/release/audio/timing/physical/listening acceptance',
        'full_package_directory': str(root), 'full_package_regular_members': len(rows),
        'full_package_member_bytes': sum(r['bytes'] for r in rows),
        'full_provenance_identity': identity(root / PROVENANCE),
        'full_sha256sums_identity': identity(root / 'SHA256SUMS'),
        'full_input_verification': full_verification,
        'full_inputs_before_after_identical': True,
        'archive_verification': archive_verification,
        'items': controls,
        'control_file_exclusions_from_items': ['compact-index.json', 'SHA256SUMS'],
    }
    (out / 'compact-index.json').write_bytes(json_bytes(index))
    compact_rows = inventory(out)
    (out / 'SHA256SUMS').write_text(''.join(r['sha256'] + '  ' + r['path'] + '\n' for r in compact_rows))
    final_rows = inventory(out)
    require(set(r['path'] for r in final_rows) == set(r['path'] for r in controls) | {'compact-index.json', 'SHA256SUMS'},
            'Compact index does not cover every path')
    final_lookup = {r['path']: r for r in final_rows}
    final_sums = {}
    for line in (out / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        safe_name(name)
        require(name not in final_sums, 'Duplicate compact checksum row')
        final_sums[name] = digest
    require(set(final_sums) == set(final_lookup) - {'SHA256SUMS'}, 'Compact checksum coverage mismatch')
    require(all(final_lookup[p]['sha256'] == digest for p, digest in final_sums.items()),
            'Compact checksum identity mismatch')
    for row in controls:
        data = (out / row['path']).read_bytes()
        require(len(data) == row['bytes'] and sha(data) == row['sha256'], 'Compact indexed identity changed')
        if 'origin' in row:
            original = Path(row['origin']['path']).read_bytes()
            require(len(original) == row['origin']['bytes'] and sha(original) == row['origin']['sha256'],
                    'Compact control origin changed')
            if row['kind'] == 'exact_byte_copy':
                require(data == original, 'Compact exact copy changed')
            elif row['kind'] == 'gzip_exact_byte_derivation':
                require(gzip.decompress(data) == original, 'Compact gzip original changed')
    require(inventory(root) == rows, 'Full originals changed during final verification')
    qa = {
        'scope': 'Archive and copy/derivation integrity only; no target operations or new acceptance',
        'package': str(out), 'files': len(final_rows), 'bytes': sum(r['bytes'] for r in final_rows),
        'full_original_files': len(rows), 'full_original_bytes': sum(r['bytes'] for r in rows),
        'full_originals_before_after_identical': True, 'full_input_verification': full_verification,
        'payload': identity(archive_path), 'archive_verification': archive_verification,
        'compact_index': identity(out / 'compact-index.json'),
        'sha256sums': identity(out / 'SHA256SUMS'), 'checksum_rows': len(compact_rows),
        'all_compact_checksums_verified_unique_complete': True,
        'indexed_copies_and_derivations': len(controls),
        'all_compact_origin_bytes_and_hashes_verified': True,
        'full_provenance_decompression_exact': True, 'member_index_decompression_exact': True,
        'every_original_top_nested_and_failure_file_retained': True,
        'qualification_fields_unchanged': True, 'prior_not_run_fields_unchanged': True,
        'root_coordination_is_copied_root_report_only': True,
    }
    qa_out.write_bytes(json_bytes(qa))
    print(json.dumps({'package': str(out), 'qa': str(qa_out), 'files': qa['files'], 'bytes': qa['bytes'],
                      'payload': qa['payload'], 'compact_index': qa['compact_index'],
                      'sha256sums': qa['sha256sums'], 'qa_identity': identity(qa_out)}, indent=2))


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print('COMPACT PACKAGING FAILED; existing output preserved: ' + repr(error), file=sys.stderr)
        raise
