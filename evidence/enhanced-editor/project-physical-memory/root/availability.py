import fcntl,json,os,sys,time
from pathlib import Path
INFRA=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(INFRA/'scripts'))
from emulator import processes,owns
from bridge_checks import target
r={'scope':'Passive local lock/process/health availability; no target selection, guest commands or physical probe','time_ns':time.time_ns(),'locks':[]};fds=[]
try:
 for name in ['test.lock','real-a1200-safari.lock']:
  p=INFRA/'runtime'/name
  if not p.is_file() or p.is_symlink():raise RuntimeError('Unsafe lock path')
  fd=os.open(p,os.O_RDWR);fds.append(fd);fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);s=os.fstat(fd);q=p.stat()
  if (s.st_dev,s.st_ino)!=(q.st_dev,q.st_ino):raise RuntimeError('Lock identity changed')
  r['locks'].append({'path':str(p),'identity':[s.st_dev,s.st_ino],'free':True})
 rows=processes();r['processes']=rows
 if len(rows)!=1 or rows[0][0]!=19081 or not owns(rows[0][1],'amiberry-030'):raise RuntimeError('Original process identity unknown')
 s=target(INFRA,'amiberry-030');r['serial']={k:s.get(k) for k in ['host','port','connected']};r['passed']=True
except BaseException as e:r['passed']=False;r['error']=str(e);raise
finally:
 for fd in reversed(fds):os.close(fd)
 p=Path(__file__).parent/'availability-corrected.json';p.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
