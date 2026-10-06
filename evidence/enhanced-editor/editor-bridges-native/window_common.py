"""Exact saved-candidate custody checks. This module performs no target I/O."""
import hashlib,json,stat,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'v2'
REPO=Path('/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A')
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
COMMIT='5771d3ce1fcc3dee1967ec0c728e1fa654954bba'
NAME='PTEditorBridgesTest'
STAGE=INFRA/'runtime/Dev/Tests/PT-editor-bridges-20261006-v1'
SHA='21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e'
SIZE=385144
def fp(p):
 p=Path(p);s=p.lstat();assert stat.S_ISREG(s.st_mode) and not s.st_flags&0x40000000,str(p)
 b=p.read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def save(n,v):
 with (HERE/n).open('x') as f:json.dump(v,f,indent=2);f.write('\n')
def check_custody():
 pins=json.loads((HERE/'pins.json').read_text())['files']
 for p,h in pins.items():assert fp(p)==h,p
 controls=json.loads((BASE/'source-controls.json').read_text())
 for name,h in controls['protected'].items():
  p=REPO/name;assert fp(p)['sha256']==h['sha256'] and p.lstat().st_mode==h['mode'],name
 assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=REPO,text=True).strip()==COMMIT
 assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=REPO)
 build=json.loads((BASE/'native/manifest.json').read_text())
 host=json.loads((BASE/'host/execution.json').read_text())
 assert build['status']=='PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' and host['status']=='PASS_HOST_ONLY'
 assert build['source_stable'] and host['source_stable']
 assert build['source_before']==build['source_after']==host['source_before']==host['source_after']
 assert len(build['source_before'])==909
 for name,h in build['source_before'].items():assert fp(BASE/'candidate'/name)['sha256']==h,name
 for p,h in build['dependencies'].items():assert fp(p)=={k:h[k] for k in ('bytes','sha256')},p
 product=build['product'];assert product['bytes']==SIZE and product['sha256']==SHA
 assert fp(product['path'])=={'bytes':SIZE,'sha256':SHA}
 return product,len(pins)
def check_complete(data):
 expected=(BASE/'host/bridges-run.stdout').read_bytes()
 assert data==expected,'Complete native stdout differs from exact host assertion markers'
 assert len(data.splitlines())==3 and data.endswith(b'\n')
 return {'markerLines':3,'editorLegacy':15,'editorCheckedLifecycle':39,'wholeControlAliases':18,
         'bridgeLifecycle':9,'bridgeAliasesAndAdmission':75,'completeStdout':fp(BASE/'host/bridges-run.stdout'),
         'scope':'Actual portable assertion bodies on target CPU with injected clocks, voices and ordinary-RAM bus callbacks only'}
