from pathlib import Path
import sys,fcntl,json,asyncio,shutil,hashlib,time,os
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra');sys.path.insert(0,str(infra/'scripts'))
import emulator
from shared_guest import Guest
out=Path('build/dev/mixed-cost-recovery-'+str(time.time_ns())).resolve();out.mkdir()
failed='render-files-1790801822251988000';print('EVIDENCE',out,flush=True)
with (infra/'runtime/test.lock').open('a') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
 g=Guest(infra,out);rows=emulator.processes();assert len(rows)==1 and emulator.owns(rows[0][1],'amiberry-030');oldpid=rows[0][0]
 status=g.command('GET_STATUS');audio=g.command('GET_AUDIO_STATE')
 assert 'Paused=false' in status.split('\t') and all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
 run=g.share/failed;launch=g.share/('launch-'+failed)
 assert run.is_dir() and launch.is_file()
 shutil.copy2(run/'mixed-owner/test.log',out/'failed-test.log');shutil.copy2(launch,out/'failed-launcher.txt')
 (out/'before.json').write_text(json.dumps({'pid':oldpid,'status':status,'audio':audio,'failed_run':str(run),'launcher':str(launch)},indent=2)+'\n')
 asyncio.run(emulator.main(['stop','--target','amiberry-030']))
 assert not emulator.processes();(out/'old-process-stopped.json').write_text(json.dumps({'old_pid':oldpid,'no_emulator_process':True})+'\n')
 # Old emulator address space and guest filesystem handles no longer exist.
 launch.unlink();shutil.rmtree(run)
 assert not os.path.lexists(run) and not os.path.lexists(launch)
 asyncio.run(emulator.main(['start','--target','amiberry-030']))
 g=Guest(infra,out);rows=emulator.processes();assert len(rows)==1 and rows[0][0]!=oldpid and emulator.owns(rows[0][1],'amiberry-030')
 status=g.command('GET_STATUS');audio=g.command('GET_AUDIO_STATE')
 assert 'Paused=false' in status.split('\t') and all('ch%d_dma=0'%i in audio.split('\t') for i in range(4))
 assert not os.path.lexists(run) and not os.path.lexists(launch)
 (out/'result.json').write_text(json.dumps({'passed':True,'old_pid':oldpid,'new_pid':rows[0][0],'status':status,'audio':audio,'owned_run_absent':True,'launcher_absent':True,'scope':'authorized recovery-only; old process stopped, fresh owned030 guest, no test launched'},indent=2)+'\n')
print('RECOVERY PASS',out,flush=True)
