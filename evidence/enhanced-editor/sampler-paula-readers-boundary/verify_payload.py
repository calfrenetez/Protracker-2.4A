#!/usr/bin/env python3
"""Read and verify saved proof bytes without extracting, compiling or running products."""
import hashlib, json, struct, tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    manifest = json.loads((ROOT / 'manifest.json').read_text())
    raw = (ROOT / 'payload.tar.gz').read_bytes()
    assert len(raw) == manifest['payload']['bytes'] and sha(raw) == manifest['payload']['sha256']
    rows = {r['path']: r for r in manifest['files']}
    assert len(rows) == len(manifest['files'])
    data = {}
    with tarfile.open(ROOT / 'payload.tar.gz') as tar:
        members = tar.getmembers()
        assert len(members) == len(rows)
        for member in members:
            name = member.name
            assert member.isfile() and name in rows and name not in data
            assert not Path(name).is_absolute() and '..' not in Path(name).parts
            value = tar.extractfile(member).read()
            assert len(value) == rows[name]['bytes'] and sha(value) == rows[name]['sha256']
            data[name] = value
    assert set(data) == set(rows)
    def read(name):
        return json.loads(data[name])
    source = read('records/boundary-source-manifest-final-v1.json')
    native_source = read('records/boundary-native-source-manifest-final-v1.json')
    assert native_source['files'] == source['source'] and len(source['source']) == 8099
    assert native_source['base_commit'] == source['base_commit'] == manifest['base_commit']
    assert len(source['owned']) == 6
    for name, row in source['owned'].items():
        assert sha(data['source/' + name]) == row['sha256'] == source['source'][name]
    groups = 0
    for lane, commands in (('boundary-host-final-v1', 6), ('root-host-final-v1', 9)):
        prefix = 'proof/' + lane + '/'
        report = read(prefix + 'host-checks.json')
        assert report['passed'] and len(report['commands']) == commands
        assert report['source_files_before'] == report['source_files_after'] == 8099
        assert all(c['product_returncode'] == 0 for c in report['commands'])
        assert all(g['passed'] for g in report['groups'])
        groups += len(report['groups'])
        for name, value in report['canonical_dependencies'].items():
            assert sha(data['source/' + name]) == value == source['source'][name]
        for product in report['products']:
            saved = data[prefix + 'products/' + Path(product['archive_path']).name]
            assert len(saved) == product['bytes'] and sha(saved) == product['sha256']
        for generated in report['generated_inputs']:
            saved = data[prefix + 'generated/' + Path(generated['archive_path']).name]
            assert len(saved) == generated['bytes'] and sha(saved) == generated['sha256']
    assert groups == 7
    products = []
    for lane, target_name, contract_name, units in (
            ('native-genuine-final-v1', 'PTPaulaReadersTest', 'genuine', 22),
            ('native-startup-final-v1', 'PTPaulaReadersStartupTest', 'startup', 22),
            ('native-boundary-final-v1', 'PTPaulaReadersBoundaryTest', 'boundary', 29)):
        prefix = 'proof/' + lane + '/'
        report = read(prefix + 'manifest.json')
        contract = read('preparation/native-preparation/' + contract_name + '-fixture-contract.json')
        assert report['passed'] and report['native_NOT_RUN']
        assert len(report['commands']) == units + 11 and all(c['returncode'] == 0 for c in report['commands'])
        assert report['source_files_before'] == report['source_files_after'] == 8099
        assert report['source_manifest']['sha256'] == sha(data['records/boundary-native-source-manifest-final-v1.json'])
        assert '-m68000' in report['flags'] and '-msoft-float' in report['flags'] and '-DNDEBUG' not in report['flags']
        assert len(report['runtime_inputs']) == 7 and set(report['targets']) == {target_name}
        target = report['targets'][target_name]
        assert target['source_inputs'] == contract['ordered_sources'] and len(target['source_inputs']) == units
        for name, value in target['dependencies'].items():
            assert sha(data['source/' + name]) == value == source['source'][name]
        binary = data[prefix + target_name]
        record = target['binary']
        marker = contract['marker'].encode()
        assert len(binary) == record['bytes'] and sha(binary) == record['sha256']
        assert struct.unpack('>I', binary[:4])[0] == 0x3f3 and binary.count(marker) == 1
        text = data[prefix + Path(target['preprocessed_fixture']['path']).name]
        assert b'__assert_func' in target['last_assert_macro'].encode() and text.count(b'__assert_func (') >= 2
        assert text.count(marker) == 1
        products.append({'name': target_name, 'bytes': len(binary), 'sha256': sha(binary), 'native_NOT_RUN': True})
    print(json.dumps({'passed': True, 'files': len(data), 'host_groups': groups, 'payload': manifest['payload'],
                      'products': products, 'scope': 'Saved host/compiler evidence only; no native/target/physical/activation/timing/audio/listening execution.'}))

if __name__ == '__main__':
    main()
