import hashlib,json,sys,time
from pathlib import Path
assert __debug__
root=Path('/private/tmp/protracker-pcm-frame-root-z8bmyumb');infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
from emulator import processes
old=Path('/private/tmp/protracker-physical-project-root-5j11jtqn/idle-verification-1791039710236863000/result.json')
assert hashlib.sha256(old.read_bytes()).hexdigest()=='cd7b62c714da5719a6e262f07b044603cdb9fe6a859a4099d8d28b4bfb2aa5bb'
expected=json.loads(old.read_bytes())['observations'][0]['processes']
rows=[list(r) for r in processes()];assert rows==expected and len(rows)==1
profile=infra/'runtime/amiberry/Configurations/A1200-030-DEV.uae';sha=hashlib.sha256(profile.read_bytes()).hexdigest()
assert sha=='894aa5a61a2bf6267b461c6774a0bdc913a96e7f581ea7e4e3762c4d71709a30'
d={'scope':'Fresh read-only original sole emulator process/profile identity; no Guest/IPC/lock/target call','observed_time_ns':time.time_ns(),'process':rows[0],'profile':str(profile),'profile_sha256':sha}
p=root/'fresh-identity.json';assert not p.exists();p.write_text(json.dumps(d,indent=2)+'\n');print(json.dumps({'identity':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'process':rows[0],'profile_sha256':sha}))
