#!/usr/bin/env python3
"""Verify packet bytes without producer imports, archive extraction or execution."""
import argparse,hashlib,json,stat,tarfile
from pathlib import Path,PurePosixPath

def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--origins',action='store_true');args=ap.parse_args()
 root=Path(__file__).resolve().parent
 names={'README.md','manifest.json','payload.tar.gz','verify_payload.py','SHA256SUMS'}
 assert {p.name for p in root.iterdir()}==names
 rows=(root/'SHA256SUMS').read_text().splitlines();seen=set()
 for row in rows:
  digest,name=row.split('  ',1);assert name in names-{'SHA256SUMS'} and name not in seen
  p=root/name;assert stat.S_ISREG(p.lstat().st_mode) and not p.is_symlink() and sha(p.read_bytes())==digest;seen.add(name)
 assert seen==names-{'SHA256SUMS'}
 m=json.loads((root/'manifest.json').read_text());rows=m['members'];expected={r['name']:r for r in rows};assert len(expected)==len(rows)==m['members_count']
 payload=root/'payload.tar.gz';assert payload.stat().st_size==m['payload']['bytes'] and sha(payload.read_bytes())==m['payload']['sha256']
 found=set();total=0
 with tarfile.open(payload,'r:gz') as tf:
  for t in tf:
   p=PurePosixPath(t.name);assert t.isfile() and not p.is_absolute() and str(p)==t.name and all(v not in ('.','..') for v in p.parts)
   assert t.name in expected and t.name not in found
   r=expected[t.name];b=tf.extractfile(t).read()
   assert len(b)==t.size==r['bytes'] and sha(b)==r['sha256'] and t.mode==r['mode']
   assert t.uid==t.gid==t.mtime==0 and t.uname==t.gname==''
   if args.origins:
    origin=Path(r['path']);s=origin.lstat();assert stat.S_ISREG(s.st_mode) and not origin.is_symlink() and stat.S_IMODE(s.st_mode)==r['mode'] and origin.read_bytes()==b
   found.add(t.name);total+=len(b)
 assert found==set(expected) and total==m['original_member_bytes']
 print(json.dumps({'passed':True,'members':len(found),'member_bytes':total,'origins_checked':args.origins,'scope':'Archive/control integrity only; native diagnostic NOT RUN'},sort_keys=True))
if __name__=='__main__':main()
