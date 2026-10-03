#!/usr/bin/env python3
"""Read-only packet verification; never extract/import/run archived software."""
from pathlib import Path,PurePosixPath
import argparse,gzip,hashlib,io,json,os,stat,tarfile

def sha(b):return hashlib.sha256(b).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
def regular(path):
    path=Path(path);before=path.lstat();need(stat.S_ISREG(before.st_mode),'not regular '+str(path));raw=path.read_bytes();after=path.lstat();need((before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_dev,after.st_ino,after.st_size,after.st_mtime_ns),'changed during read '+str(path));return raw,stat.S_IMODE(before.st_mode)
def archive(raws,members):
    buffer=io.BytesIO()
    with gzip.GzipFile(fileobj=buffer,mode='wb',mtime=0,filename='') as gz:
        with tarfile.open(fileobj=gz,mode='w',format=tarfile.PAX_FORMAT) as tf:
            for n in sorted(members):
                row=members[n];t=tarfile.TarInfo(n);t.size=row['bytes'];t.mode=row['mode'];t.uid=t.gid=t.mtime=0;t.uname=t.gname='';tf.addfile(t,io.BytesIO(raws[n]))
    return buffer.getvalue()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--origins',action='store_true');a=ap.parse_args();root=Path(__file__).resolve().parent
    need(set(x.name for x in root.iterdir())=={'README.md','SHA256SUMS','manifest.json','payload.tar.gz','verify_payload.py'},'unexpected packet path')
    d=json.loads(regular(root/'manifest.json')[0]);members=d['members'];payload=regular(root/'payload.tar.gz')[0];need(sha(payload)==d['payload']['sha256'] and len(payload)==d['payload']['bytes'],'payload mismatch')
    rows={}
    for line in regular(root/'SHA256SUMS')[0].decode().splitlines():
        h,n=line.split('  ',1);need(n not in rows and n in {'README.md','manifest.json','payload.tar.gz','verify_payload.py'},'checksum path');need(sha(regular(root/n)[0])==h,'control mismatch '+n);rows[n]=h
    need(len(rows)==4,'four bound controls required');raws={}
    with tarfile.open(fileobj=io.BytesIO(payload),mode='r:gz') as tf:
        seen=set()
        for t in tf:
            n=t.name;p=PurePosixPath(n);need(t.isfile() and n not in seen and n in members and not p.is_absolute() and '..' not in p.parts and str(p)==n,'unsafe member');seen.add(n)
            r=members[n];raw=tf.extractfile(t).read();need(t.size==r['bytes']==len(raw) and sha(raw)==r['sha256'] and t.mode==r['mode'] and t.uid==t.gid==t.mtime==0 and t.uname==t.gname=='','member mismatch '+n);raws[n]=raw
            if a.origins:
                actual,mode=regular(r['original_path']);need(actual==raw and mode==r['mode'],'original mismatch '+n)
        need(seen==set(members),'missing/extra archive member')
    need(archive(raws,members)==payload,'not deterministic');need(d['native_NOT_RUN'] is True and d['scope']=='host_and_native_compile_only','scope invalid')
    print(json.dumps({'passed':True,'members':len(members),'original_bytes':sum(r['bytes'] for r in members.values()),'payload_sha256':sha(payload),'all_modes_and_bytes':True,'deterministic_reconstruction':True,'origins_checked':a.origins,'native_NOT_RUN':True}))
if __name__=='__main__':main()
