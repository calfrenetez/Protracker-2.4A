"""Read archived bytes only. Does not compile, extract or execute a product."""
from pathlib import Path
import hashlib,json,re,tarfile,struct,sys

def sha(data):return hashlib.sha256(data).hexdigest()
def verify(root):
    m=json.loads((root/'manifest.json').read_bytes())
    payload=root/'payload.tar.gz'
    assert payload.stat().st_size==m['payload']['bytes'] and sha(payload.read_bytes())==m['payload']['sha256']
    data={}
    with tarfile.open(payload,'r:gz') as archive:
        for member in archive.getmembers():
            p=Path(member.name)
            assert member.isfile() and not p.is_absolute() and '..' not in p.parts and p.as_posix()==member.name
            assert member.name in m['members'] and member.name not in data
            raw=archive.extractfile(member).read(); row=m['members'][member.name]
            assert len(raw)==row['bytes'] and sha(raw)==row['sha256'] and member.mode==row['mode']
            data[member.name]=raw
    assert set(data)==set(m['members'])
    def j(name):return json.loads(data[name])
    def bound(row):
        name=m['path_members'][row['path']];raw=data[name]
        assert len(raw)==row['bytes'] and sha(raw)==row['sha256']
        return raw
    f=j(m['source_manifest']);h=j(m['host_proof']);n=j(m['native_proof']);failed=j(m['failed_host_proof'])
    assert f['production_delta']==0 and h['passed'] and n['passed'] and n['native_NOT_RUN']
    assert not failed['passed'] and [c['product_returncode'] for c in failed['commands']]==[0,-6]
    assert len(h['groups'])==1 and h['groups'][0]['passed']
    assert [c['product_returncode'] for c in h['commands']]==[0,0]
    queries=h['commands'][0]['dependency_commands'];assert len(queries)==30 and all(x['returncode']==0 for x in queries)
    for c in h['commands']:bound(c['output_log'])
    log=bound(h['commands'][1]['output_log']).decode();lines=log.splitlines()
    assert lines==m['host_runtime_lines'] and len(lines)==5
    product=h['products'][0];bound({'path':product['archive_path'],'bytes':product['bytes'],'sha256':product['sha256']})
    assert len(n['commands'])==41 and all(c['returncode']==0 for c in n['commands'])
    assert len([c for c in n['commands'] if c['label'].startswith('dependencies-')])==30
    for c in n['commands']:bound(c['log'])
    assert len(n['targets'])==1 and len(n['runtime_inputs'])==7
    t=n['targets']['PTPaulaReadersSongCompactTest'];binary=bound(t['binary'])
    assert struct.unpack('>I',binary[:4])[0]==0x3f3
    cpp=bound(t['preprocessed_fixture']).decode()
    assert '__assert_func' in t['last_assert_macro']
    assert cpp.count('__assert_func (')==115 and len(re.findall(r'__assert_func \(\s*"',cpp))==111
    for marker in m['native_CPP_HUNK_markers']:
        assert cpp.count(marker)==1 and binary.count(marker.encode())==1
    for deps in (h['canonical_dependencies'],t['dependencies']):
        for rel,value in deps.items():
            assert f['files'][rel]==value and sha(data[m['source_members'][rel]])==value
    for row in (h['source_manifest'],n['source_manifest'],h['pins'],n['pins'],n['fixture_contract'],n['builder']):bound(row)
    assert m['acceptance']['native_execution']==m['acceptance']['emulator']==m['acceptance']['physical']=='NOT_RUN'
    assert m['acceptance']['launcher_65536']=='NOT_CLEARED'
    return {'status':'PASS_SAVED_BYTES_ONLY','members':len(data),'canonical_sources':len(m['source_members']),
            'host':'PASS','native_build':'PASS','retained_host_V2_failure':'SIGABRT','native_execution':'NOT_RUN'}

if __name__=='__main__':
    print(json.dumps(verify(Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent),indent=2))
