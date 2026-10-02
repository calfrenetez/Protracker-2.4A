import asyncio,importlib.util,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
infra=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra')
sys.path.insert(0,str(infra/'scripts'))
sys.path.insert(0,str(root/'tools'))
from emulator import processes
from shared_guest import Guest
spec=importlib.util.spec_from_file_location('physical_probe',root/'tools/shared_infra_amigus_discovery.py')
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)
original=probe.run
async def guarded(out,binary,result,*args):
 rows=processes()
 if len(rows)!=1:raise RuntimeError('No sole owned emulator before physical run')
 g=Guest(infra,out)
 result['emulator_process_before']=rows
 result['current_physical_scope']='Human resumed development/testing; distinct readonly status0.9 only; no playback retry'
 await original(out,binary,result,*args)
 rows_after=processes();result['emulator_process_after']=rows_after
 if rows_after!=rows:
  result['recovery_hold']=True;raise RuntimeError('Original emulator process changed during physical observation')
 final=Guest(infra,out)
 result['emulator_after']=final.command('GET_STATUS')
 if [x for x in result['emulator_after'].split('\t') if x.startswith('Paused=')]!=['Paused=false']:
  result['recovery_hold']=True;raise RuntimeError('Original emulator running state uncertain after physical run')
probe.run=guarded
probe.main()
