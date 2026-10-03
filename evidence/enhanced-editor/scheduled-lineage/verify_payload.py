#!/usr/bin/env python3
"""Verify this packet without extracting or executing its archived software."""
from pathlib import Path,PurePosixPath
import argparse,gzip,hashlib,io,json,stat,tarfile

def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--origins',action='store_true');a=ap.parse_args();root=Path(__file__).resolve().parent
 d=json.loads((root/'manifest.json').read_bytes());members=d['members'];payload=(root/'payload.tar.gz').read_bytes();assert sha(payload)==d['payload']['sha256'] and len(payload)==d['payload']['bytes']
 rows={}
 for line in (root/'SHA256SUMS').read_text().splitlines():
  h,n=line.split('  ',1);assert n not in rows and '/' not in n and n in {'README.md','manifest.json','payload.tar.gz','verify_payload.py'};assert sha((root/n).read_bytes())==h;rows[n]=h
 assert len(rows)==4
 raws={}
 with tarfile.open(fileobj=io.BytesIO(payload),mode='r:gz') as tf:
  seen=set()
  for t in tf:
   n=t.name;p=PurePosixPath(n);assert t.isfile() and n not in seen and n in members and not p.is_absolute() and '..' not in p.parts;seen.add(n)
   r=members[n];raw=tf.extractfile(t).read();assert t.size==r['bytes']==len(raw) and sha(raw)==r['sha256'] and t.mode==r['mode'] and t.uid==t.gid==t.mtime==0 and t.uname==t.gname=='';raws[n]=raw
   if a.origins:
    origin=Path(r['original_path']);assert not origin.is_symlink() and origin.is_file() and origin.read_bytes()==raw and stat.S_IMODE(origin.stat().st_mode)==r['mode']
  assert seen==set(members)
 buf=io.BytesIO()
 with gzip.GzipFile(fileobj=buf,mode='wb',mtime=0,filename='') as gz:
  with tarfile.open(fileobj=gz,mode='w',format=tarfile.PAX_FORMAT) as tf:
   for n in sorted(members):
    r=members[n];t=tarfile.TarInfo(n);t.size=r['bytes'];t.mode=r['mode'];t.mtime=0;t.uid=t.gid=0;t.uname=t.gname='';tf.addfile(t,io.BytesIO(raws[n]))
 assert buf.getvalue()==payload
 print(json.dumps({'passed':True,'members':len(members),'original_bytes':sum(r['bytes'] for r in members.values()),'payload_sha256':sha(payload),'all_modes_and_bytes':True,'deterministic_reconstruction':True,'origins_checked':a.origins}))
if __name__=='__main__':main()
