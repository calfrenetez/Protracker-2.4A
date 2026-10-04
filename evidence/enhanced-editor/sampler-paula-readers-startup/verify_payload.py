#!/usr/bin/env python3
"""Verify saved proof bytes and source/product bindings without extracting or executing anything."""
import hashlib,json,struct,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    manifest=json.loads((ROOT/'manifest.json').read_text());raw=(ROOT/'payload.tar.gz').read_bytes()
    assert len(raw)==manifest['payload']['bytes'] and sha(raw)==manifest['payload']['sha256']
    rows={r['path']:r for r in manifest['files']};assert len(rows)==len(manifest['files'])
    with tarfile.open(ROOT/'payload.tar.gz') as tar:
        members=tar.getmembers();assert len(members)==len(rows)
        data={}
        for member in members:
            name=member.name;assert member.isfile() and name in rows and name not in data
            assert not Path(name).is_absolute() and '..' not in Path(name).parts
            value=tar.extractfile(member).read();row=rows[name]
            assert len(value)==row['bytes'] and sha(value)==row['sha256'];data[name]=value
    assert set(data)==set(rows)
    def read(name):return json.loads(data[name])
    def source(name,version='v2'):
        overlay='source-overlays/startup-final-'+version+'/'+name
        return data[overlay if overlay in data else 'source/'+name]
    for lane,version in (('startup-host-v1','v1'),('startup-host-v2','v2'),('root-regression-proof','v2')):
        report=read('proof/'+lane+'/host-checks.json');assert report['passed']
        assert all(c['product_returncode']==0 for c in report['commands'])
        for name,value in report['canonical_dependencies'].items():assert sha(source(name,version))==value,name
        for product in report['products']:
            saved=data['proof/'+lane+'/products/'+Path(product['archive_path']).name]
            assert len(saved)==product['bytes'] and sha(saved)==product['sha256']
    markers={'PTPaulaReadersTest':b'SAMPLER PAULA READERS PASS: genuine master and selective Chip cache retained through 20 controls with command capacity2',
        'PTPaulaReadersStartupTest':b'PAULA READERS STARTUP PASS: bounded validation, cancellable preparation and checked transfer; software ownership only'}
    products=[]
    for lane in ('native-genuine','native-startup'):
        report=read('proof/'+lane+'/manifest.json');assert report['passed'] and report['native_NOT_RUN']
        assert len(report['commands'])==32 and all(c['returncode']==0 for c in report['commands'])
        assert '-m68000' in report['flags'] and '-msoft-float' in report['flags']
        assert len(report['runtime_inputs'])==7
        for name,target in report['targets'].items():
            for path,value in target['dependencies'].items():assert sha(source(path))==value,path
            binary=data['proof/'+lane+'/'+name];record=target['binary']
            assert len(binary)==record['bytes'] and sha(binary)==record['sha256']
            assert struct.unpack('>I',binary[:4])[0]==0x3f3 and binary.count(markers[name])==1
            text=data['proof/'+lane+'/'+Path(target['preprocessed_fixture']['path']).name]
            assert b'__assert_func' in target['last_assert_macro'].encode() and text.count(b'__assert_func (')>=2
            assert text.count(markers[name])==1
            products.append({'name':name,'bytes':len(binary),'sha256':sha(binary),'native_NOT_RUN':True})
    comparison=read('benchmark/comparison.json')
    assert comparison['before']['master_bytes']==comparison['after']['master_bytes']
    assert not comparison['before']['final_owned_bytes'] and not comparison['after']['final_owned_bytes']
    print(json.dumps({'passed':True,'files':len(data),'payload':manifest['payload'],'products':products,
        'scope':'Saved host/compiler evidence only. No native, target, physical, timing or audio execution.'}))
if __name__=='__main__':main()
